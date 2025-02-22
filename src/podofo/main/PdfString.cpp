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
    enum class StringEncoding : uint8_t
    {
        utf8,
        utf16be,
        utf16le,
        PdfDocEncoding
    };
}

static StringEncoding getEncoding(const string_view& view);
static PdfStringCharset getCharSet(const string_view& view);

PdfString::PdfString()
    : PdfDataMember(PdfDataType::String), m_dataAllocated(false), m_isHex(false), m_Utf8View("")
{
}

PdfString::PdfString(charbuff&& buff, bool isHex)
    : PdfDataMember(PdfDataType::String), m_dataAllocated(true), m_isHex(isHex), m_data(new StringData(std::move(buff), false))
{
}

PdfString::~PdfString()
{
    // Manually call destructor for union
    if (m_dataAllocated)
        m_data.~shared_ptr();
}

PdfString::PdfString(const string& str)
    : PdfDataMember(PdfDataType::String), m_isHex(false)
{
    // Avoid copying an empty string
    if (str.empty())
    {
        new(&m_Utf8View)string_view("");
        m_dataAllocated = false;
    }
    else
    {
        new(&m_data)shared_ptr<StringData>(new StringData((charbuff)str, true));
        m_dataAllocated = true;
    }
}

PdfString::PdfString(const string_view& view)
    : PdfDataMember(PdfDataType::String), m_isHex(false)
{
    if (view.data() == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "String is null");

    // Avoid copying an empty string
    if (view.empty())
    {
        new(&m_Utf8View)string_view("");
        m_dataAllocated = false;
    }
    else
    {
        new(&m_data)shared_ptr<StringData>(new StringData((charbuff)view, true));
        m_dataAllocated = true;
    }
}

PdfString::PdfString(string&& str)
    : PdfDataMember(PdfDataType::String), m_dataAllocated(true), m_isHex(false), m_data(new StringData(charbuff(std::move(str)), true))
{
}

PdfString::PdfString(const PdfString& rhs)
    : PdfDataMember(PdfDataType::String), m_isHex(rhs.m_isHex)
{
    if (rhs.m_dataAllocated)
    {
        new(&m_data)shared_ptr<StringData>(rhs.m_data);
        m_dataAllocated = true;
    }
    else
    {
        new(&m_Utf8View)string_view(rhs.m_Utf8View);
        m_dataAllocated = false;
    }
}

PdfString::PdfString(PdfString&& rhs) noexcept
    : PdfDataMember(PdfDataType::String)
{
    moveFrom(std::move(rhs));
}

PdfString& PdfString::operator=(const PdfString& rhs)
{
    this->~PdfString();
    if (rhs.m_dataAllocated)
    {
        new(&m_data)shared_ptr<StringData>(rhs.m_data);
        m_dataAllocated = true;
    }
    else
    {
        new(&m_Utf8View)string_view(rhs.m_Utf8View);
        m_dataAllocated = false;
    }

    m_isHex = rhs.m_isHex;
    return *this;
}

