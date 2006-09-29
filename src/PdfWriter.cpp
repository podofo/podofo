/***************************************************************************
 *   Copyright (C) 2005 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "PdfWriter.h"

#include "PdfDictionary.h"
#include "PdfDocument.h"
#include "PdfHintStream.h"
#include "PdfObject.h"
#include "PdfPage.h"
#include "PdfPagesTree.h"
#include "PdfParser.h"
#include "PdfStream.h"
#include "PdfVariant.h"

//#define PDF_MAGIC "%‚„œ”\n" //"%\0x25\0xe2\0xe3\0xcf\0xd3\0x0d"
#define PDF_MAGIC           "\xe2\xe3\xcf\xd3\n"
#define EMPTY_OBJECT_OFFSET   65535
#define XREF_ENTRY_SIZE       20
#define LINEARIZATION_PADDING 10

#include <iostream>

// for htonl
#ifdef _WIN32
#include <winsock2.h>
#undef GetObject
#else 
#include <arpa/inet.h>
#endif // _WIN32

namespace PoDoFo {

bool podofo_is_little_endian()
{ 
    int _p = 1;
    return (((char*)&_p)[0] == 1);
}

PdfWriter::PdfWriter( PdfParser* pParser )
    : m_pPagesTree( NULL ), m_bCompress( true ), m_bLinearized( false ), 
      m_bXRefStream( false ), m_lFirstInXRef( 0 )
{
    if( !pParser )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_eVersion     = pParser->GetPdfVersion();
    m_pTrailer     = new PdfObject( *(pParser->GetTrailer() ) );
    m_vecObjects   = pParser->m_vecObjects;
}

PdfWriter::PdfWriter( PdfDocument* pDocument )
    : m_pPagesTree( NULL ), m_bCompress( true ), m_bLinearized( false ), 
      m_bXRefStream( false ), m_lFirstInXRef( 0 )
{
    if( !pDocument )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_eVersion     = pDocument->GetPdfVersion();
    m_pTrailer     = new PdfObject( *(pDocument->GetTrailer() ) );
    m_vecObjects   = &(pDocument->m_vecObjects);
    m_pPagesTree   = pDocument->m_pPagesTree;
}

PdfWriter::PdfWriter( PdfVecObjects* pVecObjects, const PdfObject* pTrailer )
    : m_pPagesTree( NULL ), m_bCompress( true ), m_bLinearized( false ), 
      m_bXRefStream( false ), m_lFirstInXRef( 0 )
{
    if( !pVecObjects || !pTrailer )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_eVersion     = ePdfVersion_1_3;
    m_pTrailer     = new PdfObject( *pTrailer );
    m_vecObjects   = pVecObjects;
}

PdfWriter::~PdfWriter()
{
    delete m_pTrailer;
    m_pTrailer     = NULL;
    m_vecObjects   = NULL;
    m_pPagesTree   = NULL;
}

void PdfWriter::Write( const char* pszFilename )
{
    PdfOutputDevice device( pszFilename );

    this->Write( &device );
}

void PdfWriter::Write( PdfOutputDevice* pDevice )
{
    TVecXRefTable   vecXRef;
    TVecXRefOffset  vecXRefOffset;

    if( !pDevice )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    if( m_bLinearized ) 
    {
        this->WriteLinearized( pDevice );
    }
    else
    {
        WritePdfHeader  ( pDevice );
        WritePdfObjects ( pDevice, *m_vecObjects, &vecXRef );

        if( m_bXRefStream )
            WriteXRefStream( &vecXRef, pDevice );
        else
            WritePdfTableOfContents( &vecXRef, pDevice, &vecXRefOffset, false, m_bLinearized );
    }
}

void PdfWriter::WriteLinearized( PdfOutputDevice* pDevice )
{
    PdfObject*      pLinearize  = NULL;
    PdfPage*        pPage;
    PdfObject*      pLast;
    PdfHintStream*  pHint;
    PdfOutputDevice length;
    TVecXRefTable   vecXRef;
    TVecXRefOffset  vecXRefOffset;
    TIVecOffsets    it;

    // prepare the document for linearization
    this->FetchPagesTree();
    pPage = m_pPagesTree->GetPage( 0 );
    
    pLinearize = CreateLinearizationDictionary();
    pHint      = new PdfHintStream( m_vecObjects, m_pPagesTree );
    
    this->ReorderObjectsLinearized( pLinearize, pHint, pPage, &pLast );

    // The file is prepared for linearization,
    // so write it now.
    WritePdfHeader( pDevice );

    m_lLinearizedOffset = pDevice->Length();
    pLinearize->WriteObject( pDevice );

    // fill the XRef table with the objects
    WritePdfObjects( &PdfOutputDevice(), m_vecLinearized, &vecXRef );

    // prepend the linearization dictionary to the XRef table
    TXRefEntry entry;
    entry.lOffset     = m_lLinearizedOffset;
    entry.lGeneration = pLinearize->Reference().GenerationNumber();
    entry.cUsed       = 'n';
    
    vecXRef[0].nCount++;
    vecXRef[0].nFirst--; 
    vecXRef[0].vecOffsets.insert( vecXRef[0].vecOffsets.begin(), entry );

    // Calculate the length of the xref table
    WritePdfTableOfContents( &vecXRef, &length, &vecXRefOffset, true, false );
    
    it = vecXRef[0].vecOffsets.begin(); 
    it++; // skip the linearization dictionary, as it was written before the XRef table
          // and does already have a correct offset
    while( it != vecXRef[0].vecOffsets.end() )
    {
        (*it).lOffset += pDevice->Length() + length.Length();
        m_lLinearizedLastOffset = (*it).lOffset;
        ++it;
    }
    
    vecXRefOffset.clear();
    WritePdfTableOfContents( &vecXRef, pDevice, &vecXRefOffset, true, false );
    vecXRef.clear();

    WritePdfObjects( pDevice, m_vecLinearized, &vecXRef );
    vecXRef.clear();

    WritePdfObjects( pDevice, *m_vecObjects, &vecXRef );

    if( m_bXRefStream )
        WriteXRefStream( &vecXRef, pDevice );
    else
        WritePdfTableOfContents( &vecXRef, pDevice, &vecXRefOffset, false, m_bLinearized );

    this->FillLinearizationDictionary( pLinearize, pDevice, pPage, pLast, pHint, &vecXRefOffset );
}

void PdfWriter::WritePdfHeader( PdfOutputDevice* pDevice )
{
    pDevice->Print( "%s\n%%%s", s_szPdfVersions[(int)m_eVersion], PDF_MAGIC );
}

void PdfWriter::CompressObjects( const TVecObjects& vecObjects ) 
{
    TCIVecObjects itObjects  = vecObjects.begin();

    while( itObjects != vecObjects.end() )
    {
        // make sure that all objects are FlateDecoded if compression is enabled
        if( m_bCompress )
            (*itObjects)->FlateCompressStream();

        ++itObjects;
    }
}

void PdfWriter::WritePdfObjects( PdfOutputDevice* pDevice, const TVecObjects& vecObjects, TVecXRefTable* pVecXRef )
{
    TCIPdfReferenceList itFree     = vecObjects.GetFreeObjects().begin();
    TCIVecObjects       itObjects  = vecObjects.begin();
    int                 index;
    TXRefTable          tXRef;

    this->CompressObjects( vecObjects );

    tXRef.nFirst = (*itObjects)->ObjectNumber();

    index = vecObjects.size() + vecObjects.GetFreeObjects().size();
    if( tXRef.nFirst == 1 )
    {
        tXRef.nFirst--;
        index++;
    }

    tXRef.vecOffsets.resize( index );
    tXRef.nCount = tXRef.vecOffsets.size();

    // add the first free object
    if( !tXRef.nFirst )
    {
        tXRef.vecOffsets[0].lOffset     = (itFree == vecObjects.GetFreeObjects().end() ? 0 : (*itFree).ObjectNumber());
        tXRef.vecOffsets[0].lGeneration = EMPTY_OBJECT_OFFSET;
        tXRef.vecOffsets[0].cUsed       = 'f';
    }

    while( itObjects != vecObjects.end() )
    {
        index = (*itObjects)->ObjectNumber() - tXRef.nFirst;

        tXRef.vecOffsets[index].lOffset     = pDevice->Length();
        tXRef.vecOffsets[index].lGeneration = (*itObjects)->GenerationNumber();
        tXRef.vecOffsets[index].cUsed       = 'n';

        (*itObjects)->WriteObject( pDevice );

        ++itObjects;
    }

    while( itFree != vecObjects.GetFreeObjects().end() )
    {
        if( !((*itFree).ObjectNumber() > tXRef.nFirst &&
              (*itFree).ObjectNumber() < tXRef.nFirst + tXRef.nCount ) )
        {
            ++itFree;
            continue;
        }
            
        index = (*itFree).ObjectNumber() - tXRef.nFirst;

        ++itFree;

        if( index < (int)tXRef.vecOffsets.size() )
        {
            // write empty entries into the table
            tXRef.vecOffsets[index].lOffset     = (itFree == vecObjects.GetFreeObjects().end() ? 0 : (*itFree).ObjectNumber());
            tXRef.vecOffsets[index].lGeneration = (itFree == vecObjects.GetFreeObjects().end() ? 1 : 0);
            tXRef.vecOffsets[index].cUsed       = 'f';
        }
    }

    pVecXRef->push_back( tXRef );
}

void PdfWriter::WriteXRefEntries( PdfOutputDevice* pDevice, const TVecOffsets & vecOffsets )
{
    TCIVecOffsets     itOffsets = vecOffsets.begin();

    if( !pDevice )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    while( itOffsets != vecOffsets.end() )
    {
        pDevice->Print( "%0.10i %0.5i %c \n", (*itOffsets).lOffset, (*itOffsets).lGeneration, (*itOffsets).cUsed );
        ++itOffsets;
    }
}

void PdfWriter::WritePdfTableOfContents( TVecXRefTable* pVecXRef, PdfOutputDevice* pDevice, TVecXRefOffset* pVecXRefOffset, bool bDummyOffset, bool bShortTrailer )
{
    long              lXRef     = pDevice->Length();;
    unsigned int      nSize     = 0;
    TCIVecXRefTable   it;
    PdfObject         trailer;

    pDevice->Print( "xref\n" );

    it = pVecXRef->begin();
    while( it != pVecXRef->end() )
    {
        nSize = ( nSize > (*it).nFirst + (*it).nCount ? nSize : (*it).nFirst + (*it).nCount );

        // when there is only one, then we need to start with 0 and the bogus object...
        pDevice->Print( "%u %u\n", (*it).nFirst, (*it).nCount );
        
        if( it == pVecXRef->begin() )
            m_lFirstInXRef = pDevice->Length();

        WriteXRefEntries( pDevice, (*it).vecOffsets );
        
        ++it;
    }

    // if we have a dummy offset we write also a prev entry to the trailer
    FillTrailerObject( &trailer, nSize, bDummyOffset, bShortTrailer );
    
    pDevice->Print("trailer\n");
    if( bDummyOffset )
        m_lTrailerOffset = pDevice->Length();

    trailer.WriteObject( pDevice );
    pDevice->Print( "startxref\n%li\n%%%%EOF\n",  pVecXRefOffset->size() ? pVecXRefOffset->back() : (bDummyOffset ? 0 : lXRef)  );
    pVecXRefOffset->push_back( lXRef );
}

void PdfWriter::GetByteOffset( PdfObject* pObject, unsigned long* pulOffset )
{
    TCIVecObjects   it     = m_vecObjects->begin();
    PdfOutputDevice deviceHeader;

    if( !pObject || !pulOffset )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    this->WritePdfHeader( &deviceHeader );

    *pulOffset = deviceHeader.Length();

    while( it != m_vecObjects->end() )
    {
        if( (*it) == pObject )
            break;

        *pulOffset += (*it)->GetObjectLength();
        printf("Offset = %lu\n", *pulOffset );
        ++it;
    }
}

void PdfWriter::WriteToBuffer( char** ppBuffer, unsigned long* pulLen )
{
    PdfOutputDevice device;

    if( !pulLen )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    this->Write( &device );

    *pulLen = device.Length();
    *ppBuffer = (char*)malloc( *pulLen * sizeof(char) );
    if( !*ppBuffer )
    {
        RAISE_ERROR( ePdfError_OutOfMemory );
    }

    PdfOutputDevice memDevice( *ppBuffer, *pulLen );
    this->Write( &memDevice );
}

PdfObject* PdfWriter::CreateLinearizationDictionary()
{
    PdfObject*       pLinearize        = m_vecObjects->CreateObject();

    PdfVariant place_holder( (long)0 );
    place_holder.SetPaddingLength( LINEARIZATION_PADDING );

    PdfArray array;
    array.push_back( place_holder );
    array.push_back( place_holder );

    pLinearize->GetDictionary().AddKey( "Linearized", 1.0 );  // Version
    pLinearize->GetDictionary().AddKey( "L", place_holder );  // File length
    pLinearize->GetDictionary().AddKey( "H", array );         // Hint stream offset and length as PdfArray
    pLinearize->GetDictionary().AddKey( "E", place_holder );  // Offset of end of first page
    pLinearize->GetDictionary().AddKey( "N",                  // Number of pages in the document 
                                        (long)m_pPagesTree->GetTotalNumberOfPages() );             
    pLinearize->GetDictionary().AddKey( "O", place_holder );  // Object number of the first page
    pLinearize->GetDictionary().AddKey( "T", place_holder );  // Offset of first entry in main cross reference table

    return pLinearize;
}

void PdfWriter::ReorderObjectsLinearized( PdfObject* pLinearize, PdfHintStream* pHint, PdfPage* pPage, PdfObject** ppLast ) 
{
    TPdfReferenceList   lstLinearizedGroup;
    TPdfReferenceSet    setLinearizedGroup;
    TCIPdfReferenceList it;
    TIVecObjects        itObjects;
    PdfObject*          pObj;
    PdfObject*          pTmp = NULL;
    unsigned int        index, i;

    m_vecObjects->GetObjectDependencies( pPage->GetObject(), &lstLinearizedGroup );
    printf("FIRST PAGElstLinearizedGroup.size() = %i\n", lstLinearizedGroup.size() );

    pObj = m_vecObjects->GetObject( m_pTrailer->GetDictionary().GetKey( "Root" )->GetReference() );
    lstLinearizedGroup.push_back( pObj->Reference() );
    lstLinearizedGroup.push_back( pPage->GetObject()->Reference() );

    this->FindCatalogDependencies( pObj, "ViewerPreferences", &lstLinearizedGroup, true );
    this->FindCatalogDependencies( pObj, "PageMode", &lstLinearizedGroup, true );
    this->FindCatalogDependencies( pObj, "Threads", &lstLinearizedGroup, false );
    this->FindCatalogDependencies( pObj, "OpenAction", &lstLinearizedGroup, true );
    this->FindCatalogDependencies( pObj, "AcroForm", &lstLinearizedGroup, false );
    this->FindCatalogDependencies( pObj, "Encrypt", &lstLinearizedGroup, true );

    lstLinearizedGroup.push_back( pHint->GetObject()->Reference() );
    lstLinearizedGroup.push_back( pLinearize->Reference() );

    i  = m_vecObjects->size()-1;
    it = lstLinearizedGroup.begin();

    printf("lstLinearizedGroup.size() = %i\n", lstLinearizedGroup.size() );
    while( it != lstLinearizedGroup.end() )
    {
        index = m_vecObjects->GetIndex( *it );

        if( index < i ) 
        {
            pTmp                   = (*m_vecObjects)[index];
            (*m_vecObjects)[index] = (*m_vecObjects)[i];
            (*m_vecObjects)[i]     = pTmp;
        }

        i--;
        ++it;
    }

    std::copy( lstLinearizedGroup.begin(), lstLinearizedGroup.end(), std::inserter(setLinearizedGroup, setLinearizedGroup.begin()) );

    printf("RenumberObjects\n");
    m_vecObjects->RenumberObjects( m_pTrailer, &setLinearizedGroup );
    printf("RenumberObjects DONE\n");

    // reorder the objects in the file
    itObjects = m_vecObjects->begin();
    itObjects += m_vecObjects->size() - setLinearizedGroup.size();
    m_vecObjects->erase( itObjects );

    while( itObjects != m_vecObjects->end() )
    {
        m_vecLinearized.push_back_and_do_not_own( *itObjects );
        m_vecObjects->erase( itObjects ); 
    }
    
    *ppLast = m_vecLinearized.back();
}

void PdfWriter::FindCatalogDependencies( PdfObject* pCatalog, const PdfName & rName, TPdfReferenceList* pList, bool bWithDependencies )
{
    if( pCatalog->GetDictionary().HasKey( rName ) && pCatalog->GetDictionary().GetKey( rName )->IsReference() )
    {
        if( bWithDependencies )
            m_vecObjects->GetObjectDependencies( pCatalog->GetIndirectKey( rName ), pList );
        else
            pList->push_back( pCatalog->GetIndirectKey( rName )->Reference() );
    }
}

#ifdef WIN32
typedef signed char 	int8_t;
typedef unsigned char 	uint8_t;
typedef signed short 	int16_t;
typedef unsigned short 	uint16_t;
typedef signed int 	int32_t;
typedef unsigned int 	uint32_t;
#endif

#define STREAM_OFFSET_TYPE uint32_t

void PdfWriter::WriteXRefStream( TVecXRefTable* pVecXRef, PdfOutputDevice* pDevice, bool bDummyOffset )
{
    long                lXRef;
    unsigned int        nSize     = 0;
    TCIVecXRefTable     it;
    TCIVecOffsets       itOffsets;
    const size_t        bufferLen = 2 + sizeof( STREAM_OFFSET_TYPE );
    char                buffer[bufferLen];
    bool                bLittle   = podofo_is_little_endian();

    STREAM_OFFSET_TYPE* pValue    = (STREAM_OFFSET_TYPE*)(buffer+1);

    PdfObject           object( PdfReference( m_vecObjects->m_nObjectCount, 0 ), "XRef" );
    PdfArray            indeces;
    PdfArray            w;

    m_lFirstInXRef =    0;

    w.push_back( (long)1 );
    w.push_back( (long)sizeof(STREAM_OFFSET_TYPE) );
    w.push_back( (long)1 );

    it = pVecXRef->begin();
    while( it != pVecXRef->end() )
    {
        nSize = ( nSize > (*it).nFirst + (*it).nCount ? nSize : (*it).nFirst + (*it).nCount );

        indeces.push_back( (long)(*it).nFirst );
        indeces.push_back( (long)(*it).nCount );

        itOffsets = (*it).vecOffsets.begin();
        while( itOffsets != (*it).vecOffsets.end() )
        {
            if( (*itOffsets).cUsed == 'n' )
            {
                buffer[0]         = (char)1;
                buffer[bufferLen] = (char)0;
            }
            else if( (*itOffsets).cUsed == 'f' )
            {
                buffer[0]         = (char)0;
                buffer[bufferLen] = (char)1;
            }

            *pValue = (STREAM_OFFSET_TYPE)((*itOffsets).lOffset );
                
            if( bLittle )
                *pValue = htonl( *pValue );

            object.Stream()->Append( buffer, bufferLen );
            ++itOffsets;
        }
        
        ++it;
    }

    FillTrailerObject( &object, nSize, false, false );

    object.GetDictionary().AddKey( "Index", indeces );
    object.GetDictionary().AddKey( "W", w );
    object.FlateCompressStream();

    lXRef = bDummyOffset ? 0 : pDevice->Length();
    object.WriteObject( pDevice );
    pDevice->Print( "startxref\n%li\n%%%%EOF\n", lXRef );
}

void PdfWriter::FillTrailerObject( PdfObject* pTrailer, long lSize, bool bPrevEntry, bool bOnlySizeKey )
{
    PdfVariant place_holder( (long)0 );
    place_holder.SetPaddingLength( LINEARIZATION_PADDING );

    pTrailer->GetDictionary().AddKey( PdfName::KeySize, lSize );

    if( !bOnlySizeKey ) 
    {
        if( m_pTrailer->GetDictionary().HasKey( "Root" ) )
            pTrailer->GetDictionary().AddKey( "Root", m_pTrailer->GetDictionary().GetKey( "Root" ) );
        if( m_pTrailer->GetDictionary().HasKey( "Encrypt" ) )
            pTrailer->GetDictionary().AddKey( "Encrypt", m_pTrailer->GetDictionary().GetKey( "Encrypt" ) );
        if( m_pTrailer->GetDictionary().HasKey( "Info" ) )
            pTrailer->GetDictionary().AddKey( "Info", m_pTrailer->GetDictionary().GetKey( "Info" ) );
        if( m_pTrailer->GetDictionary().HasKey( "ID" ) )
            pTrailer->GetDictionary().AddKey( "ID", m_pTrailer->GetDictionary().GetKey( "ID" ) );
        
        if( bPrevEntry )
        {
            pTrailer->GetDictionary().AddKey( "Prev", place_holder );
        }
    }
}

void PdfWriter::FetchPagesTree() 
{
    if( !m_pPagesTree )
    {
        // try to find the pages tree
        PdfObject* pRoot = m_pTrailer->GetDictionary().GetKey( "Root" );

        if( !pRoot || !pRoot->IsReference() )
        {
            RAISE_ERROR( ePdfError_InvalidDataType );
        }

        printf("Fetching: %i\n", pRoot->GetReference().ObjectNumber() );
        printf("Size    : %i\n", m_vecObjects->size() );
        pRoot            = m_vecObjects->GetObject( pRoot->GetReference() );
        if( !pRoot ) 
        {
            RAISE_ERROR( ePdfError_InvalidHandle );
        }

        m_pPagesTree     = new PdfPagesTree( pRoot->GetIndirectKey( "Pages" ) );
    }
}

void PdfWriter::FillLinearizationDictionary( PdfObject* pLinearize, PdfOutputDevice* pDevice, PdfPage* pPage, PdfObject* pLast, PdfHintStream* pHint, TVecXRefOffset* pVecXRefOffset )
{
    long            lFileSize        = pDevice->Length();
    PdfVariant      value( (long)0 );
    PdfArray        hints;
    PdfObject       trailer;

    value.SetPaddingLength( LINEARIZATION_PADDING );

    value.SetNumber( lFileSize );
    pLinearize->GetDictionary().AddKey( "L", value );
    value.SetNumber( pPage->GetObject()->ObjectNumber() );
    pLinearize->GetDictionary().AddKey( "O", value );
    value.SetNumber( m_lFirstInXRef );
    pLinearize->GetDictionary().AddKey( "T", value );
    value.SetNumber( m_lLinearizedLastOffset + pLast->GetObjectLength() );
    pLinearize->GetDictionary().AddKey( "E", value );

    value.SetNumber( m_lLinearizedOffset + pLinearize->GetObjectLength() );
    hints.push_back( value );
    value.SetNumber( pHint->GetObject()->GetObjectLength() );
    hints.push_back( value );
    pLinearize->GetDictionary().AddKey( "H", hints );

    pDevice->Seek( m_lLinearizedOffset );
    pLinearize->WriteObject( pDevice );
    pDevice->Seek( lFileSize );

    value.SetNumber( pVecXRefOffset->back() );
    FillTrailerObject( &trailer, pLast->ObjectNumber()+1, true, false );
    trailer.GetDictionary().AddKey("Prev", value );

    pDevice->Seek( m_lTrailerOffset );
    trailer.WriteObject( pDevice );
    pDevice->Seek( lFileSize );
}

};

