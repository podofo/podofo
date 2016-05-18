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

#include "PdfDefines.h"
#include "PdfFiltersPrivate.h"

#include "PdfDictionary.h"
#include "PdfOutputDevice.h"
#include "PdfOutputStream.h"
#include "PdfTokenizer.h"
#include "PdfDefinesPrivate.h"

#ifdef PODOFO_HAVE_JPEG_LIB
extern "C" {
#include "jerror.h"
}
#endif // PODOFO_HAVE_JPEG_LIB

#include <stdlib.h>
#include <string.h>

#ifdef PODOFO_HAVE_TIFF_LIB
extern "C" {
#ifdef _WIN32		// For O_RDONLY
    // TODO: DS
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif
}
#endif // PODOFO_HAVE_TIFF_LIB


#define LZW_TABLE_SIZE      4096

namespace {

// Private data for PdfAscii85Filter. This will be optimised
// by the compiler through compile-time constant expression
// evaluation.
const unsigned long sPowers85[] = {
    85*85*85*85, 85*85*85, 85*85, 85, 1
};

} // end anonymous namespace


namespace PoDoFo {

/** 
 * This structur contains all necessary values
 * for a FlateDecode and LZWDecode Predictor.
 * These values are normally stored in the /DecodeParams
 * key of a PDF dictionary.
 */
class PdfPredictorDecoder {

public:
    PdfPredictorDecoder( const PdfDictionary* pDecodeParms ) {
        m_nPredictor   = static_cast<int>(pDecodeParms->GetKeyAsLong( "Predictor", 1L ));
        m_nColors      = static_cast<int>(pDecodeParms->GetKeyAsLong( "Colors", 1L ));
        m_nBPC         = static_cast<int>(pDecodeParms->GetKeyAsLong( "BitsPerComponent", 8L ));
        m_nColumns     = static_cast<int>(pDecodeParms->GetKeyAsLong( "Columns", 1L ));
        m_nEarlyChange = static_cast<int>(pDecodeParms->GetKeyAsLong( "EarlyChange", 1L ));

        if( m_nPredictor >= 10)
        {
          m_bNextByteIsPredictor = true;
          m_nCurPredictor = -1;
        }
        else
        {
          m_bNextByteIsPredictor = false;
          m_nCurPredictor = m_nPredictor;
        }

        m_nCurRowIndex  = 0;
        m_nBpp  = (m_nBPC * m_nColors) >> 3;
        m_nRows = (m_nColumns * m_nColors * m_nBPC) >> 3;

        m_pPrev = static_cast<char*>(podofo_calloc( m_nRows, sizeof(char) ));
        if( !m_pPrev )
        {
            PODOFO_RAISE_ERROR( ePdfError_OutOfMemory );
        }

        memset( m_pPrev, 0, sizeof(char) * m_nRows );
    };

    ~PdfPredictorDecoder()
    {
        podofo_free( m_pPrev );
    }

    void Decode( const char* pBuffer, pdf_long lLen, PdfOutputStream* pStream ) 
    {
        if( m_nPredictor == 1 )
        {
            pStream->Write( pBuffer, lLen );
            return;
        }


        while( lLen-- ) 
        {
            if( m_bNextByteIsPredictor )
            {
                m_nCurPredictor = *pBuffer + 10;
                m_bNextByteIsPredictor = false;
            }
            else
            {
                switch( m_nCurPredictor )
                {
                    case 2: // Tiff Predictor
                    {
                        if(m_nBPC == 8)
                        {   // Same as png sub
                            int prev = (m_nCurRowIndex - m_nBpp < 0
                                        ? 0 : m_pPrev[m_nCurRowIndex - m_nBpp]);
                            m_pPrev[m_nCurRowIndex] = *pBuffer + prev;
                            break;
                        }

                        // TODO: implement tiff predictor for other than 8 BPC
                        PODOFO_RAISE_ERROR( ePdfError_InvalidPredictor );
                        break;
                    }
                    case 10: // png none
                    {
                        m_pPrev[m_nCurRowIndex] = *pBuffer;
                        break;
                    }
                    case 11: // png sub
                    {
                        int prev = (m_nCurRowIndex - m_nBpp < 0 
                                    ? 0 : m_pPrev[m_nCurRowIndex - m_nBpp]);
                        m_pPrev[m_nCurRowIndex] = *pBuffer + prev;
                        break;
                    }
                    case 12: // png up
                    {
                        m_pPrev[m_nCurRowIndex] += *pBuffer;
                        break;
                    }
                    case 13: // png average
                    {
                        int prev = (m_nCurRowIndex - m_nBpp < 0 
                                    ? 0 : m_pPrev[m_nCurRowIndex - m_nBpp]);
                        m_pPrev[m_nCurRowIndex] = ((prev + m_pPrev[m_nCurRowIndex]) >> 1) + *pBuffer;
                        break;
                    }
                    case 14: // png paeth
                    case 15: // png optimum
                        PODOFO_RAISE_ERROR( ePdfError_InvalidPredictor );
                        break;
                        
                    default:
                    {
                        //PODOFO_RAISE_ERROR( ePdfError_InvalidPredictor );
                        break;
                    }
                }

                ++m_nCurRowIndex;
            }

            ++pBuffer;

            if( m_nCurRowIndex >= m_nRows ) 
            {   // One line finished
                m_nCurRowIndex  = 0;
                m_bNextByteIsPredictor = (m_nCurPredictor >= 10);
                pStream->Write( m_pPrev, m_nRows );
            }
        }
    }


private:
    int m_nPredictor;
    int m_nColors;
    int m_nBPC; //< Bytes per component
    int m_nColumns;
    int m_nEarlyChange;
    int m_nBpp; ///< Bytes per pixel

