/***************************************************************************
 *   Copyright (C) 2006 by Dominik Seichter                                *
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
 *                                                                         *
 *   In addition, as a special exception, the copyright holders give       *
 *   permission to link the code of portions of this program with the      *
 *   OpenSSL library under certain conditions as described in each         *
 *   individual source file, and distribute linked combinations            *
 *   including the two.                                                    *
 *   You must obey the GNU General Public License in all respects          *
 *   for all of the code used other than OpenSSL.  If you modify           *
 *   file(s) with this exception, you may extend this exception to your    *
 *   version of the file(s), but you are not obligated to do so.  If you   *
 *   do not wish to do so, delete this exception statement from your       *
 *   version.  If you delete this exception statement from all source      *
 *   files in the program, then also delete it here.                       *
 ***************************************************************************/

#include "PdfHintStream.h"

#include "base/PdfDefinesPrivate.h"

#include "base/PdfData.h"
#include "base/PdfDictionary.h"
#include "base/PdfStream.h"
#include "base/PdfVariant.h"
#include "base/PdfVecObjects.h"

#include "PdfPage.h"
#include "PdfPagesTree.h"

// See PdfWriter.cpp
#define LINEARIZATION_PADDING "1234567890"

using namespace PoDoFo;

namespace {

struct TPageEntrySharedObjectInfo {
    pdf_uint16 nIndex;
    pdf_uint16 nNumerator;
};

typedef std::vector<TPageEntrySharedObjectInfo>       TVecPageEntrySharedObjectInfo;
typedef TVecPageEntrySharedObjectInfo::iterator       TIVecPageEntrySharedObjectInfo;
typedef TVecPageEntrySharedObjectInfo::const_iterator TCIVecPageEntrySharedObjectInfo;

#if 0
class PdfPageOffsetEntry {
public:
    PdfPageOffsetEntry()
        : nObjectsPerPage( 0 ),
          nPageLength( 0 ),
          nSharedObjects( 0 ),

          nContentsOffset( 0 ),
          nContentsLength( 0 )
    {
        vecSharedObjects.resize( 0 );
    }

    pdf_uint16 nObjectsPerPage;
    pdf_uint16 nPageLength;
    pdf_uint16 nSharedObjects;

    // item4 and item5:
    TVecPageEntrySharedObjectInfo vecSharedObjects;

    pdf_uint16 nContentsOffset;
    pdf_uint16 nContentsLength;

public:
    void Write( PoDoFo::NonPublic::PdfHintStream* pHint );
};

void PdfPageOffsetEntry::Write( PoDoFo::NonPublic::PdfHintStream* pHint )
{
    TCIVecPageEntrySharedObjectInfo it;

    pHint->WriteUInt16( nObjectsPerPage );
    pHint->WriteUInt16( nPageLength );
    pHint->WriteUInt16( nSharedObjects );

    it = vecSharedObjects.begin();
    while( it != vecSharedObjects.end() )
    {
        pHint->WriteUInt16( (*it).nIndex );
        ++it;
    }

    it = vecSharedObjects.begin();
    while( it != vecSharedObjects.end() )
    {
        pHint->WriteUInt16( (*it).nNumerator );
        ++it;
    }

    pHint->WriteUInt16( nContentsOffset );
    pHint->WriteUInt16( nContentsLength );
}
#endif

class PdfPageOffsetHeader {
public:
    PdfPageOffsetHeader()
        : nLeastNumberOfObjects( 0 ),
          nFirstPageObject( 0 ),
          nBitsPageObject( 0 ),
          nLeastPageLength( 0 ),
          nBitsPageLength( 0 ),
          nOffsetContentStream( 0 ),
          nBitsContentStream( 0 ),
          nLeastContentStreamLength( 0 ),
          nBitsLeastContentStreamLength( 0 ),
          nBitsNumSharedObjects( 0 ),
          nBitsGreatestSharedObject( 0 ),
          nItem12( 0 ),
          nItem13( 0 )
    {

    }