PdfString& PdfString::operator=(PdfString&& rhs) noexcept
{
    this->~PdfString();
    moveFrom(std::move(rhs));
    return *this;
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
        if (PoDoFo::IsCharWhitespace(ch))
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

void PdfString::Write(OutputStream& device, PdfWriteFlags writeFlags,
    const PdfStatefulEncrypt* encrypt, charbuff& buffer) const
{
    (void)buffer; // TODO: Just use the supplied buffer instead of the many ones below

    // Strings in PDF documents may contain \0 especially if they are encrypted
    // this case has to be handled!

    string_view view;
    bool stringEvalued;
    if (m_dataAllocated)
    {
        view = m_data->Chars;
        stringEvalued = m_data->StringEvaluated;
    }
    else
    {
        view = m_Utf8View;
        stringEvalued = true;
    }

    u16string string16;
    string pdfDocEncoded;
    if (stringEvalued)
    {
        auto charset = getCharSet(view);
        switch (charset)
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

    utls::SerializeEncodedString(device, view, m_isHex,
        (writeFlags & PdfWriteFlags::SkipDelimiters) != PdfWriteFlags::None);
}

PdfStringCharset PdfString::GetCharset() const
{
    if (m_dataAllocated)
    {
        ensureCharsEvaluated();
        return getCharSet(m_data->Chars);
    }
    else
    {
        return getCharSet(m_Utf8View);
    }
}

string_view PdfString::GetString() const
{
    if (m_dataAllocated)
    {
        ensureCharsEvaluated();
        return m_data->Chars;
    }
    else
    {
        return m_Utf8View;
    }
}

bool PdfString::IsEmpty() const
{
    if (m_dataAllocated)
        return m_data->Chars.empty();
    else
        return m_Utf8View.empty();
}

bool PdfString::IsStringEvaluated() const
{
    if (m_dataAllocated)
        return m_data->StringEvaluated;
    else
        return true;
}

bool PdfString::operator==(const PdfString& rhs) const
{
    if (this->m_dataAllocated)
    {
        if (rhs.m_dataAllocated)
        {
            if (this->m_data == rhs.m_data)
                return true;

            if (this->m_data->StringEvaluated != this->m_data->StringEvaluated)
                return false;

            return this->m_data->Chars == rhs.m_data->Chars;
        }
        else
        {
            if (!this->m_data->StringEvaluated)
                return false;

            return this->m_data->Chars == rhs.m_Utf8View;
        }
    }
    else
    {
        if (rhs.m_dataAllocated)
        {
            if (!rhs.m_data->StringEvaluated)
                return false;

            return this->m_Utf8View == rhs.m_data->Chars;
        }
        else
        {
            return this->m_Utf8View == rhs.m_Utf8View;
        }
    }
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
    if (m_dataAllocated)
    {
        ensureCharsEvaluated();
        return m_data->Chars == view;
    }
    else
    {
        return m_Utf8View == view;
    }
}

bool PdfString::operator!=(const PdfString& rhs) const
{
    if (this->m_dataAllocated)
    {
        if (rhs.m_dataAllocated)
        {
            if (this->m_data != rhs.m_data)
                return true;

            if (this->m_data->StringEvaluated != this->m_data->StringEvaluated)
                return true;

            return this->m_data->Chars != rhs.m_data->Chars;
        }
        else
        {
            if (!this->m_data->StringEvaluated)
                return true;

            return this->m_data->Chars != rhs.m_Utf8View;
        }
    }
    else
    {
        if (rhs.m_dataAllocated)
        {
            if (!rhs.m_data->StringEvaluated)
                return true;

            return this->m_Utf8View != rhs.m_data->Chars;
        }
        else
        {
            return this->m_Utf8View != rhs.m_Utf8View;
        }
    }
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
    if (m_dataAllocated)
    {
        ensureCharsEvaluated();
        return m_data->Chars != view;
    }
    else
    {
        return m_Utf8View != view;
    }
}

PdfString::operator string_view() const
{
    if (m_dataAllocated)
    {
        ensureCharsEvaluated();
        return m_data->Chars;
    }
    else
    {
        return m_Utf8View;
    }
}

void PdfString::initFromUtf8String(const char* str, size_t length, bool literal)
{
    if (str == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "String is null");

    if (literal)
    {
        new(&m_Utf8View)string_view(str, length);
        m_dataAllocated = false;
    }
    else
    {
        if (length == 0)
        {
            // Avoid copying an empty string
            new(&m_Utf8View)string_view("");
            m_dataAllocated = false;
        }
        else
        {
            new(&m_data)shared_ptr<StringData>(new StringData(charbuff(str, length), true));
            m_dataAllocated = true;
        }
    }
}

void PdfString::ensureCharsEvaluated() const
{
    PODOFO_INVARIANT(m_dataAllocated);
    if (m_data->StringEvaluated)
        return;

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
            break;
        }
        case StringEncoding::utf16le:
        {
            // Remove BOM and decode utf-16 string
            string utf8;
            auto view = string_view(m_data->Chars).substr(2);
            utls::ReadUtf16LEString(view, utf8);
            utf8.swap(m_data->Chars);
            break;
        }
        case StringEncoding::utf8:
        {
            // Remove BOM
            m_data->Chars.substr(3).swap(m_data->Chars);
            break;
        }
        case StringEncoding::PdfDocEncoding:
        {
            bool isAsciiEqual;
            auto utf8 = PoDoFo::ConvertPdfDocEncodingToUTF8(m_data->Chars, isAsciiEqual);
            utf8.swap(m_data->Chars);
            break;
        }
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Unsupported");
    }

    m_data->StringEvaluated = true;
}

void PdfString::moveFrom(PdfString&& rhs)
{
    if (rhs.m_dataAllocated)
        new(&m_data)shared_ptr<StringData>(std::move(rhs.m_data));
    else
        new(&m_Utf8View)string_view(rhs.m_Utf8View);

    m_dataAllocated = rhs.m_dataAllocated;
    m_isHex = rhs.m_isHex;

    new(&rhs.m_Utf8View)string_view("");
    rhs.m_dataAllocated = false;
    rhs.m_isHex = false;
}

string_view PdfString::GetRawData() const
{
    if (!m_dataAllocated || m_data->StringEvaluated)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "The raw data buffer has been evaluated to a string");

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

PdfStringCharset getCharSet(const string_view& view)
{
    bool isAsciiEqual;
    if (PoDoFo::CheckValidUTF8ToPdfDocEcondingChars(view, isAsciiEqual))
        return isAsciiEqual ? PdfStringCharset::Ascii : PdfStringCharset::PdfDocEncoding;
    else
        return PdfStringCharset::Unicode;
}

// Empty string constructor
PdfString::StringData::StringData(charbuff&& buff, bool stringEvaluated)
    : Chars(std::move(buff)), StringEvaluated(stringEvaluated) { }
