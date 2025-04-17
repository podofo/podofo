/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfName.h"

#include <podofo/private/PdfEncodingPrivate.h>

#include <podofo/auxiliary/OutputDevice.h>
#include "PdfTokenizer.h"
#include "PdfPredefinedEncoding.h"

using namespace std;
using namespace PoDoFo;

template<typename T>
void hexchr(const unsigned char ch, T& it);

static void escapeNameTo(string& dst, bufferview view);
static charbuff unescapeName(string_view view);

const PdfName PdfName::Null = PdfName();

PdfName::PdfName()
    : PdfDataMember(PdfDataType::Name), m_dataAllocated(false), m_Utf8View() { }

PdfName::~PdfName()
{
    if (m_dataAllocated)
        m_data.~shared_ptr();
}

PdfName::PdfName(const string& str)
    : PdfDataMember(PdfDataType::Name)
{
    initFromUtf8String(str);
}

PdfName::PdfName(const string_view& view)
    : PdfDataMember(PdfDataType::Name)
{
    if (view.data() == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidName, "Name is null");

    initFromUtf8String(view);
}

PdfName::PdfName(charbuff&& buff)
    : PdfDataMember(PdfDataType::Name), m_dataAllocated(true), m_data(new NameData{ std::move(buff), nullptr, false })
{
}

// We expect the input to be a const string literal: we just set the data view
PdfName::PdfName(const char& str, size_t length)
    : PdfDataMember(PdfDataType::Name), m_dataAllocated(false), m_Utf8View(&str, length)
{
}

PdfName::PdfName(const PdfName& rhs)
    : PdfDataMember(PdfDataType::Name)
{
    if (rhs.m_dataAllocated)
    {
        new(&m_data)shared_ptr<NameData>(rhs.m_data);
        m_dataAllocated = true;
    }
    else
    {
        new(&m_data)string_view(rhs.m_Utf8View);
        m_dataAllocated = false;
    }
}

PdfName::PdfName(PdfName&& rhs) noexcept
    : PdfDataMember(PdfDataType::Name)
{
    moveFrom(std::move(rhs));
}

PdfName& PdfName::operator=(const PdfName& rhs)
{
    this->~PdfName();
    if (rhs.m_dataAllocated)
    {
        new(&m_data)shared_ptr<NameData>(rhs.m_data);
        m_dataAllocated = true;
    }
    else
    {
        new(&m_data)string_view(rhs.m_Utf8View);
        m_dataAllocated = false;
    }
    return *this;
}

PdfName& PdfName::operator=(PdfName&& rhs) noexcept
{
    this->~PdfName();
    moveFrom(std::move(rhs));
    return *this;
}

void PdfName::initFromUtf8String(const char* str, size_t length)
{
    if (str == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidName, "Name is null");

    initFromUtf8String(string_view(str, length));
}

void PdfName::initFromUtf8String(const string_view& view)
{
    if (view.length() == 0)
    {
        // We assume it will be the null name
        new(&m_Utf8View)string_view();
        m_dataAllocated = false;
        return;
    }

    bool isAsciiEqual;
    if (!PoDoFo::CheckValidUTF8ToPdfDocEcondingChars(view, isAsciiEqual))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidName, "Characters in string must be PdfDocEncoding character set");

    if (isAsciiEqual)
        new(&m_data)shared_ptr<NameData>(new NameData{ charbuff(view), nullptr, true });
    else
        new(&m_data)shared_ptr<NameData>(new NameData{ (charbuff)PoDoFo::ConvertUTF8ToPdfDocEncoding(view), std::make_unique<string>(view), true });

    m_dataAllocated = true;
}

void PdfName::moveFrom(PdfName&& rhs)
{
    if (rhs.m_dataAllocated)
        new(&m_data)shared_ptr<NameData>(std::move(rhs.m_data));
    else
        new(&m_Utf8View)string_view(rhs.m_Utf8View);

    m_dataAllocated = rhs.m_dataAllocated;

    new(&rhs.m_Utf8View)string_view("");
    rhs.m_dataAllocated = false;
}

PdfName PdfName::FromEscaped(const string_view& view)
{
    // Slightly optimize memory usage by checking
    // against some well known values
    if (view == "Filter"sv)
        return "Filter"_n;
    else if (view == "Length"sv)
        return "Length"_n;
    else if (view == "FlateDecode"sv)
        return "FlateDecode"_n;
    else if (view == "Type"sv)
        return "Type"_n;
    else if (view == "Subtype"sv)
        return "Subtype"_n;
    else if (view == "Parent"sv)
        return "Parent"_n;
    else
        return PdfName(unescapeName(view));
}

PdfName PdfName::FromRaw(const bufferview& rawcontent)
{
    return PdfName((charbuff)rawcontent);
}

void PdfName::Write(OutputStream& device, PdfWriteFlags,
    const PdfStatefulEncrypt* encrypt, charbuff& buffer) const
{
    (void)encrypt;
    // Allow empty names, which are legal according to the PDF specification
    device.Write('/');
    auto dataView = GetRawData();
    if (dataView.size() != 0)
    {
        escapeNameTo(buffer, dataView);
        device.Write(buffer);
    }
}

string PdfName::GetEscapedName() const
{
    auto dataView = GetRawData();
    if (dataView.size() == 0)
        return string();

    string ret;
    escapeNameTo(ret, dataView);
    return ret;
}