    int m_nCurPredictor;
    int m_nCurRowIndex;
    int m_nRows;

    bool m_bNextByteIsPredictor;

    char* m_pPrev;
};


// -------------------------------------------------------
// Hex
// -------------------------------------------------------

void PdfHexFilter::EncodeBlockImpl( const char* pBuffer, pdf_long lLen )
{
    char data[2];
    while( lLen-- )
    {
        data[0]  = (*pBuffer & 0xF0) >> 4;
        data[0] += (data[0] > 9 ? 'A' - 10 : '0');

        data[1]  = (*pBuffer & 0x0F);
        data[1] += (data[1] > 9 ? 'A' - 10 : '0');

        GetStream()->Write( data, 2 );

        ++pBuffer;
    }
}

void PdfHexFilter::BeginDecodeImpl( const PdfDictionary* )
{ 
    m_cDecodedByte = 0;
    m_bLow         = true;
}

void PdfHexFilter::DecodeBlockImpl( const char* pBuffer, pdf_long lLen )
{
    char val;

    while( lLen-- ) 
    {
        if( PdfTokenizer::IsWhitespace( *pBuffer ) )
        {
            ++pBuffer;
            continue;
        }

        val  = PdfTokenizer::GetHexValue( *pBuffer );
        if( m_bLow ) 
        {
            m_cDecodedByte = (val & 0x0F);
            m_bLow         = false;
        }
        else
        {
            m_cDecodedByte = ((m_cDecodedByte << 4) | val);
            m_bLow         = true;

            GetStream()->Write( &m_cDecodedByte, 1 );
        }

        ++pBuffer;
    }
}

void PdfHexFilter::EndDecodeImpl()
{ 
    if( !m_bLow ) 
    {
        // an odd number of bytes was read,
        // so the last byte is 0
        GetStream()->Write( &m_cDecodedByte, 1 );
    }
}


// -------------------------------------------------------
// Ascii 85
// 
// based on public domain software from:
// Paul Haahr - http://www.webcom.com/~haahr/
// -------------------------------------------------------

void PdfAscii85Filter::EncodeTuple( unsigned long tuple, int count )
{
    int      i      = 5;
    int      z      = 0;
    char     buf[5];
    char     out[5];
    char*    start  = buf;;

    do 
    {
        *start++ = static_cast<char>(tuple % 85);
        tuple /= 85;
    } 
    while (--i > 0);

    i = count;
    do 
    {
        out[z++] = static_cast<unsigned char>(*--start) + '!';
    } 
    while (i-- > 0);

    GetStream()->Write( out, z );
}

void PdfAscii85Filter::BeginEncodeImpl()
{
    m_count = 0;
    m_tuple = 0;
}

void PdfAscii85Filter::EncodeBlockImpl( const char* pBuffer, pdf_long lLen )
{
    unsigned int  c;
    const char*   z = "z";

    while( lLen ) 
    {
        c = *pBuffer & 0xff;
        switch (m_count++) {
            case 0: m_tuple |= ( c << 24); break;
            case 1: m_tuple |= ( c << 16); break;
            case 2: m_tuple |= ( c <<  8); break;
            case 3:
                m_tuple |= c;
                if( 0 == m_tuple ) 
                {
                    GetStream()->Write( z, 1 );
                }
                else
                {
                    this->EncodeTuple( m_tuple, m_count ); 
                }

                m_tuple = 0;
                m_count = 0;
                break;
        }
        --lLen;
        ++pBuffer;
    }
}

void PdfAscii85Filter::EndEncodeImpl()
{
    if( m_count > 0 )
        this->EncodeTuple( m_tuple, m_count );
    //GetStream()->Write( "~>", 2 );
}

void PdfAscii85Filter::BeginDecodeImpl( const PdfDictionary* )
{ 
    m_count = 0;
    m_tuple = 0;
}

void PdfAscii85Filter::DecodeBlockImpl( const char* pBuffer, pdf_long lLen )
{
    bool foundEndMarker = false;

    while( lLen && !foundEndMarker ) 
    {
        switch ( *pBuffer ) 
        {
            default:
                if ( *pBuffer < '!' || *pBuffer > 'u') 
                {
                    PODOFO_RAISE_ERROR( ePdfError_ValueOutOfRange );
                }

                m_tuple += ( *pBuffer - '!') * sPowers85[m_count++];
                if( m_count == 5 ) 
                {
                    WidePut( m_tuple, 4 );
                    m_count = 0;
                    m_tuple = 0;
                }
                break;
            case 'z':
                if (m_count != 0 ) 
                {
                    PODOFO_RAISE_ERROR( ePdfError_ValueOutOfRange );
                }

                this->WidePut( 0, 4 );
                break;
            case '~':
                ++pBuffer; 
                --lLen;
                if( lLen && *pBuffer != '>' ) 
                {
                    PODOFO_RAISE_ERROR( ePdfError_ValueOutOfRange );
                }
                foundEndMarker = true;
                break;
            case '\n': case '\r': case '\t': case ' ':
            case '\0': case '\f': case '\b': case 0177:
                break;
        }

        --lLen;
        ++pBuffer;
    }
}

void PdfAscii85Filter::EndDecodeImpl()
{ 
    if( m_count > 0 ) 
    {
        m_count--;
        m_tuple += sPowers85[m_count];
        WidePut( m_tuple, m_count );
    }
}

void PdfAscii85Filter::WidePut( unsigned long tuple, int bytes ) const
{
    char data[4];

    switch( bytes ) 
    {
	case 4:
            data[0] = static_cast<char>(tuple >> 24);
            data[1] = static_cast<char>(tuple >> 16);
            data[2] = static_cast<char>(tuple >>  8);
            data[3] = static_cast<char>(tuple);
            break;
	case 3:
            data[0] = static_cast<char>(tuple >> 24);
            data[1] = static_cast<char>(tuple >> 16);
            data[2] = static_cast<char>(tuple >>  8);
            break;
	case 2:
            data[0] = static_cast<char>(tuple >> 24);
            data[1] = static_cast<char>(tuple >> 16);
            break;
	case 1:
            data[0] = static_cast<char>(tuple >> 24);
            break;
    }

    GetStream()->Write( data, bytes );
}

// -------------------------------------------------------
// Flate
// -------------------------------------------------------
PdfFlateFilter::PdfFlateFilter()
    : m_pPredictor( 0 )
{
    memset( m_buffer, 0, sizeof(m_buffer) );
    memset( &m_stream, 0, sizeof(m_stream) );
}

PdfFlateFilter::~PdfFlateFilter()
{
    delete m_pPredictor;
}

void PdfFlateFilter::BeginEncodeImpl()
{
    m_stream.zalloc   = Z_NULL;
    m_stream.zfree    = Z_NULL;
    m_stream.opaque   = Z_NULL;

    if( deflateInit( &m_stream, Z_DEFAULT_COMPRESSION ) )
    {
        PODOFO_RAISE_ERROR( ePdfError_Flate );
    }
}

void PdfFlateFilter::EncodeBlockImpl( const char* pBuffer, pdf_long lLen )
{
    this->EncodeBlockInternal( pBuffer, lLen, Z_NO_FLUSH );
}

void PdfFlateFilter::EncodeBlockInternal( const char* pBuffer, pdf_long lLen, int nMode )
{
    int nWrittenData = 0;

    m_stream.avail_in = static_cast<long>(lLen);
    m_stream.next_in  = reinterpret_cast<Bytef*>(const_cast<char*>(pBuffer));

    do {
        m_stream.avail_out = PODOFO_FILTER_INTERNAL_BUFFER_SIZE;
        m_stream.next_out  = m_buffer;

        if( deflate( &m_stream, nMode) == Z_STREAM_ERROR )
        {
            FailEncodeDecode();
            PODOFO_RAISE_ERROR( ePdfError_Flate );
        }


        nWrittenData = PODOFO_FILTER_INTERNAL_BUFFER_SIZE - m_stream.avail_out;
        try {
            if( nWrittenData > 0 ) 
            {
                GetStream()->Write( reinterpret_cast<char*>(m_buffer), nWrittenData );
            }
        } catch( PdfError & e ) {
            // clean up after any output stream errors
            FailEncodeDecode();
            e.AddToCallstack( __FILE__, __LINE__ );
            throw e;
        }
    } while( m_stream.avail_out == 0 );
}

void PdfFlateFilter::EndEncodeImpl()
{
    this->EncodeBlockInternal( NULL, 0, Z_FINISH );
    deflateEnd( &m_stream );
}

// --

void PdfFlateFilter::BeginDecodeImpl( const PdfDictionary* pDecodeParms )
{
    m_stream.zalloc   = Z_NULL;
    m_stream.zfree    = Z_NULL;
    m_stream.opaque   = Z_NULL;

    m_pPredictor = pDecodeParms ? new PdfPredictorDecoder( pDecodeParms ) : NULL;

    if( inflateInit( &m_stream ) != Z_OK )
    {
        PODOFO_RAISE_ERROR( ePdfError_Flate );
    }
}

void PdfFlateFilter::DecodeBlockImpl( const char* pBuffer, pdf_long lLen )
{
    int flateErr;
    int nWrittenData;

    m_stream.avail_in = static_cast<long>(lLen);
    m_stream.next_in  = reinterpret_cast<Bytef*>(const_cast<char*>(pBuffer));

    do {
        m_stream.avail_out = PODOFO_FILTER_INTERNAL_BUFFER_SIZE;
        m_stream.next_out  = m_buffer;

        switch( (flateErr = inflate(&m_stream, Z_NO_FLUSH)) ) {
            case Z_NEED_DICT:
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
            {
                PdfError::LogMessage( eLogSeverity_Error, "Flate Decoding Error from ZLib: %i\n", flateErr );
                (void)inflateEnd(&m_stream);

                FailEncodeDecode();
                PODOFO_RAISE_ERROR( ePdfError_Flate );
            }
            default:
                break;
        }

        nWrittenData = PODOFO_FILTER_INTERNAL_BUFFER_SIZE - m_stream.avail_out;
        try {
            if( m_pPredictor ) 
                m_pPredictor->Decode( reinterpret_cast<char*>(m_buffer), nWrittenData, GetStream() );
            else
                GetStream()->Write( reinterpret_cast<char*>(m_buffer), nWrittenData );
        } catch( PdfError & e ) {
            // clean up after any output stream errors
            FailEncodeDecode();
            e.AddToCallstack( __FILE__, __LINE__ );
            throw e;
        }
    } while( m_stream.avail_out == 0 );
}

void PdfFlateFilter::EndDecodeImpl()
{
    delete m_pPredictor;
    m_pPredictor = NULL;

    (void)inflateEnd(&m_stream);
}

// -------------------------------------------------------
// RLE
// -------------------------------------------------------

void PdfRLEFilter::BeginEncodeImpl()
{
    PODOFO_RAISE_ERROR( ePdfError_UnsupportedFilter );
}

void PdfRLEFilter::EncodeBlockImpl( const char*, pdf_long )
{
    PODOFO_RAISE_ERROR( ePdfError_UnsupportedFilter );
}

void PdfRLEFilter::EndEncodeImpl()
{
    PODOFO_RAISE_ERROR( ePdfError_UnsupportedFilter );
}

void PdfRLEFilter::BeginDecodeImpl( const PdfDictionary* )
{ 
    m_nCodeLen = 0;
}

void PdfRLEFilter::DecodeBlockImpl( const char* pBuffer, pdf_long lLen )
{
    while( lLen-- )
    {
        if( !m_nCodeLen )
        {
            m_nCodeLen = static_cast<int>(*pBuffer);
        } else if( m_nCodeLen == 128 )
            break;
        else if( m_nCodeLen <= 127 )
        {
            GetStream()->Write( pBuffer, 1 );
            m_nCodeLen--;
        }
        else if( m_nCodeLen >= 129 )
        {
            m_nCodeLen = 257 - m_nCodeLen;

            while( m_nCodeLen-- ) 
                GetStream()->Write( pBuffer, 1 );
        }

        ++pBuffer;
    }
}

// -------------------------------------------------------
// LZW
// -------------------------------------------------------

const unsigned short PdfLZWFilter::s_masks[] = { 0x01FF,
                                        0x03FF,
                                        0x07FF,
                                        0x0FFF };

const unsigned short PdfLZWFilter::s_clear  = 0x0100;      // clear table
const unsigned short PdfLZWFilter::s_eod    = 0x0101;      // end of data

PdfLZWFilter::PdfLZWFilter()
    : m_mask(0),
    m_code_len(0),
    m_character(0),
    m_bFirst(false),
    m_pPredictor( 0 )
{
}

PdfLZWFilter::~PdfLZWFilter()
{
    delete m_pPredictor;
}

void PdfLZWFilter::BeginEncodeImpl()
{
    PODOFO_RAISE_ERROR( ePdfError_UnsupportedFilter );
}

void PdfLZWFilter::EncodeBlockImpl( const char*, pdf_long )
{
    PODOFO_RAISE_ERROR( ePdfError_UnsupportedFilter );
}

void PdfLZWFilter::EndEncodeImpl()
{
    PODOFO_RAISE_ERROR( ePdfError_UnsupportedFilter );
}

void PdfLZWFilter::BeginDecodeImpl( const PdfDictionary* pDecodeParms )
{ 
    m_mask       = 0;
    m_code_len   = 9;
    m_character  = 0;

    m_bFirst     = true;

    m_pPredictor = pDecodeParms ? new PdfPredictorDecoder( pDecodeParms ) : NULL;

    InitTable();
}

void PdfLZWFilter::DecodeBlockImpl( const char* pBuffer, pdf_long lLen )
{
    unsigned int       buffer_size = 0;
    const unsigned int buffer_max  = 24;

    pdf_uint32         old         = 0;
    pdf_uint32         code        = 0;
    pdf_uint32         buffer      = 0;

    TLzwItem           item;

    std::vector<unsigned char> data;

    if( m_bFirst ) 
    {
        m_character = *pBuffer;
        m_bFirst    = false;
    }

    while( lLen ) 
    {
        // Fill the buffer
        while( buffer_size <= (buffer_max-8) && lLen )
        {
            buffer <<= 8;
            buffer |= static_cast<pdf_uint32>(static_cast<unsigned char>(*pBuffer));
            buffer_size += 8;

            ++pBuffer;
            lLen--;
        }

        // read from the buffer
        while( buffer_size >= m_code_len ) 
        {
            code         = (buffer >> (buffer_size - m_code_len)) & PdfLZWFilter::s_masks[m_mask];
            buffer_size -= m_code_len;

            if( code == PdfLZWFilter::s_clear ) 
            {
                m_mask     = 0;
                m_code_len = 9;

                InitTable();
            }
            else if( code == PdfLZWFilter::s_eod ) 
            {
                lLen = 0;
                break;
            }
            else 
            {
                if( code >= m_table.size() )
                {
                    if (old >= m_table.size())
                    {
                        PODOFO_RAISE_ERROR( ePdfError_ValueOutOfRange );
                    }
                    data = m_table[old].value;
                    data.push_back( m_character );
                }
                else
                    data = m_table[code].value;

                // Write data to the output device
                if( m_pPredictor ) 
                    m_pPredictor->Decode( reinterpret_cast<char*>(&(data[0])), data.size(), GetStream() );
                else
                    GetStream()->Write( reinterpret_cast<char*>(&(data[0])), data.size());

                m_character = data[0];
                if( old < m_table.size() ) // fix the first loop
                    data = m_table[old].value;
                data.push_back( m_character );

                item.value = data;
                m_table.push_back( item );

                old = code;

                switch( m_table.size() ) 
                {
                    case 511:
                    case 1023:
                    case 2047:
                        ++m_code_len;
                        ++m_mask;
                    default:
                        break;
                }
            }
        }
    }
}

void PdfLZWFilter::EndDecodeImpl()
{
    delete m_pPredictor;
    m_pPredictor = NULL;
}

void PdfLZWFilter::InitTable()
{
    int      i;
    TLzwItem item;

    m_table.clear();
    m_table.reserve( LZW_TABLE_SIZE );

    for( i=0;i<=255;i++ )
    {
        item.value.clear();
        item.value.push_back( static_cast<unsigned char>(i) );
        m_table.push_back( item );
    }

    // Add dummy entry, which is never used by decoder
    item.value.clear();
    m_table.push_back( item );
}



// -------------------------------------------------------
// DCTDecode
// -------------------------------------------------------
#ifdef PODOFO_HAVE_JPEG_LIB

/*
 * Handlers for errors inside the JPeg library
 */
extern "C" {
void JPegErrorExit(j_common_ptr cinfo)
{
#if 1
    char buffer[JMSG_LENGTH_MAX];

    /* Create the message */
    (*cinfo->err->format_message) (cinfo, buffer);
#endif
    jpeg_destroy(cinfo);
    PODOFO_RAISE_ERROR_INFO( ePdfError_UnsupportedImageFormat, buffer);
}

void JPegErrorOutput(j_common_ptr, int)
{
}

};

/*
 * The actual filter implementation
 */
PdfDCTFilter::PdfDCTFilter()
    : m_pDevice( NULL )
{
}

PdfDCTFilter::~PdfDCTFilter()
{
}

void PdfDCTFilter::BeginEncodeImpl()
{
    PODOFO_RAISE_ERROR( ePdfError_UnsupportedFilter );
}

void PdfDCTFilter::EncodeBlockImpl( const char*, pdf_long )
{
    PODOFO_RAISE_ERROR( ePdfError_UnsupportedFilter );
}

void PdfDCTFilter::EndEncodeImpl()
{
    PODOFO_RAISE_ERROR( ePdfError_UnsupportedFilter );
}

void PdfDCTFilter::BeginDecodeImpl( const PdfDictionary* )
{ 
    // Setup variables for JPEGLib
    m_cinfo.err = jpeg_std_error( &m_jerr );
    m_jerr.error_exit = &JPegErrorExit;
    m_jerr.emit_message = &JPegErrorOutput;


    jpeg_create_decompress( &m_cinfo );

    m_pDevice = new PdfOutputDevice( &m_buffer );
}

void PdfDCTFilter::DecodeBlockImpl( const char* pBuffer, pdf_long lLen )
{
    m_pDevice->Write( pBuffer, lLen );
}

void PdfDCTFilter::EndDecodeImpl()
{
    delete m_pDevice;
    m_pDevice = NULL;

    jpeg_memory_src ( &m_cinfo, reinterpret_cast<JOCTET*>(m_buffer.GetBuffer()), m_buffer.GetSize() );

    if( jpeg_read_header(&m_cinfo, TRUE) <= 0 )
    {
        (void) jpeg_destroy_decompress(&m_cinfo);

        PODOFO_RAISE_ERROR( ePdfError_UnexpectedEOF );
    }

    jpeg_start_decompress(&m_cinfo);


    char*      pOutBuffer;
    JSAMPARRAY pBuffer;	
    long       lRowBytes   = m_cinfo.output_width * m_cinfo.output_components;
    const int  iComponents = m_cinfo.output_components;

    // pBuffer will be deleted by jpeg_destroy_decompress
    pBuffer    = (*m_cinfo.mem->alloc_sarray)( reinterpret_cast<j_common_ptr>( &m_cinfo ), JPOOL_IMAGE, lRowBytes, 1);
    pOutBuffer = static_cast<char*>(podofo_calloc( lRowBytes, sizeof(char)) );
	if (!pOutBuffer)
	{
		PODOFO_RAISE_ERROR(ePdfError_OutOfMemory);
	}

    while( m_cinfo.output_scanline < m_cinfo.output_height ) 
    {
        jpeg_read_scanlines(&m_cinfo, pBuffer, 1);
        if( iComponents == 4 ) 
        {
            for( unsigned int i=0, c=0; i < m_cinfo.output_width; i++, c+=4 ) 
            {
                pOutBuffer[c]   = pBuffer[0][i*4];
                pOutBuffer[c+1] = pBuffer[0][i*4+1];
                pOutBuffer[c+2] = pBuffer[0][i*4+2];
                pOutBuffer[c+3] = pBuffer[0][i*4+3];
            }
        }
        else if( iComponents == 3 ) 
        {
            for( unsigned int i=0, c=0; i < m_cinfo.output_width; i++, c+=3 ) 
            {
                pOutBuffer[c]   = pBuffer[0][i*3];
                pOutBuffer[c+1] = pBuffer[0][i*3+1];
                pOutBuffer[c+2] = pBuffer[0][i*3+2];
            }
        }
        else if( iComponents == 1 ) 
        {
            memcpy( pOutBuffer, pBuffer[0], m_cinfo.output_width );
        }
        else
        {
            PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "DCTDecode unknown components" );
        }
        
        GetStream()->Write( reinterpret_cast<char*>(pOutBuffer), lRowBytes );
    }

