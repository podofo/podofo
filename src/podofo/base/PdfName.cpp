/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfName.h"

#include <podofo/private/PdfEncodingPrivate.h>

#include "PdfOutputDevice.h"
#include "PdfTokenizer.h"
#include "PdfPredefinedEncoding.h"

using namespace std;
using namespace PoDoFo;

template<typename T>
void hexchr(const unsigned char ch, T& it);

static void EscapeNameTo(string& dst, const string_view& view);
static string UnescapeName(const string_view& view);

const PdfName PdfName::KeyNull = PdfName();
const PdfName PdfName::KeyContents = PdfName("Contents");
const PdfName PdfName::KeyFlags = PdfName("Flags");
const PdfName PdfName::KeyLength = PdfName("Length");
const PdfName PdfName::KeyRect = PdfName("Rect");
const PdfName PdfName::KeySize = PdfName("Size");
const PdfName PdfName::KeySubtype = PdfName("Subtype");
const PdfName PdfName::KeyType = PdfName("Type");
const PdfName PdfName::KeyFilter = PdfName("Filter");

PdfName::PdfName()
    : m_data(new NameData{ true, { }, nullptr })
{
}

PdfName::PdfName(const char* str)
{
    initFromUtf8String(string_view(str, std::strlen(str)));
}

PdfName::PdfName(const string& str)
{
    initFromUtf8String(str);
}

PdfName::PdfName(const string_view& view)
{
    initFromUtf8String(view);
}

PdfName::PdfName(const PdfName& rhs)
    : m_data(rhs.m_data)
{
}

PdfName::PdfName(charbuff&& buff)
    : m_data(new NameData{ false, std::move(buff), nullptr })
{
}

void PdfName::initFromUtf8String(const string_view& view)
{
    if (view.data() == nullptr)
        throw runtime_error("Name is null");

    if (view.length() == 0)
    {
        m_data.reset(new NameData{ true, { }, nullptr });
        return;
    }

    bool isAsciiEqual;
    if (!PoDoFo::CheckValidUTF8ToPdfDocEcondingChars(view, isAsciiEqual))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidName, "Characters in string must be PdfDocEncoding character set");

    if (isAsciiEqual)
        m_data.reset(new NameData{ true, charbuff(view), nullptr });
    else
        m_data.reset(new NameData{ true, (charbuff)PoDoFo::ConvertUTF8ToPdfDocEncoding(view), std::make_unique<string>(view) });
}

PdfName PdfName::FromEscaped(const string_view& view)
{
    return FromRaw(UnescapeName(view));
}

PdfName PdfName::FromRaw(const bufferview& rawcontent)
{
    return PdfName((charbuff)rawcontent);
}

void PdfName::Write(OutputStreamDevice& device, PdfWriteFlags,
    const PdfStatefulEncrypt& encrypt, charbuff& buffer) const
{
    (void)encrypt;
    // Allow empty names, which are legal according to the PDF specification
    device.Write('/');
    if (m_data->Chars.size() != 0)
    {
        EscapeNameTo(buffer, m_data->Chars);
        device.Write(buffer);
    }
}

string PdfName::GetEscapedName() const
{
    if (m_data->Chars.size() == 0)
        return string();

    string ret;
    EscapeNameTo(ret, m_data->Chars);
    return ret;
}

void PdfName::expandUtf8String() const
{
    if (!m_data->IsUtf8Expanded)
    {
        bool isAsciiEqual;
        string utf8str;
        PoDoFo::ConvertPdfDocEncodingToUTF8(m_data->Chars, utf8str, isAsciiEqual);
        if (!isAsciiEqual)
            m_data->Utf8String.reset(new string(std::move(utf8str)));

        m_data->IsUtf8Expanded = true;
    }
}

/** Escape the input string according to the PDF name
 *  escaping rules and return the result.
 *
 *  \param it Iterator referring to the start of the input string
 *            ( eg a `const char *' or a `std::string::iterator' )
 *  \param length Length of input string
 *  \returns Escaped string
 */
void EscapeNameTo(string& dst, const string_view& view)
{
    // Scan the input string once to find out how much memory we need
    // to reserve for the encoded result string. We could do this in one
    // pass using a ostringstream instead, but it's a LOT slower.
    size_t outchars = 0;
    for (size_t i = 0; i < view.length(); i++)
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
            outchars += (PdfTokenizer::IsRegular(ch) &&
                PdfTokenizer::IsPrintable(ch) && (ch != '#')) ? 1 : 3;
        }
    }
    // Reserve it. We can't use reserve() because the GNU STL doesn't seem to
    // do it correctly; the memory never seems to get allocated.
    dst.resize(outchars);
    // and generate the encoded string
    string::iterator bufIt(dst.begin());
    for (size_t i = 0; i < view.length(); i++)
    {
        char ch = view[i];
        if (PdfTokenizer::IsRegular(ch)
            && PdfTokenizer::IsPrintable(ch)
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
string UnescapeName(const string_view& view)
{
    // We know the decoded string can be AT MOST
    // the same length as the encoded one, so:
    string ret;
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

const string& PdfName::GetString() const
{
    expandUtf8String();
    if (m_data->Utf8String == nullptr)
        return m_data->Chars;
    else
        return *m_data->Utf8String;
}

bool PdfName::IsNull() const
{
    return m_data->Chars.empty();
}

const string& PdfName::GetRawData() const
{
    return m_data->Chars;
}

const PdfName& PdfName::operator=(const PdfName& rhs)
{
    m_data = rhs.m_data;
    return *this;
}

bool PdfName::operator==(const PdfName& rhs) const
{
    if (this->m_data == rhs.m_data)
        return true;

    return this->m_data->Chars == rhs.m_data->Chars;
}

bool PdfName::operator!=(const PdfName& rhs) const
{
    if (this->m_data == rhs.m_data)
        return false;

    return this->m_data->Chars != rhs.m_data->Chars;
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
    auto& str = GetString();
    return str == view;
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
    auto& str = GetString();
    return str != view;
}

bool PdfName::operator<(const PdfName& rhs) const
{
    return this->m_data->Chars < rhs.m_data->Chars;
}

PdfName::operator string_view() const
{
    return m_data->Chars;
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
