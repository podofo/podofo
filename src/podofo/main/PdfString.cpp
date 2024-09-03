/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfString.h"

#include <utf8cpp/utf8.h>

#include <podofo/private/PdfEncodingPrivate.h>

#include "PdfPredefinedEncoding.h"
#include "PdfEncodingFactory.h"
#include "PdfTokenizer.h"
#include <podofo/auxiliary/OutputDevice.h>

using namespace std;
using namespace PoDoFo;

namespace
{
    enum class StringEncoding
    {
        utf8,
        utf16be,
        utf16le,
        PdfDocEncoding
    };
}

static StringEncoding getEncoding(const string_view& view);

PdfString::PdfString()
    : m_data(new StringData()), m_isHex(false)
{
}

PdfString::PdfString(charbuff&& buff, bool isHex)
    : m_data(new StringData(std::move(buff), false)), m_isHex(isHex)
{
}

PdfString::PdfString(const string& str)
    : m_isHex(false)
{
    // Avoid copying an empty string
    if (str.empty())
        m_data.reset(new StringData());
    else
        m_data.reset(new StringData(charbuff(str), true));
}

PdfString::PdfString(const string_view& view)
    : m_isHex(false)
{
    if (view.data() == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "String is null");

    // Avoid copying an empty string
    if (view.empty())
        m_data.reset(new StringData());
    else
        m_data.reset(new StringData(charbuff(view), true));
}

PdfString::PdfString(string&& str)
    : m_data(new StringData(charbuff(std::move(str)), true)), m_isHex(false)
{
}

PdfString PdfString::FromRaw(const bufferview& view, bool isHex)
{
    return PdfString((charbuff)view, isHex);
}

PdfString PdfString::FromHexData(const string_view& hexView, const PdfStatefulEncrypt* encrypt)
{
    size_t len = hexView.size();
    charbuff buffer;
    buffer.reserve(len % 2 ? (len + 1) >> 1 : len >> 1);

    unsigned char val;
    char decodedChar = 0;
    bool low = true;
    for (size_t i = 0; i < len; i++)
    {
        char ch = hexView[i];
        if (PdfTokenizer::IsWhitespace(ch))
            continue;

        (void)utls::TryGetHexValue(ch, val);
        if (low)
        {
            decodedChar = (char)(val & 0x0F);
            low = false;
        }
        else
        {
            decodedChar = (char)((decodedChar << 4) | val);
            low = true;
            buffer.push_back(decodedChar);
        }
    }

    if (!low)
    {
        // an odd number of bytes was read,
        // so the last byte is 0
        buffer.push_back(decodedChar);
    }

    if (encrypt != nullptr)
    {
        charbuff decrypted;
        encrypt->DecryptTo(decrypted, buffer);
        return PdfString(std::move(decrypted), true);
    }
    else
    {
        buffer.shrink_to_fit();
        return PdfString(std::move(buffer), true);
    }
}

