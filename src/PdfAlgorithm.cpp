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

#include "PdfDefines.h"
#include "PdfAlgorithm.h"

#include "PdfParserBase.h"

#include <zlib.h>

#define CHUNK       16384

namespace PoDoFo {

PdfError PdfAlgorithm::FlateDecodeBuffer( char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen )
{
    PdfError  eCode;

    int          flateErr;
    unsigned int have;
    z_stream strm;
    char  out[CHUNK];
    char* pBuf = NULL;
    char* pTmp = NULL;

    long  lBufSize = 0;

    if( !pInBuffer || !plOutLen || !ppOutBuffer )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;

    if( inflateInit(&strm) != Z_OK)
    {
        RAISE_ERROR( ePdfError_Flate );
    }

    strm.avail_in = lInLen;
    strm.next_in  = (Bytef*)pInBuffer;

    do {
        strm.avail_out = CHUNK;
        strm.next_out  = (Bytef*)out;


        switch ( (flateErr = inflate(&strm, Z_NO_FLUSH)) ) {
            case Z_NEED_DICT:
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                PdfError::LogMessage( eLogSeverity_Error, "Flate Decoding Error from ZLib: %i", flateErr );
                eCode.SetError( ePdfError_Flate );     /* and fall through */
                (void)inflateEnd(&strm);
                return eCode;
            default:
                break;
        }

        if( eCode.IsError() )
            eCode = ePdfError_ErrOk;

        have = CHUNK - strm.avail_out;

        if( pBuf )
            pBuf = (char*)realloc( pBuf, sizeof( char ) * (lBufSize + have) );
        else
            pBuf = (char*)malloc( sizeof( char ) * (lBufSize + have) );

        if( !pBuf )
        {
            (void)inflateEnd(&strm);
            free( pTmp );
            RAISE_ERROR( ePdfError_InvalidHandle );
        }

        memcpy( (pBuf+lBufSize), out, have );
        lBufSize += have;
        free( pTmp );
    } while (strm.avail_out == 0);
    
    /* clean up and return */
    (void)inflateEnd(&strm);

    *ppOutBuffer = pBuf;
    *plOutLen    = lBufSize;

    return eCode;
}

PdfError PdfAlgorithm::FlateEncodeBuffer( char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen )
{
    PdfError eCode;

    z_stream d_stream; 
    char*    buf;
    long     lBufLen;

    if( !pInBuffer || !plOutLen || !ppOutBuffer )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    d_stream.zalloc   = Z_NULL;
    d_stream.zfree    = Z_NULL;
    d_stream.opaque   = Z_NULL;
    d_stream.avail_in = lInLen;
    d_stream.next_in  = (Bytef*)pInBuffer;
    
    if( deflateInit( &d_stream, Z_DEFAULT_COMPRESSION ) )
    {
        RAISE_ERROR( ePdfError_Flate );
    }

    lBufLen = deflateBound( &d_stream, lInLen );
    buf = (char*)malloc( sizeof( char ) * lBufLen );
    if( !buf )
    {
        RAISE_ERROR( ePdfError_OutOfMemory );
    }

    d_stream.avail_out = lBufLen;
    d_stream.next_out  = (Bytef*)buf;
    
    if( deflate( &d_stream, Z_FINISH ) != Z_STREAM_END )
    {
        RAISE_ERROR( ePdfError_Flate );
    }

    *plOutLen = lBufLen - d_stream.avail_out;
    *ppOutBuffer = buf;

    (void)deflateEnd(&d_stream);

    return eCode;
}

PdfError PdfAlgorithm::HexDecodeBuffer( const char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen )
{
    PdfError eCode;
    int     i      = 0;
    char*   pStart;
    char    hi, low;

    if( !plOutLen || !pInBuffer || !ppOutBuffer )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    *ppOutBuffer = (char*)malloc( sizeof(char) * (lInLen >> 1) );
    pStart       = *ppOutBuffer;

    if( !pStart )
    {
        RAISE_ERROR( ePdfError_OutOfMemory );
    }

    while( i < lInLen )
    {
        while( PdfParserBase::IsWhitespace( pInBuffer[i] ) )
            ++i;
        hi  = pInBuffer[i++];

        while( PdfParserBase::IsWhitespace( pInBuffer[i] ) )
            ++i;
        low = pInBuffer[i++];

        hi  -= ( hi  < 'A' ? '0' : 'A'-10 );
        low -= ( low < 'A' ? '0' : 'A'-10 );

        *pStart = (hi << 4) | (low & 0x0F);
        ++pStart;
    }

    *plOutLen = (pStart - *ppOutBuffer);

    return eCode;
}

PdfError PdfAlgorithm::HexEncodeBuffer( const char* pInBuffer, long lInLen, char** ppOutBuffer, long *plOutLen )
{
    PdfError eCode;
    char*    pData;
    char*    pStart;
    int      i      = 0;

    if( !plOutLen || !pInBuffer || !ppOutBuffer )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    *plOutLen = (lInLen << 1);
    pData = (char*)malloc( *plOutLen * sizeof(char) );
    if( !pData )
    {
        RAISE_ERROR( ePdfError_OutOfMemory );
    }

    pStart = pData;
    while( i < lInLen )
    {
        *pData  = (pInBuffer[i] & 0xF0) >> 4;
        *pData += (*pData > 9 ? 'A' - 10 : '0');

        ++pData;

        *pData  = (pInBuffer[i] & 0x0F);
        *pData += (*pData > 9 ? 'A' - 10 : '0');

        ++pData;

        ++i;
    }

    *ppOutBuffer = pStart;

    return eCode;
}

PdfError PdfAlgorithm::RevertFlateDecodePredictor( const TFlatePredictorParams* pParams, char* pInBuffer, long lInLen, char** ppOutBuffer, long *plOutLen )
{
    PdfError eCode;
    unsigned char*   pPrev;
    int     nRows;
    int     i;
    char*   pOutBufStart;
    char*   pBuffer = pInBuffer;
    int     nPredictor;

    printf("Applying Predictor %i to buffer of size %i\n", pParams->nPredictor, lInLen );
    printf("Cols: %i Modulo: %i Comps: %i\n", pParams->nColumns, lInLen % (pParams->nColumns +1), pParams->nBPC );

    if( pParams->nPredictor == 1 )  // No Predictor
        return ePdfError_ErrOk;

    nRows = (pParams->nColumns * pParams->nBPC) >> 3; 
    printf("nRows=%i\n", nRows );
    printf("nBPC=%i\n", pParams->nBPC );

    pPrev = (unsigned char*)malloc( sizeof(char) * nRows );
    if( !pPrev )
    {
        RAISE_ERROR( ePdfError_OutOfMemory );
    }

    memset( pPrev, 0, sizeof(char) * nRows );

    printf("Alloc: %i\n", (lInLen / (pParams->nColumns + 1)) * pParams->nColumns );
    *ppOutBuffer = (char*)malloc( sizeof(char) * (lInLen / (pParams->nColumns + 1)) * pParams->nColumns );
    pOutBufStart = *ppOutBuffer;

    if( !*ppOutBuffer )
    {
        free( pPrev );
        RAISE_ERROR( ePdfError_OutOfMemory );
    }

    while( pBuffer < (pInBuffer + lInLen) && eCode == ePdfError_ErrOk )
    {
        nPredictor = pParams->nPredictor >= 10 ? *pBuffer + 10 : *pBuffer;
        ++pBuffer;

        for( i=0;i<nRows;i++ )
        {
            switch( nPredictor )
            {
                case 2: // Tiff Predictor
                    // TODO: implement tiff predictor
                    
                    break;
                case 10: // png none
                case 11: // png sub
                case 12: // png up
                    *pOutBufStart = (unsigned char)(pPrev[i] + (unsigned char)*pBuffer);
                    break;
                case 13: // png average
                case 14: // png paeth
                case 15: // png optimum
                    break;
                
                default:
                    eCode.SetError( ePdfError_InvalidPredictor );
                    break;
            }
  
            pPrev[i] = *pOutBufStart;          
            ++pOutBufStart;
            ++pBuffer;
        }
    }

    printf("pOutBufStart=%p\n", pOutBufStart );
    printf("pOutBuffer=%p\n", *ppOutBuffer );
    *plOutLen = (pOutBufStart - *ppOutBuffer);
    printf("Size of new buffer: %i ecode=%i\n", *plOutLen, eCode.Error() );
    free( pPrev );

    return eCode;
}

PdfError PdfAlgorithm::RunLengthDecodeBuffer( char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen )
{
    PdfError              eCode;
    char*                 pBuf;
    long                  lCur;
    long                  lSize;
    unsigned char         cLen;
    int                   i;

    if( !plOutLen || !pInBuffer || !ppOutBuffer )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    lCur  = 0;
    lSize = lInLen;
    pBuf  = (char*)malloc( sizeof(char)*lSize );
    if( !pBuf )
    {
        RAISE_ERROR( ePdfError_OutOfMemory );
    }

    while( lInLen )
    {
        cLen = *pInBuffer;
        ++pInBuffer;

        if( cLen == 128 )
            // reached EOD
            break;
        else if( cLen <= 127 )
        {
            if( lCur + cLen+1 > lSize )
            {
                // buffer to small, do a realloc
                lSize = PDF_MAX( lCur + cLen+1, lSize << 1 );
                pBuf  = (char*)realloc( pBuf, lSize  );
                if( !pBuf )
                {
                    RAISE_ERROR( ePdfError_OutOfMemory );
                }
            }
                
            memcpy( pBuf + lCur, pInBuffer, cLen+1 );
            lCur      += (cLen + 1);
            pInBuffer += (cLen + 1);
            lInLen    -= (cLen + 1);
        }
        else if( cLen >= 129 )
        {
            cLen = 257 - cLen;

            if( lCur + cLen > lSize )
            {
                // buffer to small, do a realloc
                lSize = PDF_MAX( lCur + cLen, lSize << 1 );
                pBuf  = (char*)realloc( pBuf, lSize  );
                if( !pBuf )
                {
                    RAISE_ERROR( ePdfError_OutOfMemory );
                }
            }

            for( i=0;i<cLen;i++ )
            {
                *(pBuf + lCur) = *pInBuffer;
                ++lCur;
            }

            ++pInBuffer;
            --lInLen;
        }
    }

    *ppOutBuffer = pBuf;
    *plOutLen    = lCur;

    return eCode;
}

};

