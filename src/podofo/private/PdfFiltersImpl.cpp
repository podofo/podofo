/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "PdfDeclarationsPrivate.h"
#include "PdfFiltersImpl.h"

#include <podofo/main/PdfDictionary.h>
#include <podofo/main/PdfTokenizer.h>
#include <podofo/auxiliary/StreamDevice.h>

using namespace std;
using namespace PoDoFo;

namespace PoDoFo {

// Private data for PdfAscii85Filter. This will be optimised
// by the compiler through compile-time constant expression
// evaluation.
const unsigned s_Powers85[] = { 85 * 85 * 85 * 85, 85 * 85 * 85, 85 * 85, 85, 1 };

/**
 * This structure contains all necessary values
 * for a FlateDecode and LZWDecode Predictor.
 * These values are normally stored in the /DecodeParams
 * key of a PDF dictionary.
 */
class PdfPredictorDecoder
{
public:
    PdfPredictorDecoder(const PdfDictionary& decodeParms)
    {
        // check that input values are in range (CVE-2018-20797)
        // ISO 32000-2008 specifies these values as all 1 or greater
        // Negative values for Columns / Colors / BitsPerComponent result in huge alloc

        int64_t num = decodeParms.FindKeyAsSafe<int64_t>("Predictor", 1);
        if (num < 1)
        {
        OutOfRange:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Image parameters are out of range");
        }
        m_Predictor = (unsigned)num;

        num = decodeParms.FindKeyAsSafe<int64_t>("Colors", 1);
        if (num < 1)
            goto OutOfRange;
        m_Colors = (unsigned)num;

        num = decodeParms.FindKeyAsSafe<int64_t>("BitsPerComponent", 8);
        if (num < 1)
            goto OutOfRange;
        m_BitsPerComponent = (unsigned)num;

        num = decodeParms.FindKeyAsSafe<int64_t>("Columns", 1);
        if (num < 1)
            goto OutOfRange;
        m_ColumnCount = (unsigned)num;

        num = decodeParms.FindKeyAsSafe<int64_t>("EarlyChange", 1);
        m_EarlyChange = num < 1 ? 1 : (unsigned)num;

        m_BytesPerPixel = (m_BitsPerComponent * m_Colors) / 8;
        if (m_Predictor >= 10)
        {
            m_NextByteIsPredictor = true;
            m_CurrPredictor = -1;
        }
        else
        {
            m_NextByteIsPredictor = false;
            m_CurrPredictor = (int)m_Predictor;
        }

        m_CurrRowIndex = 0;
        m_Rows = (m_ColumnCount * m_Colors * m_BitsPerComponent) / 8;

        // check for multiplication overflow on buffer sizes (e.g. if m_nBPC=2 and m_nColors=SIZE_MAX/2+1)
        if (utls::DoesMultiplicationOverflow(m_BitsPerComponent, m_Colors)
            || utls::DoesMultiplicationOverflow(m_ColumnCount, (size_t)m_BitsPerComponent * m_Colors))
        {
            PODOFO_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);
        }

        // check that computed allocation sizes are > 0 (CVE-2018-20797)
        if (m_Rows < 1 || m_BitsPerComponent < 1)
            PODOFO_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);

        m_Prev.resize(m_Rows);
        memset(m_Prev.data(), 0, sizeof(char) * m_Rows);

