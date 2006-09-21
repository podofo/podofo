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
    PdfObject*     pLinearize  = NULL;
    PdfPage*       pPage;
    PdfObject*     pLast;
    PdfHintStream* pHint;

    // Start with an empty XRefTable
    m_vecXRef.clear();

    if( !pDevice )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    // Might change the version of the PDF.
    // As a reason we have to create it before writing
    // the PDF header.
    if( m_bLinearized && !m_vecObjects->IsLinearizationClean() )
    {
        this->FetchPagesTree();
        pPage = m_pPagesTree->GetPage( 0 );

        pLinearize = CreateLinearizationDictionary( );
        pHint      = new PdfHintStream( m_vecObjects, m_pPagesTree );

        this->ReorderObjectsLinearized( pLinearize, pHint, pPage, &pLast );

        m_vecObjects->SetLinearizationClean();
        this->FillLinearizationDictionary( pLinearize, pHint, pPage, pLast );
    }

    WritePdfHeader( pDevice );
    
    WritePdfObjects( pDevice, *m_vecObjects );
    if( m_bXRefStream )
        WriteXRefStream( pDevice );
    else
        WritePdfTableOfContents( pDevice );
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

void PdfWriter::WritePdfObjects( PdfOutputDevice* pDevice, const TVecObjects& vecObjects, bool bFillXRefOnly )
{
    TCIPdfReferenceList itFree     = vecObjects.GetFreeObjects().begin();
    TCIVecObjects       itObjects  = vecObjects.begin();
    int                 index;
    TXRefTable          tXRef;

    this->CompressObjects( vecObjects );

    tXRef.vecOffsets.resize( vecObjects.size() + vecObjects.GetFreeObjects().size() + 1 );
    tXRef.nFirst = 0;
    tXRef.nCount = tXRef.vecOffsets.size();

    // add the first free object
    tXRef.vecOffsets[0].lOffset     = (itFree == vecObjects.GetFreeObjects().end() ? 0 : (*itFree).ObjectNumber());
    tXRef.vecOffsets[0].lGeneration = EMPTY_OBJECT_OFFSET;
    tXRef.vecOffsets[0].cUsed       = 'f';

    while( itObjects != vecObjects.end() )
    {
        index = (*itObjects)->ObjectNumber();

        tXRef.vecOffsets[index].lOffset     = pDevice->Length();
        tXRef.vecOffsets[index].lGeneration = (*itObjects)->GenerationNumber();
        tXRef.vecOffsets[index].cUsed       = 'n';

        if( !bFillXRefOnly )
            (*itObjects)->WriteObject( pDevice );

        ++itObjects;
    }

    while( itFree != vecObjects.GetFreeObjects().end() )
    {
        index = (*itFree).ObjectNumber();

        ++itFree;

        if( index < (int)tXRef.vecOffsets.size() )
        {
            // write empty entries into the table
            tXRef.vecOffsets[index].lOffset     = (itFree == vecObjects.GetFreeObjects().end() ? 0 : (*itFree).ObjectNumber());
            tXRef.vecOffsets[index].lGeneration = (itFree == vecObjects.GetFreeObjects().end() ? 1 : 0);
            tXRef.vecOffsets[index].cUsed       = 'f';
        }
    }

    m_vecXRef.push_back( tXRef );
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

void PdfWriter::WritePdfTableOfContents( PdfOutputDevice* pDevice )
{
    long              lXRef;
    unsigned int      nSize     = 0;
    TCIVecXRefTable   it;
    PdfObject         trailer;

    lXRef = pDevice->Length();
    pDevice->Print( "xref\n" );

    it = m_vecXRef.begin();
    while( it != m_vecXRef.end() )
    {
        nSize = ( nSize > (*it).nFirst + (*it).nCount ? nSize : (*it).nFirst + (*it).nCount );

        // when there is only one, then we need to start with 0 and the bogus object...
        pDevice->Print( "%u %u\n", (*it).nFirst, (*it).nCount );
        
        if( it == m_vecXRef.begin() )
            m_lFirstInXRef = pDevice->Length();

        WriteXRefEntries( pDevice, (*it).vecOffsets );
        
        ++it;
    }

    FillTrailerObject( &trailer, nSize );

    pDevice->Print("trailer\n");
    trailer.WriteObject( pDevice );
    pDevice->Print( "startxref\n%li\n%%%%EOF\n", lXRef );
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
    pLinearize->GetDictionary().AddKey( "T", place_holder );  // Offset of first entry in main cross reference table

    return pLinearize;
}

void PdfWriter::ReorderObjectsLinearized( PdfObject* pLinearize, PdfHintStream* pHint, PdfPage* pPage, PdfObject** ppLast ) 
{
    TPdfReferenceList   lstLinearizedGroup;
    TPdfReferenceSet    setLinearizedGroup;
    TCIPdfReferenceList it;
    PdfObject*          pObj;
    PdfObject*          pTmp = NULL;
    unsigned int        index, i;

    m_vecObjects->GetObjectDependencies( pPage->Object(), &lstLinearizedGroup );

    pObj = m_vecObjects->GetObject( m_pTrailer->GetDictionary().GetKey( "Root" )->GetReference() );
    lstLinearizedGroup.push_back( pObj->Reference() );
    lstLinearizedGroup.push_back( pPage->Object()->Reference() );

    this->FindCatalogDependencies( pObj, "ViewerPreferences", &lstLinearizedGroup, true );
    this->FindCatalogDependencies( pObj, "PageMode", &lstLinearizedGroup, true );
    this->FindCatalogDependencies( pObj, "Threads", &lstLinearizedGroup, false );
    this->FindCatalogDependencies( pObj, "OpenAction", &lstLinearizedGroup, true );
    this->FindCatalogDependencies( pObj, "AcroForm", &lstLinearizedGroup, false );
    this->FindCatalogDependencies( pObj, "Encrypt", &lstLinearizedGroup, true );

    lstLinearizedGroup.push_back( pHint->Object()->Reference() );
    lstLinearizedGroup.push_back( pLinearize->Reference() );

    i  = m_vecObjects->size()-1;
    it = lstLinearizedGroup.begin();

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

    m_vecObjects->RenumberObjects( m_pTrailer, &setLinearizedGroup );

    // reorder the objects in the file
    i      = setLinearizedGroup.size();
    index  = m_vecObjects->size()-i;

    while( i )
    {
        pTmp                   = (*m_vecObjects)[index];
        (*m_vecObjects)[index] = (*m_vecObjects)[setLinearizedGroup.size()-i];
        (*m_vecObjects)[setLinearizedGroup.size()-i] = pTmp;

        i--;
        index  = m_vecObjects->size()-i;
    }

    *ppLast = pTmp;
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

void PdfWriter::WriteXRefStream( PdfOutputDevice* pDevice )
{
    long                lXRef;
    unsigned int        nSize     = 0;
    TCIVecXRefTable     it;
    TCIVecOffsets       itOffsets;
    const size_t        bufferLen = 2 + sizeof( STREAM_OFFSET_TYPE );
    char                buffer[bufferLen];
    bool                bLittle   = podofo_is_little_endian();

    STREAM_OFFSET_TYPE* pValue    = (STREAM_OFFSET_TYPE*)(buffer+1);

    PdfObject           object( m_vecObjects->m_nObjectCount, 0, "XRef" );
    PdfArray            indeces;
    PdfArray            w;

    m_lFirstInXRef =    0;

    w.push_back( (long)1 );
    w.push_back( (long)sizeof(STREAM_OFFSET_TYPE) );
    w.push_back( (long)1 );

    it = m_vecXRef.begin();
    while( it != m_vecXRef.end() )
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

            *pValue = (STREAM_OFFSET_TYPE)(*itOffsets).lOffset;
                
            if( bLittle )
                *pValue = htonl( *pValue );

            object.Stream()->Append( buffer, bufferLen );
            ++itOffsets;
        }
        
        ++it;
    }

    FillTrailerObject( &object, nSize );

    object.GetDictionary().AddKey( "Index", indeces );
    object.GetDictionary().AddKey( "W", w );
    object.FlateCompressStream();

    lXRef = pDevice->Length();
    object.WriteObject( pDevice );
    pDevice->Print( "startxref\n%li\n%%%%EOF\n", lXRef );
}