    podofo_free( pOutBuffer );
    (void) jpeg_destroy_decompress( &m_cinfo );
}

// -------------------------------------------------------
// memsrc.c
// -------------------------------------------------------
/*
 * memsrc.c
 *
 * Copyright (C) 1994-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains decompression data source routines for the case of
 * reading JPEG data from a memory buffer that is preloaded with the entire
 * JPEG file. This would not seem especially useful at first sight, but
 * a number of people have asked for it.
 * This is really just a stripped-down version of jdatasrc.c. Comparison
 * of this code with jdatasrc.c may be helpful in seeing how to make
 * custom source managers for other purposes.
*/



/* Expanded data source object for memory input */
typedef struct {
    struct jpeg_source_mgr pub; /* public fields */
    JOCTET eoi_buffer[2]; /* a place to put a dummy EOI */
} my_source_mgr;

typedef my_source_mgr * my_src_ptr;

/*
 * Initialize source --- called by jpeg_read_header
 * before any data is actually read.
 */



METHODDEF(void)
init_source (j_decompress_ptr)
{
    /* No work, since jpeg_memory_src set up the buffer pointer and count.
     * Indeed, if we want to read multiple JPEG images from one buffer,
     * this *must* not do anything to the pointer.
     */
}

/*
 * Fill the input buffer --- called whenever buffer is emptied.
 *
 * In this application, this routine should never be called; if it is called,
 * the decompressor has overrun the end of the input buffer, implying we
 * supplied an incomplete or corrupt JPEG datastream. A simple error exit
 * might be the most appropriate response.
 *
 * But what we choose to do in this code is to supply dummy EOI markers
 * in order to force the decompressor to finish processing and supply
 * some sort of output image, no matter how corrupted.
 */