        m_UpperLeftPixelComponents.resize(m_BytesPerPixel);
        memset(m_UpperLeftPixelComponents.data(), 0, sizeof(char) * m_BytesPerPixel);
    }

    void Decode(const char* buffer, size_t len, OutputStream& stream)
    {
        if (m_Predictor == 1)
        {
            stream.Write(buffer, len);
            return;
        }

        while (len-- != 0)
        {
            if (m_NextByteIsPredictor)
            {
                m_CurrPredictor = *buffer + 10;
                m_NextByteIsPredictor = false;
            }
            else
            {
                if (m_BitsPerComponent != 8)
                    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidPredictor, "Predictors with bits per component othern than 8 are not implemented");

                PODOFO_ASSERT(m_BytesPerPixel != 0);
                switch (m_CurrPredictor)
                {
                    case 2: // Tiff Predictor
                    {
                        // Same as png sub
                        char prev = ((int)m_CurrRowIndex - (int)m_BytesPerPixel < 0
                            ? 0 : m_Prev[m_CurrRowIndex - m_BytesPerPixel]);
                        m_Prev[m_CurrRowIndex] = *buffer + prev;
                        break;
                    }
                    case 10: // png none
                    {
                        m_Prev[m_CurrRowIndex] = *buffer;
                        break;
                    }
                    case 11: // png sub
                    {
                        char prev = ((int)m_CurrRowIndex - (int)m_BytesPerPixel < 0
                            ? 0 : m_Prev[m_CurrRowIndex - m_BytesPerPixel]);
                        m_Prev[m_CurrRowIndex] = *buffer + prev;
                        break;
                    }
                    case 12: // png up
                    {
                        m_Prev[m_CurrRowIndex] += *buffer;
                        break;
                    }
                    case 13: // png average
                    {
                        int prev = ((int)m_CurrRowIndex - (int)m_BytesPerPixel < 0
                            ? 0 : m_Prev[m_CurrRowIndex - m_BytesPerPixel]);
                        m_Prev[m_CurrRowIndex] = (char)((prev + m_Prev[m_CurrRowIndex]) >> 1) + *buffer;
                        break;
                    }
                    case 14: // png paeth
                    {
                        int nLeftByteIndex = (int)m_CurrRowIndex - (int)m_BytesPerPixel;

                        int a = nLeftByteIndex < 0 ? 0 : static_cast<unsigned char>(m_Prev[nLeftByteIndex]);
                        int b = static_cast<unsigned char>(m_Prev[m_CurrRowIndex]);

                        int nCurrComponentIndex = m_CurrRowIndex % m_BytesPerPixel;
                        int c = nLeftByteIndex < 0 ? 0 : static_cast<unsigned char>(m_UpperLeftPixelComponents[nCurrComponentIndex]);

                        int p = a + b - c;

                        int pa = p - a;
                        if (pa < 0)
                            pa = -pa;

                        int pb = p - b;
                        if (pb < 0)
                            pb = -pb;

                        int pc = p - c;
                        if (pc < 0)
                            pc = -pc;

                        int closestByte;
                        if (pa <= pb && pa <= pc)
                            closestByte = a;
                        else if (pb <= pc)
                            closestByte = b;
                        else
                            closestByte = c;

                        // Save the byte we're about to clobber for the next pixel's prediction
                        m_UpperLeftPixelComponents[nCurrComponentIndex] = m_Prev[m_CurrRowIndex];

                        m_Prev[m_CurrRowIndex] = *buffer + (char)closestByte;
                        break;
                    }
                    case 15: // png optimum
                        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidPredictor, "png optimum predictor is not implemented");
                        break;

                    default:
                    {
                        //PODOFO_RAISE_ERROR( EPdfError::InvalidPredictor );
                        break;
                    }
                }

                m_CurrRowIndex++;
            }

            buffer++;

            if (m_CurrRowIndex >= m_Rows)
            {   // One line finished
                m_CurrRowIndex = 0;
                m_NextByteIsPredictor = (m_CurrPredictor >= 10);
                stream.Write(m_Prev.data(), m_Rows);
            }
        }
    }

private:
    unsigned m_Predictor;
    unsigned m_Colors;
    unsigned m_BitsPerComponent;
    unsigned m_ColumnCount;
    unsigned m_EarlyChange;
    unsigned m_BytesPerPixel;     // Bytes per pixel

    int m_CurrPredictor;
    unsigned m_CurrRowIndex;
    unsigned m_Rows;

    bool m_NextByteIsPredictor;

    charbuff m_Prev;

    // The PNG Paeth predictor uses the values of the pixel above and to the left
    // of the current pixel. But we overwrite the row above as we go, so we'll
    // have to store the bytes of the upper-left pixel separately.
    charbuff m_UpperLeftPixelComponents;
};

} // end anonymous namespace

#pragma region PdfHexFilter

PdfHexFilter::PdfHexFilter()
    : m_DecodedByte(0), m_Low(true)
{
}

void PdfHexFilter::EncodeBlockImpl(const char* buffer, size_t len)
{
    char data[2];
    while (len-- != 0)
    {
        utls::WriteCharHexTo(data, *buffer);
        GetStream().Write(data, 2);
        buffer++;
    }
}

void PdfHexFilter::BeginDecodeImpl(const PdfDictionary*)
{
    m_DecodedByte = 0;
    m_Low = true;
}

void PdfHexFilter::DecodeBlockImpl(const char* buffer, size_t len)
{
    unsigned char val;
    while (len-- != 0)
    {
        if (PoDoFo::IsCharWhitespace(*buffer))
        {
            buffer++;
            continue;
        }

        (void)utls::TryGetHexValue(*buffer, val);
        if (m_Low)
        {
            m_DecodedByte = (char)(val & 0x0F);
            m_Low = false;
        }
        else
        {
            m_DecodedByte = (char)((m_DecodedByte << 4) | val);
            m_Low = true;

            GetStream().Write(m_DecodedByte);
        }

        buffer++;
    }
}