void PdfWriter::FillTrailerObject( PdfObject* pTrailer, long lSize )
{
    pTrailer->GetDictionary().AddKey( PdfName::KeySize, lSize );

    if( m_pTrailer->GetDictionary().HasKey( "Root" ) )
        pTrailer->GetDictionary().AddKey( "Root", m_pTrailer->GetDictionary().GetKey( "Root" ) );
    if( m_pTrailer->GetDictionary().HasKey( "Encrypt" ) )
        pTrailer->GetDictionary().AddKey( "Encrypt", m_pTrailer->GetDictionary().GetKey( "Encrypt" ) );
    if( m_pTrailer->GetDictionary().HasKey( "Info" ) )
        pTrailer->GetDictionary().AddKey( "Info", m_pTrailer->GetDictionary().GetKey( "Info" ) );
    if( m_pTrailer->GetDictionary().HasKey( "ID" ) )
        pTrailer->GetDictionary().AddKey( "ID", m_pTrailer->GetDictionary().GetKey( "ID" ) );
}

void PdfWriter::FetchPagesTree() 
{
    if( !m_pPagesTree )
    {
        // try to find the pages tree
        PdfObject* pRoot = m_pTrailer->GetDictionary().GetKey( "Root" );
        PdfOutputDevice device( &(std::cout) );
        m_pTrailer->WriteObject( &device );

        if( !pRoot || !pRoot->IsReference() )
        {
            printf("pRoot=%p\n", pRoot );
            RAISE_ERROR( ePdfError_InvalidDataType );
        }

        pRoot            = m_vecObjects->GetObject( pRoot->GetReference() );
        m_pPagesTree     = new PdfPagesTree( pRoot->GetIndirectKey( "Pages" ) );
    }
}