METHODDEF(boolean)
fill_input_buffer (j_decompress_ptr cinfo)
{
    my_src_ptr src = reinterpret_cast<my_src_ptr>(cinfo->src);
    WARNMS(cinfo, JWRN_JPEG_EOF);

    /* Create a fake EOI marker */
    src->eoi_buffer[0] = static_cast<JOCTET>(0xFF);
    src->eoi_buffer[1] = static_cast<JOCTET>(JPEG_EOI);
    src->pub.next_input_byte = src->eoi_buffer;
    src->pub.bytes_in_buffer = 2;

    return TRUE;
}

/*
 * Skip data --- used to skip over a potentially large amount of
 * uninteresting data (such as an APPn marker).
 *
 * If we overrun the end of the buffer, we let fill_input_buffer deal with
 * it. An extremely large skip could cause some time-wasting here, but
 * it really isn't supposed to happen ... and the decompressor will never
 * skip more than 64K anyway.
 */
METHODDEF(void)
skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
    my_src_ptr src = reinterpret_cast<my_src_ptr>(cinfo->src);

    if (num_bytes > 0) {
        while (num_bytes > static_cast<long>(src->pub.bytes_in_buffer) ) {
            num_bytes -= static_cast<long>(src->pub.bytes_in_buffer);
            fill_input_buffer(cinfo);
            /* note we assume that fill_input_buffer will never return FALSE,
             * so suspension need not be handled.
             */
        }

        src->pub.next_input_byte += static_cast<size_t>(num_bytes);
        src->pub.bytes_in_buffer -= static_cast<size_t>(num_bytes);
    }
}