void PdfHexFilter::EndDecodeImpl()
{
    if (!m_Low)
    {
        // an odd number of bytes was read,
        // so the last byte is 0
        GetStream().Write(m_DecodedByte);
    }
}

#pragma endregion // PdfHexFilter

#pragma region PdfAscii85Filter

// Ascii 85
// 
// based on public domain software from:
// Paul Haahr - http://www.webcom.com/~haahr/

PdfAscii85Filter::PdfAscii85Filter()
    : m_count(0), m_tuple(0)
{
}

void PdfAscii85Filter::EncodeTuple(unsigned tuple, int count)
{
    int i = 5;
    int z = 0;
    char buf[5];
    char out[5];
    char* start = buf;;

    do
    {
        *start++ = static_cast<char>(tuple % 85);
        tuple /= 85;
    } while (--i > 0);

    i = count;
    do
    {
        out[z++] = static_cast<unsigned char>(*--start) + '!';
    } while (i-- > 0);

    GetStream().Write(out, z);
}

void PdfAscii85Filter::BeginEncodeImpl()
{
    m_count = 0;
    m_tuple = 0;
}

void PdfAscii85Filter::EncodeBlockImpl(const char* buffer, size_t len)
{
    unsigned c;
    const char* z = "z";

    while (len != 0)
    {
        c = *buffer & 0xFF;
        switch (m_count++) {
            case 0: m_tuple |= (c << 24); break;
            case 1: m_tuple |= (c << 16); break;
            case 2: m_tuple |= (c << 8); break;
            case 3:
                m_tuple |= c;
                if (m_tuple == 0)
                {
                    GetStream().Write(z, 1);
                }
                else
                {
                    this->EncodeTuple(m_tuple, m_count);
                }

                m_tuple = 0;
                m_count = 0;
                break;
        }
        len--;
        buffer++;
    }
}

void PdfAscii85Filter::EndEncodeImpl()
{
    if (m_count > 0)
        this->EncodeTuple(m_tuple, m_count);
    //GetStream().Write( "~>", 2 );
}

void PdfAscii85Filter::BeginDecodeImpl(const PdfDictionary*)
{
    m_count = 0;
    m_tuple = 0;
}

void PdfAscii85Filter::DecodeBlockImpl(const char* buffer, size_t len)
{
    bool foundEndMarker = false;

    while (len != 0 && !foundEndMarker)
    {
        switch (*buffer)
        {
            default:
                if (*buffer < '!' || *buffer > 'u')
                {
                    PODOFO_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);
                }

                m_tuple += (*buffer - '!') * s_Powers85[m_count++];
                if (m_count == 5)
                {
                    WidePut(m_tuple, 4);
                    m_count = 0;
                    m_tuple = 0;
                }
                break;
            case 'z':
                if (m_count != 0)
                {
                    PODOFO_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);
                }

                this->WidePut(0, 4);
                break;
            case '~':
                buffer++;
                len--;
                if (len != 0 && *buffer != '>')
                    PODOFO_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);

                foundEndMarker = true;
                break;
            case '\n': case '\r': case '\t': case ' ':
            case '\0': case '\f': case '\b': case 0177:
                break;
        }

        len--;
        buffer++;
    }
}

void PdfAscii85Filter::EndDecodeImpl()
{
    if (m_count > 0)
    {
        m_count--;
        m_tuple += s_Powers85[m_count];
        WidePut(m_tuple, m_count);
    }
}

void PdfAscii85Filter::WidePut(unsigned tuple, int bytes) const
{
    char data[4];

    switch (bytes)
    {
        case 4:
            data[0] = static_cast<char>(tuple >> 24);
            data[1] = static_cast<char>(tuple >> 16);
            data[2] = static_cast<char>(tuple >> 8);
            data[3] = static_cast<char>(tuple);
            break;
        case 3:
            data[0] = static_cast<char>(tuple >> 24);
            data[1] = static_cast<char>(tuple >> 16);
            data[2] = static_cast<char>(tuple >> 8);
            break;
        case 2:
            data[0] = static_cast<char>(tuple >> 24);
            data[1] = static_cast<char>(tuple >> 16);
            break;
        case 1:
            data[0] = static_cast<char>(tuple >> 24);
            break;
    }

    GetStream().Write(data, bytes);
}

#pragma endregion // PdfAscii85Filter

#pragma endregion PdfFlateFilter

PdfFlateFilter::PdfFlateFilter()
    : m_buffer{ }, m_stream{ } { }