void PdfWriter::FillLinearizationDictionary( PdfObject* pLinearize, PdfHintStream* pHint, PdfPage* pPage, PdfObject* pLast )
{
    PdfOutputDevice device;
    PdfOutputDevice device2;
    PdfVariant      value( (long)0 );
    PdfArray        hints;
    unsigned long   lPageOffset;

    value.SetPaddingLength( LINEARIZATION_PADDING );
    pLinearize->GetDictionary().AddKey( "O", (long)pPage->Object()->ObjectNumber() );             

    // fill the hint stream now
    this->Write( &device2 );
    pHint->Create( &m_vecXRef );
    m_vecXRef.clear(); // clean after writing

    // continue with the linearization dictionary
    this->Write( &device );

    value.SetNumber( device.Length() );
    pLinearize->GetDictionary().AddKey( "L", value );

    lPageOffset = m_vecXRef[0].vecOffsets[pLast->Reference().ObjectNumber()].lOffset;
    lPageOffset += pLast->GetObjectLength();

    value.SetNumber( lPageOffset );
    pLinearize->GetDictionary().AddKey( "E", value );

    // DS: evtl:  m_lFirstInXRef-1
    value.SetNumber( m_lFirstInXRef );
    pLinearize->GetDictionary().AddKey( "T", value );

    value.SetNumber( m_vecXRef[0].vecOffsets[pHint->Object()->Reference().ObjectNumber()].lOffset );
    hints.push_back( value );
    value.SetNumber( pHint->Object()->GetObjectLength() );
    hints.push_back( value );

    pLinearize->GetDictionary().AddKey( "H", hints );

    m_vecXRef.clear(); // clean after writing
}

};