/*
 * An additional method that can be provided by data source modules is the
 * resync_to_restart method for error recovery in the presence of RST markers.
 * For the moment, this source module just uses the default resync method
 * provided by the JPEG library. That method assumes that no backtracking
 * is possible.
 */

/*
 * Terminate source --- called by jpeg_finish_decompress
 * after all data has been read. Often a no-op.
 *
 * NB: *not* called by jpeg_abort or jpeg_destroy; surrounding
 * application must deal with any cleanup that should happen even
 * for error exit.
 */
METHODDEF(void)
term_source (j_decompress_ptr)
{
    /* no work necessary here */
}

/*
 * Prepare for input from a memory buffer.
 */
GLOBAL(void)
jpeg_memory_src (j_decompress_ptr cinfo, const JOCTET * buffer, size_t bufsize)
{
    my_src_ptr src;

    /* The source object is made permanent so that a series of JPEG images
     * can be read from a single buffer by calling jpeg_memory_src
     * only before the first one.
     * This makes it unsafe to use this manager and a different source
     * manager serially with the same JPEG object. Caveat programmer.
     */

    if (cinfo->src == NULL) { /* first time for this JPEG object? */
        cinfo->src = static_cast<struct jpeg_source_mgr *>(
            (*cinfo->mem->alloc_small) ( reinterpret_cast<j_common_ptr>(cinfo), JPOOL_PERMANENT,
                                        sizeof(my_source_mgr)));

    }


    src = reinterpret_cast<my_src_ptr>(cinfo->src);
    src->pub.init_source = init_source;
    src->pub.fill_input_buffer = fill_input_buffer;
    src->pub.skip_input_data = skip_input_data;
    src->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
    src->pub.term_source = term_source;

    src->pub.next_input_byte = buffer;
    src->pub.bytes_in_buffer = bufsize;
}                                