void PdfString::Write(OutputStream& device, PdfWriteFlags writeMode,
    const PdfStatefulEncrypt* encrypt, charbuff& buffer) const
{
    (void)writeMode;
    (void)buffer; // TODO: Just use the supplied buffer instead of the many ones below

    // Strings in PDF documents may contain \0 especially if they are encrypted
    // this case has to be handled!

    string_view view;
    if (m_data->CharsAllocated)
        view = m_data->Chars;
    else
        view = m_data->Utf8View;

    u16string string16;
    string pdfDocEncoded;
    if (m_data->StringEvaluated)
    {
        if (m_data->CharSet == PdfStringCharset::Unknown)
        {
            bool isAsciiEqual;
            if (PoDoFo::CheckValidUTF8ToPdfDocEcondingChars(view, isAsciiEqual))
                m_data->CharSet = isAsciiEqual ? PdfStringCharset::Ascii : PdfStringCharset::PdfDocEncoding;
            else
                m_data->CharSet = PdfStringCharset::Unicode;
        }

        switch (m_data->CharSet)
        {
            case PdfStringCharset::Ascii:
            {
                // Ascii charset can be serialized without further processing
                break;
            }
            case PdfStringCharset::PdfDocEncoding:
            {
                (void)PoDoFo::TryConvertUTF8ToPdfDocEncoding(view, pdfDocEncoded);
                view = string_view(pdfDocEncoded);
                break;
            }
            case PdfStringCharset::Unicode:
            {
                // Prepend utf-16 BE BOM
                string16.push_back((char16_t)(0xFEFF));
                utf8::utf8to16(view.data(), view.data() + view.size(), std::back_inserter(string16));
#ifdef PODOFO_IS_LITTLE_ENDIAN
                // Ensure the output will be BE
                utls::ByteSwap(string16);
#endif
                view = string_view((const char*)string16.data(), string16.size() * sizeof(char16_t));
                break;
            }
            default:
                PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
        }
    }

    // NOTE: We do not encrypt empty strings (it's access violation)
    charbuff tempBuffer;
    if (encrypt != nullptr && view.size() > 0)
    {
        charbuff encrypted;
        encrypt->EncryptTo(encrypted, view);
        encrypted.swap(tempBuffer);
        view = string_view(tempBuffer.data(), tempBuffer.size());
    }

    utls::SerializeEncodedString(device, view, m_isHex);
}

PdfStringCharset PdfString::GetCharset() const
{
    return m_data->CharSet;
}

string_view PdfString::GetString() const
{
    if (m_data->CharsAllocated)
    {
        ensureCharsEvaluated();
        return m_data->Chars;
    }
    else
    {
        return m_data->Utf8View;
    }
}

bool PdfString::IsEmpty() const
{
    if (m_data->CharsAllocated)
        return m_data->Chars.empty();
    else
        return m_data->Utf8View.empty();
}

bool PdfString::IsStringEvaluated() const
{
    return m_data->StringEvaluated;
}

bool PdfString::operator==(const PdfString& rhs) const
{
    if (this == &rhs)
        return true;

    if (this->m_data == rhs.m_data)
        return true;

    if (this->m_data->StringEvaluated != rhs.m_data->StringEvaluated)
        return false;

    if (m_data->CharsAllocated)
        return this->m_data->Chars == rhs.m_data->Chars;
    else
        return this->m_data->Utf8View == rhs.m_data->Utf8View;
}

bool PdfString::operator==(const char* str) const
{
    return operator==(string_view(str, std::strlen(str)));
}

bool PdfString::operator==(const string& str) const
{
    return operator==((string_view)str);
}

bool PdfString::operator==(const string_view& view) const
{
    if (m_data->CharsAllocated)
    {
        ensureCharsEvaluated();
        return m_data->Chars == view;
    }
    else
    {
        return m_data->Utf8View == view;
    }
}

bool PdfString::operator!=(const PdfString& rhs) const
{
    if (this == &rhs)
        return false;

    if (this->m_data == rhs.m_data)
        return false;

    if (this->m_data->StringEvaluated != rhs.m_data->StringEvaluated)
        return true;

    if (m_data->CharsAllocated)
        return this->m_data->Chars != rhs.m_data->Chars;
    else
        return this->m_data->Utf8View != rhs.m_data->Utf8View;
}

bool PdfString::operator!=(const char* str) const
{
    return operator!=(string_view(str, std::strlen(str)));
}

bool PdfString::operator!=(const string& str) const
{
    return operator!=((string_view)str);
}

bool PdfString::operator!=(const string_view& view) const
{
    if (m_data->CharsAllocated)
    {
        ensureCharsEvaluated();
        return m_data->Chars != view;
    }
    else
    {
        return m_data->Utf8View != view;
    }
}

PdfString::operator string_view() const
{
    if (m_data->CharsAllocated)
    {
        ensureCharsEvaluated();
        return m_data->Chars;
    }
    else
    {
        return m_data->Utf8View;
    }
}

