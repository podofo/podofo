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
    : m_pPagesTree( NULL ), m_bCompress( true ), m_bXRefStream( false )
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
    : m_pPagesTree( NULL ), m_bCompress( true ), m_bXRefStream( false )
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
    : m_pPagesTree( NULL ), m_bCompress( true ), m_bXRefStream( false )
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
    //PdfObject*     pLinearize  = NULL;
    PdfVecObjects  linearized;

    // Start with an empty XRefTable
    m_vecXRef.clear();

    if( !pDevice )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    /*
    if( m_bLinearized )
        pLinearize = CreateLinearizationDictionary();
    */

    WritePdfHeader( pDevice );
    
    /*
    if( m_bLinearized )
    {
        // cast to stl::vector so that the linearized PdfVecObjects
        // does not take owner ship of the linearization dictionary
        linearized.push_back_and_do_not_own( pLinearize );

        printf("linearized count=%i\n", linearized.size() );
        WritePdfObjects( pDevice, linearized );
        // write the table of conetns for the firs page
        WritePdfTableOfContents( pDevice );
        // WritePdfObjects fills the XRef table which we do not need right now
        m_vecXRef.clear();
    }
    */

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

    TXRefEntry          tEntry; 
    TXRefTable          tXRef;

    tEntry.lGeneration = 0;
    tXRef .nFirst      = 0;
    tXRef .nCount      = 0;

    this->CompressObjects( vecObjects );

    // add the first free object
    tEntry.lOffset     = (itFree == vecObjects.GetFreeObjects().end() ? 0 : (*itFree).ObjectNumber());
    tEntry.lGeneration = EMPTY_OBJECT_OFFSET;
    tEntry.cUsed       = 'f';
    tXRef.vecOffsets.push_back( tEntry );
    tXRef.nCount++;

    while( itObjects != vecObjects.end() )
    {
        while( itFree != vecObjects.GetFreeObjects().end() && (*itFree).ObjectNumber() < (*itObjects)->ObjectNumber() )
        {
            ++itFree;
    
            // write empty entries into the table
            tEntry.lOffset     = (itFree == vecObjects.GetFreeObjects().end() ? 0 : (*itFree).ObjectNumber());
            tEntry.lGeneration = (itFree == vecObjects.GetFreeObjects().end() ? 1 : 0);
            tEntry.cUsed       = 'f';

            tXRef.vecOffsets.push_back( tEntry );
            tXRef.nCount++;
        }

        tEntry.lOffset     = pDevice->Length();
        tEntry.lGeneration = (*itObjects)->GenerationNumber();
        tEntry.cUsed       = 'n';
        
        tXRef.vecOffsets.push_back( tEntry );
        tXRef.nCount++;
        
        if( !bFillXRefOnly )
            (*itObjects)->WriteObject( pDevice );

        ++itObjects;
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

    lXRef = pDevice->Length();
    pDevice->Print( "xref\n" );

    it = m_vecXRef.begin();
    while( it != m_vecXRef.end() )
    {
        nSize = ( nSize > (*it).nFirst + (*it).nCount ? nSize : (*it).nFirst + (*it).nCount );

        // when there is only one, then we need to start with 0 and the bogus object...
        pDevice->Print( "%u %u\n", (*it).nFirst, (*it).nCount );

        WriteXRefEntries( pDevice, (*it).vecOffsets );
        
        ++it;
    }

    m_pTrailer->GetDictionary().AddKey( PdfName::KeySize, (long)nSize );

    pDevice->Print("trailer\n");
    m_pTrailer->WriteObject( pDevice );
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
    PdfObject* pLinearize  = m_vecObjects->CreateObject();
    PdfPage*   pPage;

    PdfVariant place_holder( (long)0 );
    place_holder.SetPaddingLength( LINEARIZATION_PADDING );

    if( !m_pPagesTree )
    {
        // try to find the pages tree
        PdfObject* pRoot = m_pTrailer->GetDictionary().GetKey( "Root" );
        if( !pRoot || !pRoot->IsReference() )
        {
            RAISE_ERROR( ePdfError_InvalidDataType );
        }

        pRoot            = m_vecObjects->GetObject( pRoot->GetReference() );
        m_pPagesTree     = new PdfPagesTree( pRoot->GetIndirectKey( "Pages" ) );
    }

    pPage = m_pPagesTree->GetPage( 0 );
    
    pLinearize->GetDictionary().AddKey( "Linearized", 1.0 );  // Version
    pLinearize->GetDictionary().AddKey( "L", place_holder );  // File length
    pLinearize->GetDictionary().AddKey( "H", place_holder );  // Hint stream offset and length as PdfArray
    if( pPage )
        pLinearize->GetDictionary().AddKey( "O",              // Object number of first page's page object
                                            (long)pPage->Object()->ObjectNumber() );             
    pLinearize->GetDictionary().AddKey( "E", place_holder );  // Offset of end of first page
    pLinearize->GetDictionary().AddKey( "N",                  // Number of pages in the document 
                                        (long)m_pPagesTree->GetTotalNumberOfPages() );             
    pLinearize->GetDictionary().AddKey( "T", place_holder );  // Offset of first entry in main cross reference table

    return pLinearize;
}

#ifdef WIN32
typedef signed char 	int8_t;
typedef unsigned char 	uint8_t;
typedef signed short 	int16_t;
typedef unsigned short 	uint16_t;
typedef signed int 		int32_t;
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

    object.GetDictionary().AddKey( PdfName::KeySize, (long)nSize );
    object.GetDictionary().AddKey( "Index", indeces );
    object.GetDictionary().AddKey( "W", w );

    if( m_pTrailer->GetDictionary().HasKey( "Root" ) )
        object.GetDictionary().AddKey( "Root", m_pTrailer->GetDictionary().GetKey( "Root" ) );
    if( m_pTrailer->GetDictionary().HasKey( "Encrypt" ) )
        object.GetDictionary().AddKey( "Encrypt", m_pTrailer->GetDictionary().GetKey( "Encrypt" ) );
    if( m_pTrailer->GetDictionary().HasKey( "Info" ) )
        object.GetDictionary().AddKey( "Info", m_pTrailer->GetDictionary().GetKey( "Info" ) );
    if( m_pTrailer->GetDictionary().HasKey( "ID" ) )
        object.GetDictionary().AddKey( "ID", m_pTrailer->GetDictionary().GetKey( "ID" ) );

    object.FlateCompressStream();

    lXRef = pDevice->Length();
    object.WriteObject( pDevice );
    pDevice->Print( "startxref\n%li\n%%%%EOF\n", lXRef );
}

};