    // item1: The least number of objects in a page including the page itself
    pdf_uint32 nLeastNumberOfObjects;
    // item2: The location of the first pages page object
    pdf_uint32 nFirstPageObject; // (*pXRef)[0].vecOffsets[ m_pPagesTree->GetPage( 0 )->Object()->Reference().ObjectNumber() ].lOffset;
    // item3: The number of bits needed to represent the difference between the 
    //        greatest and least number of objects in a page
    pdf_uint16 nBitsPageObject; // (pdf_uint16)ceil( logb( (double)(max-least) ) );
    // item4: The least length of a page in bytes
    pdf_uint32 nLeastPageLength;
    // item5: The number of bits needed to represent the greatest difference 
    //        between the greatest and the least length of a page in bytes
    pdf_uint16 nBitsPageLength;
    // item6: The least offset of the start of a content stream, relative
    //        to the beginning of a file. 
    // --> Always set to 0 by acrobat
    pdf_uint32 nOffsetContentStream;
    // item7: The number of bits needed to represent the greatest difference 
    //        between the greatest and the least offset of a the start of a content
    //        stream relative to the beginning of a file
    // --> Always set to 0 by acrobat
    pdf_uint16 nBitsContentStream;
    // item8: The least content stream length
    pdf_uint32 nLeastContentStreamLength;
    // item9: The number of bits needed to represent the greatest difference 
    //        between the greatest and the least length of a content stream
    pdf_uint16 nBitsLeastContentStreamLength;
    // item10: The number of bits needed to represent the greatest number
    //         of shared object references.
    pdf_uint16 nBitsNumSharedObjects;
    // item11: The number of bits needed to represent the nummerically 
    //         greatest shared object identifyer used by pages
    pdf_uint16 nBitsGreatestSharedObject;
    // item12: 
    pdf_uint16 nItem12;
    // item13:
    pdf_uint16 nItem13;

    void Write( PoDoFo::NonPublic::PdfHintStream* pHint )
    {
        pHint->WriteUInt32( nLeastNumberOfObjects );
        pHint->WriteUInt32( nFirstPageObject );
        pHint->WriteUInt16( nBitsPageObject );
        pHint->WriteUInt32( nLeastPageLength );
        pHint->WriteUInt16( nBitsPageLength );
        pHint->WriteUInt32( nOffsetContentStream );
        pHint->WriteUInt16( nBitsContentStream );
        pHint->WriteUInt32( nLeastContentStreamLength );
        pHint->WriteUInt16( nBitsLeastContentStreamLength );
        pHint->WriteUInt16( nBitsNumSharedObjects );
        pHint->WriteUInt16( nBitsGreatestSharedObject );
        pHint->WriteUInt16( nItem12 );
        pHint->WriteUInt16( nItem13 );
    }

};

class PdfSharedObjectHeader {
public:
    PdfSharedObjectHeader() 
        : nFirstObjectNumber( 0 ),
          nFirstObjectLocation( 0 ),
          nNumSharedObjectsFirstPage( 0 ),
          nNumSharedObjects( 0 ),
          nNumBits( 0 ),
          nLeastLength( 0 ),
          nNumBitsLengthDifference( 0 )
    {
    }

    pdf_uint32 nFirstObjectNumber;
    pdf_uint32 nFirstObjectLocation;
    pdf_uint32 nNumSharedObjectsFirstPage;
    pdf_uint32 nNumSharedObjects; // i.e. including nNumSharedObjectsFirstPage
    pdf_uint16 nNumBits;
    pdf_uint32 nLeastLength;
    pdf_uint16 nNumBitsLengthDifference;

public:
    void Write( PoDoFo::NonPublic::PdfHintStream* pHint )
    {
        pHint->WriteUInt32( nFirstObjectNumber );
        pHint->WriteUInt32( nFirstObjectLocation );
        pHint->WriteUInt32( nNumSharedObjectsFirstPage );
        pHint->WriteUInt32( nNumSharedObjects );
        pHint->WriteUInt16( nNumBits );
        pHint->WriteUInt32( nLeastLength );
        pHint->WriteUInt16( nNumBitsLengthDifference );
    }
};

}; // end anon namespace

