/***************************************************************************
 *   Copyright (C) 2007 by Dominik Seichter                                *
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

#include "FilterTest.h"

#include <cppunit/Asserter.h>

#include <stdlib.h>

using namespace PoDoFo;

CPPUNIT_TEST_SUITE_REGISTRATION( FilterTest );

static const char s_pTestBuffer1[]  = "Man is distinguished, not only by his reason, but by this singular passion from other animals, which is a lust of the mind, that by a perseverance of delight in the continued and indefatigable generation of knowledge, exceeds the short vehemence of any carnal pleasure.";

// We treat the buffer as _excluding_ the trailing \0
static const long s_lTestLength1 = strlen(s_pTestBuffer1);

const char s_pTestBuffer2[]  = { 
    0x01, 0x64, 0x65, static_cast<char>(0xFE), 0x6B, static_cast<char>(0x80), 0x45, 0x32, static_cast<char>(0x88), 0x12, static_cast<char>(0x71), static_cast<char>(0xEA), 0x01,
    0x01, 0x64, 0x65, static_cast<char>(0xFE), 0x6B, static_cast<char>(0x80), 0x45, 0x32, static_cast<char>(0x88), 0x12, static_cast<char>(0x71), static_cast<char>(0xEA), 0x03,
    0x01, 0x64, 0x65, static_cast<char>(0xFE), 0x6B, static_cast<char>(0x80), 0x45, 0x32, static_cast<char>(0x88), 0x12, static_cast<char>(0x71), static_cast<char>(0xEA), 0x02,
    0x01, 0x64, 0x65, static_cast<char>(0xFE), 0x6B, static_cast<char>(0x80), 0x45, 0x32, static_cast<char>(0x88), 0x12, static_cast<char>(0x71), static_cast<char>(0xEA), 0x00,
    0x01, 0x64, 0x65, static_cast<char>(0xFE), 0x6B, static_cast<char>(0x80), 0x45, 0x32, static_cast<char>(0x88), 0x12, static_cast<char>(0x71), static_cast<char>(0xEA), 0x00,
    0x00, 0x00, 0x00, 0x00, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

const long s_lTestLength2 = 6*13;

void FilterTest::setUp()
{
}

void FilterTest::tearDown()
{
}

void FilterTest::TestFilter( EPdfFilter eFilter, const char * pTestBuffer, const long lTestLength )
{
    char*      pEncoded;
    char*      pDecoded;
    pdf_long   lEncoded;
    pdf_long   lDecoded;
   
    std::auto_ptr<PdfFilter> pFilter = PdfFilterFactory::Create( eFilter );
    if( !pFilter.get() )
    {
        printf("!!! Filter %i not implemented.\n", eFilter);
        return;
    }

    printf("Testing Algorithm %i:\n", eFilter);
    printf("\t-> Testing Encoding\n");
    try {
        pFilter->Encode( pTestBuffer, lTestLength, &pEncoded, &lEncoded );
    } catch( PdfError & e ) {
        if( e == ePdfError_UnsupportedFilter ) 
        {
            printf("\t-> Encoding not supported for filter %i.\n", eFilter );
            return;
        }
        else
        {
            e.AddToCallstack( __FILE__, __LINE__ );
            throw e;
        }
    }

    printf("\t-> Testing Decoding\n");
    try {
        pFilter->Decode( pEncoded, lEncoded, &pDecoded, &lDecoded );
    } catch( PdfError & e ) {
        if( e == ePdfError_UnsupportedFilter ) 
        {
            printf("\t-> Decoding not supported for filter %i.\n", eFilter);
            return;
        }
        else
        {
            e.AddToCallstack( __FILE__, __LINE__ );
            throw e;
        }
    }

    printf("\t-> Original Data Length: %li\n", lTestLength );
    printf("\t-> Encoded  Data Length: %li\n", lEncoded );
    printf("\t-> Decoded  Data Length: %li\n", lDecoded );

    CPPUNIT_ASSERT_EQUAL( static_cast<long>(lTestLength), static_cast<long>(lDecoded) );
    CPPUNIT_ASSERT_EQUAL( memcmp( pTestBuffer, pDecoded, lTestLength ), 0 );

    free( pEncoded );
    free( pDecoded );

    printf("\t-> Test succeeded!\n");
}

void FilterTest::testFilters()
{
    for( int i =0; i<=ePdfFilter_Crypt; i++ )
    {
        TestFilter( static_cast<EPdfFilter>(i), s_pTestBuffer1, s_lTestLength1 );
        TestFilter( static_cast<EPdfFilter>(i), s_pTestBuffer2, s_lTestLength2 );
    }
}

void FilterTest::testCCITT()
{
    std::auto_ptr<PdfFilter> pFilter = PdfFilterFactory::Create( ePdfFilter_CCITTFaxDecode );
    if( !pFilter.get() )
    {
        printf("!!! ePdfFilter_CCITTFaxDecode not implemented skipping test!\n");
        return;
    }


}
