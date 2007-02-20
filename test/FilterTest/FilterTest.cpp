/***************************************************************************
 *   Copyright (C) 2006 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "PdfFilter.h"
#include "PdfOutputStream.h"

#include "../PdfTest.h"

using namespace PoDoFo;

namespace {

const char pTestBuffer1[]  = "Man is distinguished, not only by his reason, but by this singular passion from other animals, which is a lust of the mind, that by a perseverance of delight in the continued and indefatigable generation of knowledge, exceeds the short vehemence of any carnal pleasure.";

// We treat the buffer as _excluding_ the trailing \0
const long lTestLength1 = strlen(pTestBuffer1);

const char pTestBuffer2[]  = { 
    0x01, 0x64, 0x65, 0xFE, 0x6B, 0x80, 0x45, 0x32, 0x88, 0x12, 0x71, 0xEA, 0x01,
    0x01, 0x64, 0x65, 0xFE, 0x6B, 0x80, 0x45, 0x32, 0x88, 0x12, 0x71, 0xEA, 0x03,
    0x01, 0x64, 0x65, 0xFE, 0x6B, 0x80, 0x45, 0x32, 0x88, 0x12, 0x71, 0xEA, 0x02,
    0x01, 0x64, 0x65, 0xFE, 0x6B, 0x80, 0x45, 0x32, 0x88, 0x12, 0x71, 0xEA, 0x00,
    0x01, 0x64, 0x65, 0xFE, 0x6B, 0x80, 0x45, 0x32, 0x88, 0x12, 0x71, 0xEA, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

const long lTestLength2 = 6*13;

void test_filter( EPdfFilter eFilter, const char * pTestBuffer, const long lTestLength )
{
    char*      pEncoded;
    char*      pDecoded;
    long       lEncoded;
    long       lDecoded;
   
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

    if( static_cast<long>(lTestLength) != lDecoded ) 
    {
        fprintf( stderr, "Error: Decoded Length != Original Length\n");

        /*
        fprintf( stderr, "Data:\n%s\n", pEncoded );

        fprintf( stderr, "DecodedData:\n%s\n", pDecoded );
        */

        PODOFO_RAISE_ERROR( ePdfError_TestFailed );
    }

    if( memcmp( pTestBuffer, pDecoded, lTestLength ) != 0 )
    {
        printf("\t-> Original Data: <%s>\n", pTestBuffer );
        printf("\t-> Encoded  Data: <%s>\n", pEncoded );
        printf("\t-> Decoded  Data: <%s>\n", pDecoded );

        fprintf( stderr, "Error: Decoded Data does not match original data.\n");
        PODOFO_RAISE_ERROR( ePdfError_TestFailed );
    }

    printf("\t-> Test succeeded!\n");
}

void test_filter_queque( const char* pBuffer, long lLen )
{
    char* pEncoded;
    long  lEncoded;

    char* pDecoded;
    long  lDecoded;

    TVecFilters filters;
    filters.push_back( ePdfFilter_ASCIIHexDecode );
    filters.push_back( ePdfFilter_ASCII85Decode );
    filters.push_back( ePdfFilter_FlateDecode );

    printf("Testing queque of filters:\n");
    printf("\tePdfFilter_ASCIIHexDecode\n");
    printf("\tePdfFilter_ASCII85Decode\n");
    printf("\tePdfFilter_FlateDecode\n");

    PdfMemoryOutputStream  stream;
    PdfOutputStream*       pEncode = PdfFilterFactory::CreateEncodeStream( filters, &stream );
    
    pEncode->Write( pBuffer, lLen );

    delete pEncode;

    lEncoded = stream.GetLength();
    pEncoded = stream.TakeBuffer();

    PdfMemoryOutputStream stream2;
    PdfOutputStream* pDecode = PdfFilterFactory::CreateDecodeStream( filters, &stream2 );
    
    pDecode->Write( pEncoded, lEncoded );

    delete pDecode;

    lDecoded = stream2.GetLength();
    pDecoded = stream2.TakeBuffer();

    printf("\t-> Original Data Length: %li\n", lLen );
    printf("\t-> Encoded  Data Length: %li\n", lEncoded );
    printf("\t-> Decoded  Data Length: %li\n", lDecoded );

    if( lDecoded != lLen ) 
    {
        fprintf( stderr, "Error: Decoded data length does not match original data length.\n");
        PODOFO_RAISE_ERROR( ePdfError_TestFailed );
    }

    if( memcmp( pBuffer, pDecoded, lLen ) != 0 )
    {
        printf("\t-> Original Data: <%s>\n", pBuffer );
        printf("\t-> Encoded  Data: \n<%s>\n", pEncoded );
        printf("\t-> Decoded  Data: \n<%s>\n", pDecoded );

        fprintf( stderr, "Error: Decoded Data does not match original data.\n");
        PODOFO_RAISE_ERROR( ePdfError_TestFailed );
    }

    free( pDecoded );
    free( pEncoded );

}

} // end anon namespace

int main() 
{
    printf("This test tests all filters of PoDoFo\n");
    printf("---\n");

    printf("ePdfFilter_ASCIIHexDecode     = 0\n");
    printf("ePdfFilter_ASCII85Decode      = 1\n");
    printf("ePdfFilter_LZWDecode          = 2\n");
    printf("ePdfFilter_FlateDecode        = 3\n");
    printf("ePdfFilter_RunLengthDecode    = 4\n");
    printf("ePdfFilter_CCITTFaxDecode     = 5\n");
    printf("ePdfFilter_JBIG2Decode        = 6\n");
    printf("ePdfFilter_DCTDecode          = 7\n");
    printf("ePdfFilter_JPXDecode          = 8\n");
    printf("ePdfFilter_Crypt              = 9\n");

    try {
        for( int i =0; i<=ePdfFilter_Crypt; i++ )
        {
            test_filter( static_cast<EPdfFilter>(i), pTestBuffer1, lTestLength1 );
            test_filter( static_cast<EPdfFilter>(i), pTestBuffer2, lTestLength2 );

            test_filter_queque( pTestBuffer1, lTestLength1 );
            test_filter_queque( pTestBuffer2, lTestLength2 );
        }
    } catch( PdfError & e ) {
        e.PrintErrorMsg();
        return e.GetError();
    }

    printf("All tests sucessfull!\n");
    return 0;
}