void PdfFlateFilter::BeginEncodeImpl()
{
    m_stream.zalloc = Z_NULL;
    m_stream.zfree = Z_NULL;
    m_stream.opaque = Z_NULL;

    if (deflateInit(&m_stream, Z_DEFAULT_COMPRESSION))
        PODOFO_RAISE_ERROR(PdfErrorCode::FlateError);
}

void PdfFlateFilter::EncodeBlockImpl(const char* buffer, size_t len)
{
    this->EncodeBlockInternal(buffer, len, Z_NO_FLUSH);
}

void PdfFlateFilter::EncodeBlockInternal(const char* buffer, size_t len, int nMode)
{
    int nWrittenData = 0;

    m_stream.avail_in = static_cast<unsigned>(len);
    m_stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(buffer));

    do
    {
        m_stream.avail_out = BUFFER_SIZE;
        m_stream.next_out = m_buffer;

        if (deflate(&m_stream, nMode) == Z_STREAM_ERROR)
        {
            FailEncodeDecode();
            PODOFO_RAISE_ERROR(PdfErrorCode::FlateError);
        }

        nWrittenData = BUFFER_SIZE - m_stream.avail_out;
        try
        {
            if (nWrittenData > 0)
            {
                GetStream().Write(reinterpret_cast<char*>(m_buffer), nWrittenData);
            }
        }
        catch (PdfError& e)
        {
            // clean up after any output stream errors
            FailEncodeDecode();
            PODOFO_PUSH_FRAME(e);
            throw;
        }
    } while (m_stream.avail_out == 0);
}

void PdfFlateFilter::EndEncodeImpl()
{
    this->EncodeBlockInternal(nullptr, 0, Z_FINISH);
    deflateEnd(&m_stream);
}

void PdfFlateFilter::BeginDecodeImpl(const PdfDictionary* decodeParms)
{
    m_stream.zalloc = Z_NULL;
    m_stream.zfree = Z_NULL;
    m_stream.opaque = Z_NULL;

    if (decodeParms != nullptr)
        m_Predictor.reset(new PdfPredictorDecoder(*decodeParms));

    if (inflateInit(&m_stream) != Z_OK)
        PODOFO_RAISE_ERROR(PdfErrorCode::FlateError);
}

void PdfFlateFilter::DecodeBlockImpl(const char* buffer, size_t len)
{
    int flateErr;
    unsigned writtenDataSize;

    m_stream.avail_in = static_cast<unsigned>(len);
    m_stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(buffer));

    do
    {
        m_stream.avail_out = BUFFER_SIZE;
        m_stream.next_out = m_buffer;

        switch ((flateErr = inflate(&m_stream, Z_NO_FLUSH)))
        {
            case Z_NEED_DICT:
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
            {
                PoDoFo::LogMessage(PdfLogSeverity::Error, "Flate Decoding Error from ZLib: {}", flateErr);
                (void)inflateEnd(&m_stream);

                FailEncodeDecode();
                PODOFO_RAISE_ERROR(PdfErrorCode::FlateError);
            }
            default:
                break;
        }

        writtenDataSize = BUFFER_SIZE - m_stream.avail_out;
        try
        {
            if (m_Predictor != nullptr)
                m_Predictor->Decode(reinterpret_cast<char*>(m_buffer), writtenDataSize, GetStream());
            else
                GetStream().Write(reinterpret_cast<char*>(m_buffer), writtenDataSize);
        }
        catch (PdfError& e)
        {
            // clean up after any output stream errors
            FailEncodeDecode();
            PODOFO_PUSH_FRAME(e);
            throw;
        }
    } while (m_stream.avail_out == 0);
}

void PdfFlateFilter::EndDecodeImpl()
{
    (void)inflateEnd(&m_stream);
    m_Predictor.reset();
}

#pragma endregion // PdfFlateFilter

#pragma region PdfRLEFilter

PdfRLEFilter::PdfRLEFilter()
    : m_CodeLen(0)
{
}

