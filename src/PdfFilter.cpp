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
 ***************************************************************************/

#include "PdfDefines.h"
#include "PdfFilter.h"

#include "PdfParserBase.h"
#include "PdfDictionary.h"

#include <zlib.h>
#define CHUNK       16384

namespace PoDoFo {

/** 
 * This structur contains all necessary values
 * for a FlateDecode and LZWDecode Predictor.
 * These values are normally stored in the /DecodeParams
 * key of a PDF dictionary.
 */
struct TFlatePredictorParams {
    TFlatePredictorParams() {
        nPredictor   = 1;
        nColors      = 1;
        nBPC         = 8;
        nColumns     = 1;
        nEarlyChange = 1;
    };

    int nPredictor;
    int nColors;
    int nBPC;
    int nColumns;
    int nEarlyChange;
};


std::map<EPdfFilter,PdfFilter*> PdfFilterFactory::s_mapFilters;

const PdfFilter* PdfFilterFactory::Create( const EPdfFilter eFilter ) 
{
    PdfFilter* pFilter = s_mapFilters[eFilter];

    if( !pFilter ) 
    {
        switch( eFilter ) 
        {
            case ePdfFilter_ASCIIHexDecode:
                pFilter = new PdfHexFilter();
                break;
                
            case ePdfFilter_ASCII85Decode:
                pFilter = new PdfAscii85Filter();
                break;
                
            case ePdfFilter_LZWDecode:
                break;
                
            case ePdfFilter_FlateDecode:
                pFilter = new PdfFlateFilter();
                break;
                
            case ePdfFilter_RunLengthDecode:
                pFilter = new PdfRLEFilter();
                break;
                
            case ePdfFilter_CCITTFaxDecode:
            case ePdfFilter_JBIG2Decode:
            case ePdfFilter_DCTDecode:
            case ePdfFilter_JPXDecode:
            case ePdfFilter_Crypt:
            default:
                break;
        }

        if( pFilter )
            s_mapFilters[eFilter] = pFilter;
    }

    return pFilter;
}

// -------------------------------------------------------
// Hex
// -------------------------------------------------------
void PdfHexFilter::Encode( const char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen ) const
{
    char*    pStart;
    int      i      = 0;

    if( !plOutLen || !pInBuffer || !ppOutBuffer )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    *plOutLen = (lInLen << 1);
    *ppOutBuffer = (char*)malloc( *plOutLen * sizeof(char) );
    if( !*ppOutBuffer )
    {
        RAISE_ERROR( ePdfError_OutOfMemory );
    }

    pStart = *ppOutBuffer;
    while( i < lInLen )
    {
        *pStart  = (pInBuffer[i] & 0xF0) >> 4;
        *pStart += (*pStart > 9 ? 'A' - 10 : '0');

        ++pStart;

        *pStart  = (pInBuffer[i] & 0x0F);
        *pStart += (*pStart > 9 ? 'A' - 10 : '0');

        ++pStart;
        ++i;
    }
}

void PdfHexFilter::Decode( const char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen, const PdfDictionary* pDecodeParms ) const
{
    PdfError eCode;
    int      i      = 0;
    char*    pStart;
    char     hi, low;

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

}

// -------------------------------------------------------
// Ascii 85
// 
// based on public domain software from:
// Paul Haahr - http://www.webcom.com/~haahr/
// -------------------------------------------------------

/* This will be optimized by the compiler */
unsigned long PdfAscii85Filter::sPowers85[] = {
    85*85*85*85, 85*85*85, 85*85, 85, 1
};

void PdfAscii85Filter::Encode( const char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen ) const
{
    int           count = 0;
    unsigned long tuple = 0;
    int           pos   = 0;
    unsigned int  c;

    if( !plOutLen || !pInBuffer || !ppOutBuffer )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    *plOutLen = (int)(lInLen/4) * 6;
    *ppOutBuffer = (char*)malloc( *plOutLen * sizeof(char) );
   
    if( !*ppOutBuffer )
    {
        RAISE_ERROR( ePdfError_OutOfMemory );
    }

    while( lInLen ) 
    {
        c = *pInBuffer & 0xff;
	switch (count++) {
            case 0: tuple |= ( c << 24); break;
            case 1: tuple |= ( c << 16); break;
            case 2: tuple |= ( c <<  8); break;
            case 3:
		tuple |= c;
		if( 0 == tuple ) 
                {
                    if( pos >= *plOutLen )
                    {
                        RAISE_ERROR( ePdfError_OutOfMemory );
                    }
                    (*ppOutBuffer)[pos++] = 'z';
		} 
                else
                {
                    this->Encode( *ppOutBuffer, &pos, *plOutLen, tuple, count ); 
                }

		tuple = 0;
		count = 0;
		break;
	}
        --lInLen;
        ++pInBuffer;
    }

    if (count > 0)
    {
        this->Encode( *ppOutBuffer, &pos, *plOutLen, tuple, count );
    }

    *plOutLen = pos;
}

void PdfAscii85Filter::Encode( char* pBuffer, int* bufferPos, long lBufferLen, unsigned long tuple, int count ) const
{
    int      i      = 5;
    char     buf[5];
    char*    start  = buf;;

    do 
    {
        *start++ = tuple % 85;
        tuple /= 85;
    } 
    while (--i > 0);
    
    i = count;
    do 
    {
        if( *bufferPos >= lBufferLen )
        {
            RAISE_ERROR( ePdfError_OutOfMemory );
        }
        pBuffer[(*bufferPos)++] = (unsigned char)(*--start) + '!';
    } 
    while (i-- > 0);
}

void PdfAscii85Filter::Decode( const char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen, const PdfDictionary* pDecodeParms ) const
{
    unsigned long tuple = 0;
    int           count = 0;
    int           pos   = 0;

    if( !plOutLen || !pInBuffer || !ppOutBuffer )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    *plOutLen    = lInLen;
    *ppOutBuffer = (char*)malloc( *plOutLen * sizeof(char) );
   
    if( !*ppOutBuffer )
    {
        RAISE_ERROR( ePdfError_OutOfMemory );
    }

    while( lInLen ) 
    {
        switch ( *pInBuffer ) 
        {
            default:
                if ( *pInBuffer < '!' || *pInBuffer > 'u') 
                {
                    RAISE_ERROR( ePdfError_ValueOutOfRange );
                }
                
                tuple += ( *pInBuffer - '!') * PdfAscii85Filter::sPowers85[count++];
                if (count == 5) 
                {
                    WidePut( *ppOutBuffer, &pos, *plOutLen, tuple, 4 );
                    count = 0;
                    tuple = 0;
                }
                break;
            case 'z':
                if (count != 0 ) 
                {
                    RAISE_ERROR( ePdfError_ValueOutOfRange );
                }

                if( pos + 4 >= *plOutLen )
                {
                    RAISE_ERROR( ePdfError_OutOfMemory );
                }

                (*ppOutBuffer)[ pos++ ] = 0;
                (*ppOutBuffer)[ pos++ ] = 0;
                (*ppOutBuffer)[ pos++ ] = 0;
                (*ppOutBuffer)[ pos++ ] = 0;
                break;
            case '\n': case '\r': case '\t': case ' ':
            case '\0': case '\f': case '\b': case 0177:
                break;
        }

        --lInLen;
        ++pInBuffer;
    }

    if (count > 0) 
    {
        count--;
        tuple += PdfAscii85Filter::sPowers85[count];
        WidePut( *ppOutBuffer, &pos, *plOutLen, tuple, count );
    }

    *plOutLen = pos;
}

void PdfAscii85Filter::WidePut( char* pBuffer, int* bufferPos, long lBufferLen, unsigned long tuple, int bytes ) const
{
    if( *bufferPos + bytes >= lBufferLen ) 
    {
        RAISE_ERROR( ePdfError_OutOfMemory );
    }

    switch (bytes) {
	case 4:
            pBuffer[ (*bufferPos)++ ] = (char)(tuple >> 24);
            pBuffer[ (*bufferPos)++ ] = (char)(tuple >> 16);
            pBuffer[ (*bufferPos)++ ] = (char)(tuple >>  8);
            pBuffer[ (*bufferPos)++ ] = (char)(tuple);
            break;
	case 3:
            pBuffer[ (*bufferPos)++ ] = (char)(tuple >> 24);
            pBuffer[ (*bufferPos)++ ] = (char)(tuple >> 16);
            pBuffer[ (*bufferPos)++ ] = (char)(tuple >>  8);
            break;
	case 2:
            pBuffer[ (*bufferPos)++ ] = (char)(tuple >> 24);
            pBuffer[ (*bufferPos)++ ] = (char)(tuple >> 16);
            break;
	case 1:
            pBuffer[ (*bufferPos)++ ] = (char)(tuple >> 24);
            break;
    }
}

// -------------------------------------------------------
// Flate
// -------------------------------------------------------
void PdfFlateFilter::Encode( const char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen ) const
{
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

#ifndef deflateBound
    lBufLen = lInLen + 1024; /* 1024=emergency extra space */
#else
    lBufLen = deflateBound( &d_stream, lInLen );
#endif
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
}

void PdfFlateFilter::Decode( const char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen, const PdfDictionary* pDecodeParms ) const
{
    int          flateErr;
    unsigned int have;
    z_stream strm;
    char  out[CHUNK];
    char* pBuf = NULL;
    char* pTmp = NULL;

    long  lBufSize = 0;
    TFlatePredictorParams tParams;

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
            {
                PdfError::LogMessage( eLogSeverity_Error, "Flate Decoding Error from ZLib: %i\n", flateErr );
                (void)inflateEnd(&strm);

                RAISE_ERROR( ePdfError_Flate );
            }
            default:
                break;
        }

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