namespace PoDoFo {

namespace NonPublic {

PdfHintStream::PdfHintStream( PdfVecObjects* pParent, PdfPagesTree* pPagesTree )
    : PdfElement( NULL, pParent ), m_pPagesTree( pPagesTree )
{
    // This is overwritten later with valid data!
    PdfVariant place_holder( PdfData( LINEARIZATION_PADDING ) );
    this->GetObject()->GetDictionary().AddKey( "S", place_holder ); // shared object hint table
}

PdfHintStream::~PdfHintStream()
{

}

/*
void PdfHintStream::Create( TVecXRefTable* pXRef )
{
    this->CreatePageHintTable( pXRef );
    this->CreateSharedObjectHintTable();
}

void PdfHintStream::CreatePageHintTable( TVecXRefTable* pXRef )
{
    TPdfReferenceList   lstPages;
    TCIPdfReferenceList it;
    int                 i;
    int                 nPageCount = m_pPagesTree->GetTotalNumberOfPages();

    PdfPageOffsetHeader header;
#if 1
	// this will init/construct each of the objects in the vector
	// AND it compiles on all platforms - where the below code
	// isn't 100% valid for all C++ compilers
	std::vector< PdfPageOffsetEntry >	vecPages( nPageCount );
#else
    // use an array instead of an vector,
    // to make sure the constructors are called,
    // which they are apparently not when using vector.resize
    PdfPageOffsetEntry  vecPages[nPageCount];
#endif

    pdf_uint32        max;
    pdf_uint32        least = 0;
    pdf_uint32        maxNumberOfObjects = 0;
    pdf_uint32        maxPageLength      = 0;
    pdf_uint32        value;
    PdfReference      maxRef;

    for( i=0;i<nPageCount;i++ )
    {
        lstPages.clear();

        this->GetObject()->GetParent()->GetObjectDependencies( m_pPagesTree->GetPage( i )->GetObject(), &lstPages );
        vecPages[i].nObjectsPerPage = lstPages.size();

        if( !header.nLeastNumberOfObjects || header.nLeastNumberOfObjects > lstPages.size() )
            header.nLeastNumberOfObjects = lstPages.size();

        if( !maxNumberOfObjects || maxNumberOfObjects < lstPages.size() )
            maxNumberOfObjects = lstPages.size();

        it    = lstPages.begin();
        least = 0;
        max   = 0;
        while( it != lstPages.end() ) 
        {
            value = (*pXRef)[0].vecOffsets[ (*it).ObjectNumber() ].lOffset;

            if( !least || least > value )
                least = value;

            if( !max || max < value )
            {
                max    = value;
                maxRef = *it;
            }

            ++it;
        }

        max += this->GetObject()->GetParent()->GetObject( maxRef )->GetObjectLength();

        vecPages[i].nPageLength     = max - least;

        if( !header.nLeastPageLength || header.nLeastPageLength > vecPages[i].nPageLength )
            header.nLeastPageLength = vecPages[i].nPageLength;

        if( !maxPageLength || maxPageLength < max )
            maxPageLength = max;

        vecPages[i].nSharedObjects  = 0;
        vecPages[i].nContentsOffset = 0;
        vecPages[i].nContentsLength = 0;
    }

    header.nFirstPageObject              = (*pXRef)[0].vecOffsets[ m_pPagesTree->GetPage( 0 )->GetObject()->Reference().ObjectNumber() ].lOffset;
    header.nBitsPageObject               = (pdf_uint16)ceil( logb( static_cast<double>(maxNumberOfObjects-header.nLeastNumberOfObjects) ) );
    header.nBitsPageLength               = (pdf_uint16)ceil( logb( static_cast<double>(maxPageLength - header.nLeastPageLength) ) );
    header.nOffsetContentStream          = 0; // acrobat sets this to 0 and ignores it
    header.nBitsContentStream            = 0; // acrobat sets this to 0 and ignores it
    header.nLeastContentStreamLength     = 0; // acrobat sets this to 0 and ignores it
    header.nBitsLeastContentStreamLength = 0; // acrobat sets this to 0 and ignores it
    header.nBitsNumSharedObjects         = 0;
    header.nBitsGreatestSharedObject     = 0;
    header.nItem12                       = 0;
    header.nItem13                       = 0;

    for( i=0;i<nPageCount;i++ )
    {
        vecPages[i].nObjectsPerPage -= header.nLeastNumberOfObjects;
        vecPages[i].nPageLength     -= header.nLeastPageLength;
        vecPages[i].Write( this );
    }

    header.Write( this );
}

void PdfHintStream::CreateSharedObjectHintTable()
{
    PdfVariant offset( static_cast<long>(this->GetObject()->GetStream()->GetLength()) );
    offset.SetPaddingLength( LINEARIZATION_PADDING );

    this->GetObject()->GetDictionary().AddKey( "S", offset ); // shared object hint table


}
*/

void PdfHintStream::WriteUInt16( pdf_uint16 val )
{
    val = ::PoDoFo::compat::podofo_htons(val);
    this->GetObject()->GetStream()->Append( reinterpret_cast<char*>(&val), 2 );
}

void PdfHintStream::WriteUInt32( pdf_uint32 val )
{
    val = ::PoDoFo::compat::podofo_htonl(val);
    this->GetObject()->GetStream()->Append( reinterpret_cast<char*>(&val), 4 );
}

}; // end namespace PoDoFo::NonPublic
}; // end namespace PoDoFo
