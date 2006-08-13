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
#include "PdfParser.h"
#include "PdfVariant.h"

//#define PDF_MAGIC "%âãÏÓ\n" //"%\0x25\0xe2\0xe3\0xcf\0xd3\0x0d"
#define PDF_MAGIC           "\xe2\xe3\xcf\xd3\n"
#define EMPTY_OBJECT_OFFSET 65535
#define XREF_ENTRY_SIZE     20

namespace PoDoFo {

PdfWriter::PdfWriter()
    : m_pTrailer( NULL )
{
    Clear();
}

PdfWriter::~PdfWriter()
{
    Clear();
}

void PdfWriter::Clear() 
{
    TIVecObjects it = m_vecObjects.begin();

    while( it != m_vecObjects.end() )
    {
        delete (*it);
        ++it;
    }

    m_vecObjects.clear();

    delete m_pTrailer;
    m_pTrailer     = NULL;
    m_eVersion     = ePdfVersion_1_3;

    m_pCatalog     = NULL;

    m_bCompress    = true;
}

void PdfWriter::Init()
{
    // clear everything - so that calling Init twice will work
    Clear();

    m_pTrailer = new PdfObject();
    m_pCatalog = m_vecObjects.CreateObject( "Catalog" );

    m_pTrailer->GetDictionary().AddKey( "Root", m_pCatalog->Reference() );
}

void PdfWriter::Init( PdfParser* pParser )
{
    if( !pParser )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_eVersion     = pParser->GetPdfVersion();
    this->Init( &(pParser->m_vecObjects), pParser->GetTrailer() );

    // clear the parsers object value
    // other PdfWriter and PdfParser
    // would delete the same objects
    pParser->m_vecObjects.clear();
}

void PdfWriter::Init( PdfDocument* pDocument )
{
    if( !pDocument )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_eVersion     = pDocument->GetPdfVersion();
    this->Init( &(pDocument->m_vecObjects), pDocument->GetTrailer() );

    // clear the parsers object value
    // other PdfWriter and PdfParser
    // would delete the same objects
    pDocument->m_vecObjects.clear();
}


void PdfWriter::Init( PdfVecObjects* pVecObjects, const PdfObject* pTrailer )
{
    const PdfObject* pObj;

    // clear everything - so that calling Init twice will work
    Clear();

    if( !pVecObjects || !pTrailer )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_pTrailer     = new PdfObject( *pTrailer );
    m_vecObjects   = *pVecObjects;

    // load the Catalog/Root object
    pObj = m_pTrailer->GetDictionary().GetKey( "Root" );
    if( !(pObj && pObj->IsReference() ) )
    {
        RAISE_ERROR( ePdfError_InvalidDataType );
    }

    PdfError::DebugMessage("/Catalog ref=%s\n", pObj->GetReference().ToString().c_str());
    m_pCatalog = m_vecObjects.GetObject( pObj->GetReference() );
    if( !m_pCatalog )
    {
        RAISE_ERROR_INFO( ePdfError_InvalidHandle, "No catalog dictionary found in the trailer." );
    }
}

void PdfWriter::Write( const char* pszFilename )
{
    PdfOutputDevice device( pszFilename );

    this->Write( &device );
}

void PdfWriter::Write( PdfOutputDevice* pDevice )
{
    // Start with an empty XRefTable
    m_vecXRef.clear();

    if( !pDevice )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    WritePdfHeader( pDevice );
    WritePdfObjects( pDevice, m_vecObjects );
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
            (*itObjects)->FlateDecodeStream();

        ++itObjects;
    }
}

void PdfWriter::WritePdfObjects( PdfOutputDevice* pDevice, const TVecObjects& vecObjects, bool bFillXRefOnly )
{
    TCIVecObjects      itObjects  = vecObjects.begin();
    TXRefEntry         tEntry; 
    unsigned int       nLast      = 0;
    TXRefTable         tXRef;
    TXRefTable*        pCur       = NULL;

    tEntry.lGeneration = 0;
    tXRef .nFirst      = 0;
    tXRef .nCount      = 0;

    this->CompressObjects( vecObjects );

    while( itObjects != vecObjects.end() )
    {
        tEntry.lOffset     = (*itObjects)->IsEmptyEntry() ? 0 : pDevice->Length();
        tEntry.lGeneration = (*itObjects)->IsEmptyEntry() ?  EMPTY_OBJECT_OFFSET : 0;
        tEntry.cUsed       = (*itObjects)->IsEmptyEntry() ? 'f' : 'n';

        if( !pCur || ++nLast != (*itObjects)->ObjectNumber() )
        {
            tXRef.nFirst = (*itObjects)->ObjectNumber();
            tXRef.nCount = 0;
            tXRef.vecOffsets.clear();
            
            m_vecXRef.push_back( tXRef );
            pCur = &(m_vecXRef.back());
        }
        
        pCur->vecOffsets.push_back( tEntry );
        pCur->nCount++;
        
        nLast = (*itObjects)->ObjectNumber();

        if( !bFillXRefOnly )
            (*itObjects)->WriteObject( pDevice );

        ++itObjects;
    }
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

	bool	onlyOneXRef = ( m_vecXRef.size() == 1 );

    it = m_vecXRef.begin();
    while( it != m_vecXRef.end() )
    {
        nSize = ( nSize > (*it).nFirst + (*it).nCount ? nSize : (*it).nFirst + (*it).nCount );

		// when there is only one, then we need to start with 0 and the bogus object...
		if ( onlyOneXRef ) {
			pDevice->Print( "%u %u\n", 0, (*it).nCount+1 );
			pDevice->Print( "%0.10i %0.5i %c \n", 0, 65535, 'f' );
		} else {
			pDevice->Print( "%u %u\n", (*it).nFirst, (*it).nCount );
		}
		WriteXRefEntries( pDevice, (*it).vecOffsets );

        ++it;
    }

    // write the trailer dictionary, see Reference manual: p. 73
    pDevice->Print( "trailer\n<<\n/Size %u\n", nSize );

    this->WriteTrailerKey( pDevice, m_pTrailer, "Root" );
    this->WriteTrailerKey( pDevice, m_pTrailer, "Encrypt" );
    this->WriteTrailerKey( pDevice, m_pTrailer, "Info" );
    this->WriteTrailerKey( pDevice, m_pTrailer, "ID" );

    pDevice->Print( ">>\nstartxref\n%li\n%%%%EOF\n", lXRef );
}

void PdfWriter::WriteTrailerKey( PdfOutputDevice* pDevice, const PdfObject* pTrailer, const PdfName & key ) 
{
    if( pTrailer->GetDictionary().HasKey( key ) ) 
    {
        key.Write( pDevice );
        pDevice->Print( " " );
        pTrailer->GetDictionary().GetKey( key )->Write( pDevice );
        pDevice->Print( "\n" );
    }
}
 
PdfObject* PdfWriter::RemoveObject( const PdfReference & ref )
{
    return m_vecObjects.RemoveObject( ref );
}

void PdfWriter::GetByteOffset( PdfObject* pObject, unsigned long* pulOffset )
{
    TVecObjects     vecObj = this->GetObjects();
    TCIVecObjects   it     = vecObj.begin();
    PdfOutputDevice deviceHeader;

    if( !pObject || !pulOffset )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    this->WritePdfHeader( &deviceHeader );

    *pulOffset = deviceHeader.Length();

    while( it != vecObj.end() )
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

};