void PdfName::expandUtf8String()
{
    PODOFO_INVARIANT(m_dataAllocated);
    if (m_data->IsUtf8Expanded)
        return;

    bool isAsciiEqual;
    string utf8str;
    PoDoFo::ConvertPdfDocEncodingToUTF8(m_data->Chars, utf8str, isAsciiEqual);
    if (!isAsciiEqual)
        m_data->Utf8String.reset(new string(std::move(utf8str)));

    m_data->IsUtf8Expanded = true;
}

/** Escape the input string according to the PDF name
 *  escaping rules and return the result.
 *
 *  \param it Iterator referring to the start of the input string
 *            ( eg a `const char *' or a `std::string::iterator' )
 *  \param length Length of input string
 *  \returns Escaped string
 */
void escapeNameTo(string& dst, bufferview view)
{
    // Scan the input string once to find out how much memory we need
    // to reserve for the encoded result string. We could do this in one
    // pass using a ostringstream instead, but it's a LOT slower.
    size_t outchars = 0;
    for (size_t i = 0; i < view.size(); i++)
    {
        char ch = view[i];

        // Null chars are illegal in names, even escaped
        if (ch == '\0')
        {
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidName, "Null byte in PDF name is illegal");
        }
        else
        {
            // Leave room for either just the char, or a #xx escape of it.
            outchars += (PoDoFo::IsCharRegular(ch) &&
                PoDoFo::IsCharASCIIPrintable(ch) && (ch != '#')) ? 1 : 3;
        }
    }
    // Reserve it. We can't use reserve() because the GNU STL doesn't seem to
    // do it correctly; the memory never seems to get allocated.
    dst.resize(outchars);
    // and generate the encoded string
    string::iterator bufIt(dst.begin());
    for (size_t i = 0; i < view.size(); i++)
    {
        char ch = view[i];
        if (PoDoFo::IsCharRegular(ch)
            && PoDoFo::IsCharASCIIPrintable(ch)
            && ch != '#')
        {
            *(bufIt++) = ch;
        }
        else
        {
            *(bufIt++) = '#';
            hexchr(static_cast<unsigned char>(ch), bufIt);
        }
    }
}

/** Interpret the passed string as an escaped PDF name
 *  and return the unescaped form.
 *
 *  \param it Iterator referring to the start of the input string
 *            ( eg a `const char *' or a `std::string::iterator' )
 *  \param length Length of input string
 *  \returns Unescaped string
 */
charbuff unescapeName(string_view view)
{
    // We know the decoded string can be AT MOST
    // the same length as the encoded one, so:
    charbuff ret;
    ret.reserve(view.length());
    size_t incount = 0;
    const char* curr = view.data();
    while (incount++ < view.length())
    {
        if (*curr == '#' && incount + 1 < view.length())
        {
            unsigned char hi = static_cast<unsigned char>(*(++curr));
            incount++;
            unsigned char low = static_cast<unsigned char>(*(++curr));
            incount++;
            hi -= (hi < 'A' ? '0' : 'A' - 10);
            low -= (low < 'A' ? '0' : 'A' - 10);
            unsigned char codepoint = (hi << 4) | (low & 0x0F);
            ret.push_back((char)codepoint);
        }
        else
            ret.push_back(*curr);

        curr++;
    }

    return ret;
}

string_view PdfName::GetString() const
{
    if (m_dataAllocated)
    {
        const_cast<PdfName&>(*this).expandUtf8String();
        if (m_data->Utf8String == nullptr)
            return m_data->Chars;
        else
            return *m_data->Utf8String;
    }
    else
    {
        // This was name was constructed from a read-only string literal
        return m_Utf8View;
    }
}

bool PdfName::IsNull() const
{
    return !m_dataAllocated && m_Utf8View.data() == nullptr;
}

string_view PdfName::GetRawData() const
{
    if (m_dataAllocated)
        return m_data->Chars;
    else
        return m_Utf8View;
}

bool PdfName::operator==(const PdfName& rhs) const
{
    return this->GetRawData() == rhs.GetRawData();
}

bool PdfName::operator!=(const PdfName& rhs) const
{
    return this->GetRawData() != rhs.GetRawData();
}

bool PdfName::operator==(const char* str) const
{
    return operator==(string_view(str, std::strlen(str)));
}

bool PdfName::operator==(const string& str) const
{
    return operator==((string_view)str);
}

bool PdfName::operator==(const string_view& view) const
{
    return GetString() == view;
}

bool PdfName::operator!=(const char* str) const
{
    return operator!=(string_view(str, std::strlen(str)));
}

bool PdfName::operator!=(const string& str) const
{
    return operator!=((string_view)str);
}

bool PdfName::operator!=(const string_view& view) const
{
    return GetString() != view;
}

PdfName::operator string_view() const
{
    if (m_dataAllocated)
        return m_data->Chars;
    else
        return m_Utf8View;
}

/**
 * This function writes a hex encoded representation of the character
 * `ch' to `buf', advancing the iterator by two steps.
 *
 * \warning no buffer length checking is performed, so MAKE SURE
 *          you have enough room for the two characters that
 *          will be written to the buffer.
 *
 * \param ch The character to write a hex representation of
 * \param buf An iterator (eg a char* or std::string::iterator) to write the
 *            characters to.  Must support the postfix ++, operator=(char) and
 *            dereference operators.
 */
template<typename T>
void hexchr(const unsigned char ch, T& it)
{
    *(it++) = "0123456789ABCDEF"[ch / 16];
    *(it++) = "0123456789ABCDEF"[ch % 16];
}