void PdfString::initFromUtf8String(const char* str, size_t length, bool literal)
{
    if (str == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "String is null");

    if (literal)
    {
        m_data.reset(new StringData(string_view(str, length)));
    }
    else
    {
        if (length == 0)
        {
            // Avoid copying an empty string
            m_data.reset(new StringData());
        }
        else
        {
            m_data.reset(new StringData(charbuff(str, length), true));
        }
    }
}

void PdfString::ensureCharsEvaluated() const
{
    PODOFO_INVARIANT(m_data->CharsAllocated);
    if (m_data->StringEvaluated)
        return;

    // CHECK-ME: Evaluate levaving the charset indeterminate
    auto encoding = getEncoding(m_data->Chars);
    switch (encoding)
    {
        case StringEncoding::utf16be:
        {
            // Remove BOM and decode utf-16 string
            string utf8;
            auto view = string_view(m_data->Chars).substr(2);
            utls::ReadUtf16BEString(view, utf8);
            utf8.swap(m_data->Chars);
            m_data->CharSet = PdfStringCharset::Unicode;
            break;
        }
        case StringEncoding::utf16le:
        {
            // Remove BOM and decode utf-16 string
            string utf8;
            auto view = string_view(m_data->Chars).substr(2);
            utls::ReadUtf16LEString(view, utf8);
            utf8.swap(m_data->Chars);
            m_data->CharSet = PdfStringCharset::Unicode;
            break;
        }
        case StringEncoding::utf8:
        {
            // Remove BOM
            m_data->Chars.substr(3).swap(m_data->Chars);
            m_data->CharSet = PdfStringCharset::Unicode;
            break;
        }
        case StringEncoding::PdfDocEncoding:
        {
            bool isAsciiEqual;
            auto utf8 = PoDoFo::ConvertPdfDocEncodingToUTF8(m_data->Chars, isAsciiEqual);
            utf8.swap(m_data->Chars);
            m_data->CharSet = isAsciiEqual ? PdfStringCharset::Ascii : PdfStringCharset::PdfDocEncoding;
            break;
        }
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Unsupported");
    }

    m_data->StringEvaluated = true;
}

string_view PdfString::GetRawData() const
{
    if (m_data->StringEvaluated)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "The raw data buffer has been evaluated to a string");

    PODOFO_ASSERT(m_data->CharsAllocated);
    return m_data->Chars;
}

StringEncoding getEncoding(const string_view& view)
{
    constexpr char utf16beMarker[2] = { static_cast<char>(0xFE), static_cast<char>(0xFF) };
    if (view.size() >= sizeof(utf16beMarker) && memcmp(view.data(), utf16beMarker, sizeof(utf16beMarker)) == 0)
        return StringEncoding::utf16be;

    // NOTE: Little endian should not be officially supported
    constexpr char utf16leMarker[2] = { static_cast<char>(0xFF), static_cast<char>(0xFE) };
    if (view.size() >= sizeof(utf16leMarker) && memcmp(view.data(), utf16leMarker, sizeof(utf16leMarker)) == 0)
        return StringEncoding::utf16le;

    constexpr char utf8Marker[3] = { static_cast<char>(0xEF), static_cast<char>(0xBB), static_cast<char>(0xBF) };
    if (view.size() >= sizeof(utf8Marker) && memcmp(view.data(), utf8Marker, sizeof(utf8Marker)) == 0)
        return StringEncoding::utf8;

    return StringEncoding::PdfDocEncoding;
}

// Empty string constructor
PdfString::StringData::StringData()
    : Utf8View(""), CharsAllocated(false), StringEvaluated(true), CharSet(PdfStringCharset::Ascii) { }

// Constructor for literal strings only
PdfString::StringData::StringData(const string_view& view)
    : Utf8View(view), CharsAllocated(false), StringEvaluated(true), CharSet(PdfStringCharset::Unknown) { }

PdfString::StringData::StringData(charbuff&& buff, bool stringEvaluated)
    : Chars(std::move(buff)), CharsAllocated(true), StringEvaluated(stringEvaluated), CharSet(PdfStringCharset::Unknown) { }

PdfString::StringData::~StringData()
{
    // Manually call constructor for union
    if (CharsAllocated)
        Chars.~charbuff();
}