    if( pDecodeParms ) 
    {
        tParams.nPredictor   = pDecodeParms->GetKeyAsLong( "Predictor", tParams.nPredictor );
        tParams.nColors      = pDecodeParms->GetKeyAsLong( "Colors", tParams.nColors );
        tParams.nBPC         = pDecodeParms->GetKeyAsLong( "BitsPerComponent", tParams.nBPC );
        tParams.nColumns     = pDecodeParms->GetKeyAsLong( "Columns", tParams.nColumns );
        tParams.nEarlyChange = pDecodeParms->GetKeyAsLong( "EarlyChange", tParams.nEarlyChange );

        try {
            this->RevertPredictor( &tParams, pBuf, lBufSize, ppOutBuffer, plOutLen );
        } catch( PdfError & e ) {
            free( pBuf );
            e.AddToCallstack( __FILE__, __LINE__ );
            throw e;
        }

        free( pBuf );
    }
}

// -------------------------------------------------------
// Flate Predictor
// -------------------------------------------------------
void PdfFlateFilter::RevertPredictor( const TFlatePredictorParams* pParams, const char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen ) const
{
    unsigned char*   pPrev;
    int     nRows;
    int     i;
    char*   pOutBufStart;
    const char*   pBuffer = pInBuffer;
    int     nPredictor;

#ifdef _DEBUG
    PdfError::DebugMessage("Applying Predictor %i to buffer of size %i\n", pParams->nPredictor, lInLen );
    PdfError::DebugMessage("Cols: %i Modulo: %i Comps: %i\n", pParams->nColumns, lInLen % (pParams->nColumns +1), pParams->nBPC );
#endif // _DEBUG

    if( pParams->nPredictor == 1 )  // No Predictor
        return;

    nRows = (pParams->nColumns * pParams->nBPC) >> 3; 

    pPrev = (unsigned char*)malloc( sizeof(char) * nRows );
    if( !pPrev )
    {
        RAISE_ERROR( ePdfError_OutOfMemory );
    }

    memset( pPrev, 0, sizeof(char) * nRows );

#ifdef _DEBUG
    PdfError::DebugMessage("Alloc: %i\n", (lInLen / (pParams->nColumns + 1)) * pParams->nColumns );
#endif // _DEBUG

    *ppOutBuffer = (char*)malloc( sizeof(char) * (lInLen / (pParams->nColumns + 1)) * pParams->nColumns );
    pOutBufStart = *ppOutBuffer;

    if( !*ppOutBuffer )
    {
        free( pPrev );
        RAISE_ERROR( ePdfError_OutOfMemory );
    }

    while( pBuffer < (pInBuffer + lInLen) )
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
                {
                    free( pPrev );
                    RAISE_ERROR( ePdfError_InvalidPredictor );
                    break;
                }
            }
  
            pPrev[i] = *pOutBufStart;          
            ++pOutBufStart;
            ++pBuffer;
        }
    }

    *plOutLen = (pOutBufStart - *ppOutBuffer);

    free( pPrev );
}

// -------------------------------------------------------
// RLE
// -------------------------------------------------------
void PdfRLEFilter::Encode( const char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen ) const
{
    RAISE_ERROR( ePdfError_UnsupportedFilter );
}

void PdfRLEFilter::Decode( const char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen, const PdfDictionary* pDecodeParms ) const
{
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
}

};