void PdfRLEFilter::EncodeBlockImpl(const char*, size_t)
{
    PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

void PdfRLEFilter::BeginDecodeImpl(const PdfDictionary*)
{
    m_CodeLen = 0;
}

void PdfRLEFilter::DecodeBlockImpl(const char* buffer, size_t len)
{
    while (len-- != 0)
    {
        if (m_CodeLen == 0)
        {
            m_CodeLen = static_cast<int>(*buffer);
        }
        else if (m_CodeLen == 128)
        {
            break;
        }
        else if (m_CodeLen <= 127)
        {
            GetStream().Write(buffer, 1);
            m_CodeLen--;
        }
        else if (m_CodeLen >= 129)
        {
            m_CodeLen = 257 - m_CodeLen;

            while (m_CodeLen--)
                GetStream().Write(buffer, 1);
        }

        buffer++;
    }
}

#pragma endregion // PdfRLEFilter

#pragma region PdfLZWFilter

const unsigned short PdfLZWFilter::s_masks[] = { 0x01FF, 0x03FF, 0x07FF, 0x0FFF };
const unsigned short PdfLZWFilter::s_clear = 0x0100;      // clear table
const unsigned short PdfLZWFilter::s_eod = 0x0101;      // end of data

PdfLZWFilter::PdfLZWFilter() :
    m_mask(0),
    m_code_len(0),
    m_character(0),
    m_First(false)
{
}

void PdfLZWFilter::EncodeBlockImpl(const char*, size_t)
{
    PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

void PdfLZWFilter::BeginDecodeImpl(const PdfDictionary* decodeParms)
{
    m_mask = 0;
    m_code_len = 9;
    m_character = 0;

    m_First = true;

    if (decodeParms != nullptr)
        m_Predictor.reset(new PdfPredictorDecoder(*decodeParms));

    InitTable();
}

void PdfLZWFilter::DecodeBlockImpl(const char* buffer, size_t len)
{
    unsigned buffer_size = 0;
    const unsigned buffer_max = 24;

    uint32_t old = 0;
    uint32_t code = 0;
    uint32_t codeBuff = 0;

    TLzwItem item;

    vector<unsigned char> data;
    if (m_First)
    {
        m_character = *buffer;
        m_First = false;
    }

    while (len != 0)
    {
        // Fill the buffer
        while (buffer_size <= (buffer_max - 8) && len)
        {
            codeBuff <<= 8;
            codeBuff |= static_cast<uint32_t>(static_cast<unsigned char>(*buffer));
            buffer_size += 8;

            buffer++;
            len--;
        }

        // read from the buffer
        while (buffer_size >= m_code_len)
        {
            code = (codeBuff >> (buffer_size - m_code_len)) & PdfLZWFilter::s_masks[m_mask];
            buffer_size -= m_code_len;

            if (code == PdfLZWFilter::s_clear)
            {
                m_mask = 0;
                m_code_len = 9;

                InitTable();
            }
            else if (code == PdfLZWFilter::s_eod)
            {
                len = 0;
                break;
            }
            else
            {
                if (code >= m_table.size())
                {
                    if (old >= m_table.size())
                    {
                        PODOFO_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);
                    }
                    data = m_table[old].value;
                    data.push_back(m_character);
                }
                else
                    data = m_table[code].value;

                // Write data to the output device
                if (m_Predictor != nullptr)
                    m_Predictor->Decode(reinterpret_cast<char*>(data.data()), data.size(), GetStream());
                else
                    GetStream().Write(reinterpret_cast<char*>(data.data()), data.size());

                m_character = data[0];
                if (old < m_table.size()) // fix the first loop
                    data = m_table[old].value;
                data.push_back(m_character);

                item.value = data;
                m_table.push_back(item);

                old = code;

                switch (m_table.size())
                {
                    case 511:
                    case 1023:
                    case 2047:
                        m_code_len++;
                        m_mask++;
                        break;
                    default:
                        break;
                }
            }
        }
    }
}

void PdfLZWFilter::EndDecodeImpl()
{
    m_Predictor.reset();
}

void PdfLZWFilter::InitTable()
{
    constexpr unsigned LZW_TABLE_SIZE = 4096;
    TLzwItem item;

    m_table.clear();
    m_table.reserve(LZW_TABLE_SIZE);

    for (int i = 0; i <= 255; i++)
    {
        item.value.clear();
        item.value.push_back(static_cast<unsigned char>(i));
        m_table.push_back(item);
    }

    // Add dummy entry, which is never used by decoder
    item.value.clear();
    m_table.push_back(item);
}

#pragma endregion // PdfLZWFilter

#pragma region PdfCryptFilter

PdfCryptFilter::PdfCryptFilter() { }

void PdfCryptFilter::EncodeBlockImpl(const char* buffer, size_t len)
{
    (void)buffer;
    (void)len;
    PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

void PdfCryptFilter::BeginDecodeImpl(const PdfDictionary* dict)
{
    if (dict != nullptr)
    {
        const PdfName* name;
        if (dict->TryFindKeyAs("Name", name) && *name != "Identity")
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Unsupported Crypt filter");
    }
}

void PdfCryptFilter::DecodeBlockImpl(const char* buffer, size_t len)
{
    GetStream().Write(buffer, len);
}

#pragma endregion // PdfCryptFilter