#endif // PODOFO_HAVE_JPEG_LIB

#ifdef PODOFO_HAVE_TIFF_LIB

#ifdef DS_CCITT_DEVELOPMENT_CODE
// -------------------------------------------------------
// 
// -------------------------------------------------------
static tsize_t dummy_read(thandle_t, tdata_t, tsize_t)
{
    return 0;
}

// -------------------------------------------------------
// 
// -------------------------------------------------------
static tsize_t dummy_write(thandle_t, tdata_t, tsize_t size)
{
    return size;
}

// -------------------------------------------------------
// 
// -------------------------------------------------------
static toff_t dummy_seek(thandle_t, toff_t, int)
{

}

// -------------------------------------------------------
// 
// -------------------------------------------------------
static int dummy_close(thandle_t)
{

}

// -------------------------------------------------------
// 
// -------------------------------------------------------
static toff_t dummy_size(thandle_t)
{

}
#endif

// -------------------------------------------------------
// Actual filter code below
// -------------------------------------------------------
PdfCCITTFilter::PdfCCITTFilter()
    : m_tiff( NULL )
{
}

PdfCCITTFilter::~PdfCCITTFilter()
{
}

void PdfCCITTFilter::BeginEncodeImpl()
{
    PODOFO_RAISE_ERROR( ePdfError_UnsupportedFilter );
}

void PdfCCITTFilter::EncodeBlockImpl( const char*, pdf_long )
{
    PODOFO_RAISE_ERROR( ePdfError_UnsupportedFilter );
}

void PdfCCITTFilter::EndEncodeImpl()
{
    PODOFO_RAISE_ERROR( ePdfError_UnsupportedFilter );
}

#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif
void PdfCCITTFilter::BeginDecodeImpl( const PdfDictionary* pDict )
{ 
#ifdef DS_CCITT_DEVELOPMENT_CODE

    if( !pDict )
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidHandle, "PdfCCITTFilter required a DecodeParms dictionary" );
    } 

    m_tiff = TIFFClientOpen("podofo", "w", reinterpret_cast<thandle_t>(-1),
                            dummy_read, dummy_write,
                            dummy_seek, dummy_close, dummy_size, NULL, NULL);

    if( !m_tiff ) 
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidHandle, "TIFFClientOpen failed" );
    }

    m_tiff->tif_mode = O_RDONLY;

    TIFFSetField(m_tiff, TIFFTAG_IMAGEWIDTH,      pDict->GetKeyAsLong( PdfName("Columns"), 1728 )->GetNumber() );
    TIFFSetField(m_tiff, TIFFTAG_SAMPLESPERPIXEL, 1);
    TIFFSetField(m_tiff, TIFFTAG_BITSPERSAMPLE,   1);
    TIFFSetField(m_tiff, TIFFTAG_FILLORDER,       FILLORDER_LSB2MSB);
    TIFFSetField(m_tiff, TIFFTAG_PLANARCONFIG,    PLANARCONFIG_CONTIG);
    TIFFSetField(m_tiff, TIFFTAG_PHOTOMETRIC,     PHOTOMETRIC_MINISWHITE);
    TIFFSetField(m_tiff, TIFFTAG_YRESOLUTION,     196.);
    TIFFSetField(m_tiff, TIFFTAG_RESOLUTIONUNIT,  RESUNIT_INCH);

    /*
    m_tiff->tif_scanlinesize = TIFFSetField(m_tiff );

    if( pDict ) 
    {
        long lEncoding = pDict->GetKeyAsLong( PdfName("K"), 0 );
        if( lEncoding == 0 ) // pure 1D encoding, Group3 1D
        {
            TIFFSetField(faxTIFF,TIFFTAG_GROUP3OPTIONS, GROUP3OPT_1DENCODING);

        }
        else if( lEncoding < 0 ) // pure 2D encoding, Group4
        {
            TIFFSetField(faxTIFF,TIFFTAG_GROUP4OPTIONS, GROUP4OPT_2DENCODING);
        }
        else //if( lEncoding > 0 )  // mixed, Group3 2D
        {
            TIFFSetField(faxTIFF,TIFFTAG_GROUP3OPTIONS, GROUP3OPT_2DENCODING);
        }

    }
    */

#endif // DS_CCITT_DEVELOPMENT_CODE


}
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif

void PdfCCITTFilter::DecodeBlockImpl( const char*, pdf_long )
{

}

void PdfCCITTFilter::EndDecodeImpl()
{
}

#endif // PODOFO_HAVE_TIFF_LIB


};
