/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */
#include "PdfDeclarationsPrivate.h"

#include <utfcpp/utf8.h>
#include <podofo/private/charconv_compat.h>
#include <podofo/private/utfcpp_extensions.h>

#include <podofo/main/PdfInputStream.h>
#include <podofo/main/PdfOutputStream.h>

#include <podofo/private/istringviewstream.h>

#ifdef _WIN32
#include <podofo/private/WindowsLeanMean.h>
#else
 // NOTE: There's no <cstrings>, <strings.h> is a posix header
#include <strings.h>
#endif

using namespace std;
using namespace PoDoFo;

constexpr unsigned BUFFER_SIZE = 4096;

// Default stack sizes
// Windows: 1MB on x32, x64, ARM https://docs.microsoft.com/en-us/cpp/build/reference/stack-stack-allocations?view=msvc-160
// Windows IIS: 512 KB for 64-bit worker processes, 256 KB for 32-bit worker processes
// macOS: 8MB on main thread, 512KB on secondary threads https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/Multithreading/CreatingThreads/CreatingThreads.html
// iOS: 1MB on main thread, 512KB on secondary threads
// Modern Linux distros: usually 8MB on main and secondary threads (but setting ulimit RLIMIT_STACK to unlimited *reduces* the secondary stack size on most architectures: see https://man7.org/linux/man-pages/man3/pthread_create.3.html#NOTES )
// the amount allocated on stack for local variables and function parameters varies between x86 and x64
// in x86 pointers are 32-bit but all function parameters are on stack
// in x64 pointers are 64-bit but first 4 function params are passed in registers
// the biggest difference is between debug and non-debug stacks: a debug stack frame can be around 3x larger
// due to instrumentation like ASAN which put guard bytes around stack variables to detect buffer overflows
constexpr unsigned MaxRecursionDepthDefault = 256;

static unsigned s_MaxRecursionDepth = MaxRecursionDepthDefault;
thread_local unsigned s_recursionDepth = 0;

static const locale s_cachedLocale("C");

extern PODOFO_IMPORT PdfLogSeverity s_MaxLogSeverity;
extern PODOFO_IMPORT LogMessageCallback s_LogMessageCallback;

static void removeTrailingZeroes(string& str);
static bool isStringDelimter(char32_t ch);

struct VersionIdentity
{
    string_view Name;
    PdfVersion Version;
};

static VersionIdentity s_PdfVersions[] = {
    { "1.0", PdfVersion::V1_0 },
    { "1.1", PdfVersion::V1_1 },
    { "1.2", PdfVersion::V1_2 },
    { "1.3", PdfVersion::V1_3 },
    { "1.4", PdfVersion::V1_4 },
    { "1.5", PdfVersion::V1_5 },
    { "1.6", PdfVersion::V1_6 },
    { "1.7", PdfVersion::V1_7 },
    { "2.0", PdfVersion::V2_0 },
};

template<typename TInt, class = typename std::enable_if_t<std::is_integral_v<TInt>>>
void formatTo(string& str, TInt value)
{
    str.clear();
    array<char, numeric_limits<TInt>::digits10> arr;
    auto res = std::to_chars(arr.data(), arr.data() + arr.size(), value);
    str.append(arr.data(), res.ptr - arr.data());
}


void PoDoFo::LogMessage(PdfLogSeverity logSeverity, const string_view& msg)
{
    if (logSeverity > s_MaxLogSeverity)
        return;

    if (s_LogMessageCallback == nullptr)
    {
        string_view prefix;
        bool ouputstderr = false;
        switch (logSeverity)
        {
            case PdfLogSeverity::Error:
                prefix = "ERROR: ";
                ouputstderr = true;
                break;
            case PdfLogSeverity::Warning:
                prefix = "WARNING: ";
                ouputstderr = true;
                break;
            case PdfLogSeverity::Debug:
                prefix = "DEBUG: ";
                break;
            case PdfLogSeverity::Information:
                break;
            default:
                PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
        }

        ostream* stream;
        if (ouputstderr)
            stream = &cerr;
        else
            stream = &cout;

        if (!prefix.empty())
            *stream << prefix;

        *stream << msg << endl;
    }
    else
    {
        s_LogMessageCallback(logSeverity, msg);
    }
}

PdfVersion PoDoFo::GetPdfVersion(const string_view& str)
{
    for (unsigned i = 0; i < std::size(s_PdfVersions); i++)
    {
        auto& version = s_PdfVersions[i];
        if (version.Name == str)
            return version.Version;
    }

    return PdfVersion::Unknown;
}

string_view PoDoFo::GetPdfVersionName(PdfVersion version)
{
    switch (version)
    {
        case PdfVersion::V1_0:
            return s_PdfVersions[0].Name;
        case PdfVersion::V1_1:
            return s_PdfVersions[1].Name;
        case PdfVersion::V1_2:
            return s_PdfVersions[2].Name;
        case PdfVersion::V1_3:
            return s_PdfVersions[3].Name;
        case PdfVersion::V1_4:
            return s_PdfVersions[4].Name;
        case PdfVersion::V1_5:
            return s_PdfVersions[5].Name;
        case PdfVersion::V1_6:
            return s_PdfVersions[6].Name;
        case PdfVersion::V1_7:
            return s_PdfVersions[7].Name;
        case PdfVersion::V2_0:
            return s_PdfVersions[8].Name;
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}


string_view PoDoFo::AnnotationTypeToName(PdfAnnotationType type)
{
    switch (type)
    {
        case PdfAnnotationType::Text:
            return "Text"sv;
        case PdfAnnotationType::Link:
            return "Link"sv;
        case PdfAnnotationType::FreeText:
            return "FreeText"sv;
        case PdfAnnotationType::Line:
            return "Line"sv;
        case PdfAnnotationType::Square:
            return "Square"sv;
        case PdfAnnotationType::Circle:
            return "Circle"sv;
        case PdfAnnotationType::Polygon:
            return "Polygon"sv;
        case PdfAnnotationType::PolyLine:
            return "PolyLine"sv;
        case PdfAnnotationType::Highlight:
            return "Highlight"sv;
        case PdfAnnotationType::Underline:
            return "Underline"sv;
        case PdfAnnotationType::Squiggly:
            return "Squiggly"sv;
        case PdfAnnotationType::StrikeOut:
            return "StrikeOut"sv;
        case PdfAnnotationType::Stamp:
            return "Stamp"sv;
        case PdfAnnotationType::Caret:
            return "Caret"sv;
        case PdfAnnotationType::Ink:
            return "Ink"sv;
        case PdfAnnotationType::Popup:
            return "Popup"sv;
        case PdfAnnotationType::FileAttachement:
            return "FileAttachment"sv;
        case PdfAnnotationType::Sound:
            return "Sound"sv;
        case PdfAnnotationType::Movie:
            return "Movie"sv;
        case PdfAnnotationType::Widget:
            return "Widget"sv;
        case PdfAnnotationType::Screen:
            return "Screen"sv;
        case PdfAnnotationType::PrinterMark:
            return "PrinterMark"sv;
        case PdfAnnotationType::TrapNet:
            return "TrapNet"sv;
        case PdfAnnotationType::Watermark:
            return "Watermark"sv;
        case PdfAnnotationType::Model3D:
            return "3D"sv;
        case PdfAnnotationType::RichMedia:
            return "RichMedia"sv;
        case PdfAnnotationType::WebMedia:
            return "WebMedia"sv;
        case PdfAnnotationType::Redact:
            return "Redact"sv;
        case PdfAnnotationType::Projection:
            return "Projection"sv;
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}

PdfAnnotationType PoDoFo::NameToAnnotationType(const string_view& str)
{
    if (str == "Text"sv)
        return PdfAnnotationType::Text;
    else if (str == "Link"sv)
        return PdfAnnotationType::Link;
    else if (str == "FreeText"sv)
        return PdfAnnotationType::FreeText;
    else if (str == "Line"sv)
        return PdfAnnotationType::Line;
    else if (str == "Square"sv)
        return PdfAnnotationType::Square;
    else if (str == "Circle"sv)
        return PdfAnnotationType::Circle;
    else if (str == "Polygon"sv)
        return PdfAnnotationType::Polygon;
    else if (str == "PolyLine"sv)
        return PdfAnnotationType::PolyLine;
    else if (str == "Highlight"sv)
        return PdfAnnotationType::Highlight;
    else if (str == "Underline"sv)
        return PdfAnnotationType::Underline;
    else if (str == "Squiggly"sv)
        return PdfAnnotationType::Squiggly;
    else if (str == "StrikeOut"sv)
        return PdfAnnotationType::StrikeOut;
    else if (str == "Stamp"sv)
        return PdfAnnotationType::Stamp;
    else if (str == "Caret"sv)
        return PdfAnnotationType::Caret;
    else if (str == "Ink"sv)
        return PdfAnnotationType::Ink;
    else if (str == "Popup"sv)
        return PdfAnnotationType::Popup;
    else if (str == "FileAttachment"sv)
        return PdfAnnotationType::FileAttachement;
    else if (str == "Sound"sv)
        return PdfAnnotationType::Sound;
    else if (str == "Movie"sv)
        return PdfAnnotationType::Movie;
    else if (str == "Widget"sv)
        return PdfAnnotationType::Widget;
    else if (str == "Screen"sv)
        return PdfAnnotationType::Screen;
    else if (str == "PrinterMark"sv)
        return PdfAnnotationType::PrinterMark;
    else if (str == "TrapNet"sv)
        return PdfAnnotationType::TrapNet;
    else if (str == "Watermark"sv)
        return PdfAnnotationType::Watermark;
    else if (str == "3D"sv)
        return PdfAnnotationType::Model3D;
    else if (str == "RichMedia"sv)
        return PdfAnnotationType::RichMedia;
    else if (str == "WebMedia"sv)
        return PdfAnnotationType::WebMedia;
    else if (str == "Redact"sv)
        return PdfAnnotationType::Redact;
    else if (str == "Projection"sv)
        return PdfAnnotationType::Projection;
    else
        PODOFO_RAISE_ERROR(PdfErrorCode::InternalLogic);
}

PdfColorSpace PoDoFo::NameToColorSpaceRaw(const string_view& name)
{
    if (name == "DeviceGray")
        return PdfColorSpace::DeviceGray;
    else if (name == "DeviceRGB")
        return PdfColorSpace::DeviceRGB;
    else if (name == "DeviceCMYK")
        return PdfColorSpace::DeviceCMYK;
    else if (name == "CalGray")
        return PdfColorSpace::CalGray;
    else if (name == "Lab")
        return PdfColorSpace::Lab;
    else if (name == "ICCBased")
        return PdfColorSpace::ICCBased;
    else if (name == "Indexed")
        return PdfColorSpace::Indexed;
    else if (name == "Pattern")
        return PdfColorSpace::Pattern;
    else if (name == "Separation")
        return PdfColorSpace::Separation;
    else if (name == "DeviceN")
        return PdfColorSpace::DeviceN;
    else
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::CannotConvertColor, "Unsupported colorspace name: {}", name);
}

string_view PoDoFo::ColorSpaceToNameRaw(PdfColorSpace colorSpace)
{
    switch (colorSpace)
    {
        case PdfColorSpace::DeviceGray:
            return "DeviceGray"sv;
        case PdfColorSpace::DeviceRGB:
            return "DeviceRGB"sv;
        case PdfColorSpace::DeviceCMYK:
            return "DeviceCMYK"sv;
        case PdfColorSpace::CalGray:
            return "CalGray"sv;
        case PdfColorSpace::Lab:
            return "Lab"sv;
        case PdfColorSpace::ICCBased:
            return "ICCBased"sv;
        case PdfColorSpace::Indexed:
            return "Indexed"sv;
        case PdfColorSpace::Pattern:
            return "Pattern"sv;
        case PdfColorSpace::Separation:
            return "Separation"sv;
        case PdfColorSpace::DeviceN:
            return "DeviceN"sv;
        case PdfColorSpace::Unknown:
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}

PdfFilterType PoDoFo::NameToFilter(const string_view& name)
{
    if (name == "ASCIIHexDecode")
        return PdfFilterType::ASCIIHexDecode;
    else if (name == "ASCII85Decode")
        return PdfFilterType::ASCII85Decode;
    else if (name == "LZWDecode")
        return PdfFilterType::LZWDecode;
    else if (name == "FlateDecode")
        return PdfFilterType::FlateDecode;
    else if (name == "RunLengthDecode")
        return PdfFilterType::RunLengthDecode;
    else if (name == "CCITTFaxDecode")
        return PdfFilterType::CCITTFaxDecode;
    else if (name == "JBIG2Decode")
        return PdfFilterType::JBIG2Decode;
    else if (name == "DCTDecode")
        return PdfFilterType::DCTDecode;
    else if (name == "JPXDecode")
        return PdfFilterType::JPXDecode;
    else if (name == "Crypt")
        return PdfFilterType::Crypt;
    else
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedFilter, name);
}

PdfFilterType PoDoFo::NameToFilterShort(const string_view& name)
{
    if (name == "AHx")
        return PdfFilterType::ASCIIHexDecode;
    else if (name == "A85")
        return PdfFilterType::ASCII85Decode;
    else if (name == "LZW")
        return PdfFilterType::LZWDecode;
    else if (name == "Fl")
        return PdfFilterType::FlateDecode;
    else if (name == "RL")
        return PdfFilterType::RunLengthDecode;
    else if (name == "CCF")
        return PdfFilterType::CCITTFaxDecode;
    else if (name == "DCT")
        return PdfFilterType::DCTDecode;
    // No short names for JBIG2Decode, JPXDecode, Crypt
    else
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedFilter, name);
}

string_view PoDoFo::FilterToName(PdfFilterType filterType)
{
    switch (filterType)
    {
        case PdfFilterType::ASCIIHexDecode:
            return "ASCIIHexDecode";
        case PdfFilterType::ASCII85Decode:
            return "ASCII85Decode";
        case PdfFilterType::LZWDecode:
            return "LZWDecode";
        case PdfFilterType::FlateDecode:
            return "FlateDecode";
        case PdfFilterType::RunLengthDecode:
            return "RunLengthDecode";
        case PdfFilterType::CCITTFaxDecode:
            return "CCITTFaxDecode";
        case PdfFilterType::JBIG2Decode:
            return "JBIG2Decode";
        case PdfFilterType::DCTDecode:
            return "DCTDecode";
        case PdfFilterType::JPXDecode:
            return "JPXDecode";
        case PdfFilterType::Crypt:
            return "Crypt";
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}

string_view PoDoFo::FilterToNameShort(PdfFilterType filterType)
{
    switch (filterType)
    {
        case PdfFilterType::ASCIIHexDecode:
            return "AHx";
        case PdfFilterType::ASCII85Decode:
            return "A85";
        case PdfFilterType::LZWDecode:
            return "LZW";
        case PdfFilterType::FlateDecode:
            return "Fl";
        case PdfFilterType::RunLengthDecode:
            return "RL";
        case PdfFilterType::CCITTFaxDecode:
            return "CCF";
        case PdfFilterType::DCTDecode:
            return "DCT";
        // No short names for the following
        case PdfFilterType::JBIG2Decode:
        case PdfFilterType::JPXDecode:
        case PdfFilterType::Crypt:
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}

vector<string> PoDoFo::ToPdfKeywordsList(const string_view& str)
{
    vector<string> ret;
    auto it = str.begin();
    auto tokenStart = it;
    auto end = str.end();
    string token;
    while (it != end)
    {
        auto ch = (char32_t)utf8::next(it, end);
        switch (ch)
        {
            case U'\r':
            case U'\n':
            {
                token = string(tokenStart, it);
                if (token.length() != 0)
                    ret.push_back(std::move(token));
                tokenStart = it;
                break;
            }
        }
    }

    token = string(tokenStart, it);
    if (token.length() != 0)
        ret.push_back(std::move(token));

    return ret;
}

string PoDoFo::ToPdfKeywordsString(const cspan<string>& keywords)
{
    string ret;
    bool first = true;
    for (auto& keyword : keywords)
    {
        if (first)
            first = false;
        else
            ret.append("\r\n");

        ret.append(keyword);
    }

    return ret;
}

const locale& utls::GetInvariantLocale()
{
    return s_cachedLocale;
}

bool utls::IsValidUtf8String(const string_view& str)
{
    return utf8::is_valid(str);
}

bool utls::IsStringDelimiter(char32_t ch)
{
    return IsWhiteSpace(ch) || isStringDelimter(ch);
}

bool utls::IsWhiteSpace(char32_t ch)
{
    // Taken from https://docs.microsoft.com/en-us/dotnet/api/system.char.iswhitespace
    switch (ch)
    {
        // Space separators
        case U' ':          // SPACE U+0020
        case U'\x00A0':     // NO-BREAK SPACE
        case U'\x1680':     // OGHAM SPACE MARK
        case U'\x2000':     // EN QUAD
        case U'\x2001':     // EM QUAD
        case U'\x2002':     // EN SPACE
        case U'\x2003':     // EM SPACE
        case U'\x2004':     // THREE-PER-EM SPACE
        case U'\x2005':     // FOUR-PER-EM SPACE
        case U'\x2006':     // SIX-PER-EM SPACE
        case U'\x2007':     // FIGURE SPACE
        case U'\x2008':     // PUNCTUATION SPAC
        case U'\x2009':     // THIN SPACE
        case U'\x200A':     // HAIR SPACE
        case U'\x202F':     // NARROW NO-BREAK SPAC
        case U'\x205F':     // MEDIUM MATHEMATICAL SPACE
        case U'\x3000':     // IDEOGRAPHIC SPACE
        // Line separators
        case U'\x2028':     // LINE SEPARATOR
        // Paragraph separators
        case U'\x2029':     // PARAGRAPH SEPARATOR
        // Feed
        case U'\t':         // CHARACTER TABULATION U+0009
        case U'\n':         // LINE FEED U+000A
        case U'\v':         // LINE TABULATION U+000B
        case U'\f':         // FORM FEED U+000C
        case U'\r':         // CARRIAGE RETURN U+000D
        case U'\x0085':     // NEXT LINE
            return true;
        default:
            return false;
    }
}

bool isStringDelimter(char32_t ch)
{
    // TODO: More Unicode punctuation/delimiters
    // See ICU or Char.IsPunctuation https://github.com/dotnet/corert/blob/master/src/System.Private.CoreLib/shared/System/Char.cs
    switch (ch)
    {
        case U'!':
        case U'"':
        case U'#':
        case U'$':
        case U'%':
        case U'&':
        case U'\'':
        case U'(':
        case U')':
        case U'*':
        case U'+':
        case U',':
        case U'-':
        case U'.':
        case U'/':
        case U':':
        case U';':
        case U'<':
        case U'=':
        case U'>':
        case U'?':
        case U'@':
        case U'[':
        case U'\\':
        case U']':
        case U'^':
        case U'_':
        case U'`':
        case U'{':
        case U'|':
        case U'}':
        case U'~':
            return true;
        default:
            return false;
    }

    return false;
}

bool utls::IsStringEmptyOrWhiteSpace(const string_view& str)
{
    auto it = str.begin();
    auto end = str.end();
    while (it != end)
    {
        char32_t cp = utf8::next(it, end);
        if (!utls::IsWhiteSpace(cp))
            return false;
    }

    return true;
}

string utls::TrimSpacesEnd(const string_view& str)
{
    auto it = str.begin();
    auto end = str.end();
    auto prev = it;
    auto firstWhiteSpace = end;
    while (it != end)
    {
        char32_t cp = utf8::next(it, end);
        if (utls::IsWhiteSpace(cp))
        {
            if (firstWhiteSpace == end)
                firstWhiteSpace = prev;
        }
        else
        {
            firstWhiteSpace = end;
        }

        prev = it;
    }

    if (firstWhiteSpace == end)
        return (string)str;
    else
        return (string)str.substr(0, firstWhiteSpace - str.begin());
}

const char* utls::TypeNameForIndex(unsigned index, const char** types, unsigned len)
{
    return index < len ? types[index] : nullptr;
}

int utls::TypeNameToIndex(const char* type, const char** types, unsigned len, int unknownValue)
{
    if (type == nullptr)
        return unknownValue;

    for (unsigned i = 0; i < len; i++)
    {
        if (types[i] != nullptr && strcmp(type, types[i]) == 0)
            return i;
    }

    return unknownValue;
}

bool utls::TryGetHexValue(char ch, unsigned char& value)
{
    switch (ch)
    {
        case '0':
            value = 0x0;
            return true;
        case '1':
            value = 0x1;
            return true;
        case '2':
            value = 0x2;
            return true;
        case '3':
            value = 0x3;
            return true;
        case '4':
            value = 0x4;
            return true;
        case '5':
            value = 0x5;
            return true;
        case '6':
            value = 0x6;
            return true;
        case '7':
            value = 0x7;
            return true;
        case '8':
            value = 0x8;
            return true;
        case '9':
            value = 0x9;
            return true;
        case 'a':
        case 'A':
            value = 0xA;
            return true;
        case 'b':
        case 'B':
            value = 0xB;
            return true;
        case 'c':
        case 'C':
            value = 0xC;
            return true;
        case 'd':
        case 'D':
            value = 0xD;
            return true;
        case 'e':
        case 'E':
            value = 0xE;
            return true;
        case 'f':
        case 'F':
            value = 0xF;
            return true;
        default:
            value = 0x0;
            return false;
    }
}

size_t utls::FileSize(const string_view& filename)
{
    streampos fbegin;

    auto stream = utls::open_ifstream(filename, ios_base::in | ios_base::binary);
    if (stream.fail())
        goto Error;

    fbegin = stream.tellg();   // The file pointer is currently at the beginning
    if (stream.fail())
        goto Error;

    stream.seekg(0, ios::end);      // Place the file pointer at the end of file
    if (stream.fail())
        goto Error;

    return (size_t)(streamoff)(stream.tellg() - fbegin);
Error:
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation, "Failed to read file size");
}

void utls::CopyTo(ostream& dst, istream& src)
{
    if (src.eof())
        return;

    array<char, BUFFER_SIZE> buffer;
    bool eof;
    do
    {
        streamsize read = utls::ReadBuffer(src, buffer.data(), BUFFER_SIZE, eof);
        dst.write(buffer.data(), read);
    } while (!eof);
}

void utls::ReadTo(charbuff& str, const string_view& filepath)
{
    ifstream istream = utls::open_ifstream(filepath, ios_base::binary);
    ReadTo(str, istream);
}

void utls::ReadTo(charbuff& str, istream& stream)
{
    stream.seekg(0, ios::end);
    if (stream.tellg() == -1)
        throw runtime_error("Error reading from stream");

    str.reserve((size_t)stream.tellg());
    stream.seekg(0, ios::beg);

    str.assign((istreambuf_iterator<char>(stream)),
        istreambuf_iterator<char>());
}

void utls::WriteTo(const string_view& filepath, const bufferview& view)
{
    ofstream ostream = utls::open_ofstream(string(filepath), ios_base::binary);
    WriteTo(ostream, view);
}

void utls::WriteTo(ostream& stream, const bufferview& view)
{
    cmn::istringviewstream istream(view.data(), view.size());
    CopyTo(stream, istream);
}

// Read from stream an amount of bytes or less
// without setting failbit
// https://stackoverflow.com/a/22593639/213871
size_t utls::ReadBuffer(istream& stream, char* buffer, size_t size, bool& eof)
{
    PODOFO_ASSERT(!stream.eof());

    size_t read = 0;
    do
    {
        // This consistently fails on gcc (linux) 4.8.1 with failbit set on read
        // failure. This apparently never fails on VS2010 and VS2013 (Windows 7)
        read += (size_t)stream.rdbuf()->sgetn(buffer + read, (streamsize)(size - read));

        // This rarely sets failbit on VS2010 and VS2013 (Windows 7) on read
        // failure of the previous sgetn()
        (void)stream.rdstate();

        // On gcc (linux) 4.8.1 and VS2010/VS2013 (Windows 7) this consistently
        // sets eofbit when stream is EOF for the conseguences  of sgetn(). It
        // should also throw if exceptions are set, or return on the contrary,
        // and previous rdstate() restored a failbit on Windows. On Windows most
        // of the times it sets eofbit even on real read failure
        (void)stream.peek();

        if (stream.fail())
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation, "Stream I/O error while reading");

        eof = stream.eof();

        if (read == size)
            return read;

    } while (!eof);

    return read;
}

// See utls:Read(stream, buffer, count) above
bool utls::ReadChar(istream& stream, char& ch)
{
    PODOFO_ASSERT(!stream.eof());

    streamsize read;
    do
    {
        read = stream.rdbuf()->sgetn(&ch, 1);
        (void)stream.rdstate();
        (void)stream.peek();
        if (stream.fail())
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation, "Stream I/O error while reading");

        if (read == 1)
            return true;

    } while (!stream.eof());

    return false;
}

FILE* utls::fopen(const string_view& filename, const string_view& mode)
{
#ifdef _WIN32
    auto filename16 = utf8::utf8to16((string)filename);
    auto mode16 = utf8::utf8to16((string)mode);
    return _wfopen((wchar_t*)filename16.c_str(), (wchar_t*)mode16.c_str());
#else
    return std::fopen(filename.data(), mode.data());
#endif
}

ifstream utls::open_ifstream(const string_view& filename, ios_base::openmode mode)
{
#ifdef _WIN32
    auto filename16 = utf8::utf8to16((string)filename);
    return ifstream((wchar_t*)filename16.c_str(), mode);
#else
    return ifstream((string)filename, mode);
#endif
}

ofstream utls::open_ofstream(const string_view& filename, ios_base::openmode mode)
{
#ifdef _WIN32
    auto filename16 = utf8::utf8to16((string)filename);
    return ofstream((wchar_t*)filename16.c_str(), mode);
#else
    return ofstream((string)filename, mode);
#endif
}

fstream utls::open_fstream(const string_view& filename, ios_base::openmode mode)
{
#ifdef _WIN32
    auto filename16 = utf8::utf8to16((string)filename);
    return fstream((wchar_t*)filename16.c_str(), mode);
#else
    return fstream((string)filename, mode);
#endif
}

void utls::WriteCharHexTo(char buf[2], char ch)
{
    buf[0] = (ch & 0xF0) >> 4;
    buf[0] += (buf[0] > 9 ? 'A' - 10 : '0');

    buf[1] = (ch & 0x0F);
    buf[1] += (buf[1] > 9 ? 'A' - 10 : '0');
}

void utls::WriteUtf16BETo(u16string& str, char32_t codePoint)
{
    str.clear();
    utf8::unchecked::append16(codePoint, std::back_inserter(str));
#ifdef PODOFO_IS_LITTLE_ENDIAN
    ByteSwap(str);
#endif
}

void utls::ReadUtf16BEString(const bufferview& buffer, string& utf8str)
{
    utf8::u16bechariterable iterable(buffer.data(), buffer.size());
    utf8::utf16to8(iterable.begin(), iterable.end(), std::back_inserter(utf8str));
}

void utls::ReadUtf16LEString(const bufferview& buffer, string& utf8str)
{
    utf8::u16lechariterable iterable(buffer.data(), buffer.size());
    utf8::utf16to8(iterable.begin(), iterable.end(), std::back_inserter(utf8str));
}

void utls::FormatTo(string& str, signed char value)
{
    formatTo(str, value);
}

void utls::FormatTo(string& str, unsigned char value)
{
    formatTo(str, value);
}

void utls::FormatTo(string& str, short value)
{
    formatTo(str, value);
}

void utls::FormatTo(string& str, unsigned short value)
{
    formatTo(str, value);
}

void utls::FormatTo(string& str, int value)
{
    formatTo(str, value);
}

void utls::FormatTo(string& str, unsigned value)
{
    formatTo(str, value);
}

void utls::FormatTo(string& str, long value)
{
    formatTo(str, value);
}

void utls::FormatTo(string& str, unsigned long value)
{
    formatTo(str, value);
}

void utls::FormatTo(string& str, long long value)
{
    formatTo(str, value);
}

void utls::FormatTo(string& str, unsigned long long value)
{
    formatTo(str, value);
}

void utls::FormatTo(string& str, float value, unsigned short precision)
{
    utls::FormatTo(str, "{:.{}f}", value, precision);
    removeTrailingZeroes(str);
}

void utls::FormatTo(string& str, double value, unsigned short precision)
{
    utls::FormatTo(str, "{:.{}f}", value, precision);
    removeTrailingZeroes(str);
}

// NOTE: This is clearly limited, since it's supporting only ASCII
string utls::ToLower(const string_view& str)
{
    string ret = (string)str;
    std::transform(ret.begin(), ret.end(), ret.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return ret;
}

string utls::Trim(const string_view& str, char ch)
{
    string ret = (string)str;
    ret.erase(std::remove(ret.begin(), ret.end(), ch), ret.end());
    return ret;
}

void utls::ByteSwap(u16string& str)
{
    for (unsigned i = 0; i < str.length(); i++)
        str[i] = (char16_t)utls::ByteSwap((uint16_t)str[i]);
}

// TODO: Substitute this function using Chromium numerics,
// which is now included in the code (see 3rdparty/numerics)
// https://chromium.googlesource.com/chromium/src/base/+/master/numerics/
bool utls::DoesMultiplicationOverflow(size_t op1, size_t op2)
{
    // This overflow check is from OpenBSD reallocarray.c,
    // and is also used in GifLib 5.1.2 onwards.
    //
    // Very old versions of calloc() in NetBSD and OS X 10.4
    // just multiplied size*nmemb which can overflow size_t
    // and allocate much less memory than expected
    // e.g. 2*(SIZE_MAX/2+1) = 2 bytes.
    // The calloc() overflow is also present in GCC 3.1.1,
    // GNU Libc 2.2.5 and Visual C++ 6.
    // http://cert.uni-stuttgart.de/ticker/advisories/calloc.html
    //
    //  MUL_NO_OVERFLOW is sqrt(SIZE_MAX+1), as s1*s2 <= SIZE_MAX
    //  if both s1 < MUL_NO_OVERFLOW and s2 < MUL_NO_OVERFLOW

    constexpr size_t MUL_NO_OVERFLOW = ((size_t)1 << (sizeof(size_t) * 4));
    if ((op1 >= MUL_NO_OVERFLOW || op2 >= MUL_NO_OVERFLOW) &&
        op1 > 0 && SIZE_MAX / op1 < op2)
    {
        return true;
    }

    return false;
}

#ifdef _WIN32

string utls::GetWin32ErrorMessage(unsigned rc)
{
    LPWSTR psz{ nullptr };
    const DWORD cchMsg = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM
        | FORMAT_MESSAGE_IGNORE_INSERTS
        | FORMAT_MESSAGE_ALLOCATE_BUFFER,
        NULL, // (not used with FORMAT_MESSAGE_FROM_SYSTEM)
        rc,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPWSTR>(&psz),
        0,
        NULL);

    if (cchMsg == 0)
        return string();

    // Assign buffer to smart pointer with custom deleter so that memory gets released
    // in case String's c'tor throws an exception.
    auto deleter = [](void* p) { ::LocalFree(p); };
    unique_ptr<WCHAR, decltype(deleter)> ptrBuffer(psz, deleter);
    return utf8::utf16to8((char16_t*)psz);
}

#endif // _WIN322

unsigned char utls::GetCharCodeSize(unsigned code)
{
    return (unsigned char)(std::log(code) / std::log(256)) + 1;
}

unsigned utls::GetCharCodeMaxValue(unsigned char codeSize)
{
    return (unsigned)(std::pow(2, codeSize * CHAR_BIT)) - 1;
}

void utls::WriteUInt32BE(OutputStream& output, uint32_t value)
{
    char buf[4];
    WriteUInt32BE(buf, value);
    output.Write(buf, 4);
}

void utls::WriteInt32BE(OutputStream& output, int32_t value)
{
    char buf[4];
    WriteInt32BE(buf, value);
    output.Write(buf, 4);
}

void utls::WriteUInt16BE(OutputStream& output, uint16_t value)
{
    char buf[2];
    WriteUInt16BE(buf, value);
    output.Write(buf, 2);
}

void utls::WriteInt16BE(OutputStream& output, int16_t value)
{
    char buf[2];
    WriteInt16BE(buf, value);
    output.Write(buf, 2);
}

void utls::WriteUInt32BE(char* buf, uint32_t value)
{
    value = AS_BIG_ENDIAN(value);
    buf[0] = static_cast<char>((value >> 0 ) & 0xFF);
    buf[1] = static_cast<char>((value >> 8 ) & 0xFF);
    buf[2] = static_cast<char>((value >> 16) & 0xFF);
    buf[3] = static_cast<char>((value >> 24) & 0xFF);
}

void utls::WriteInt32BE(char* buf, int32_t value)
{
    value = AS_BIG_ENDIAN(value);
    buf[0] = static_cast<char>((value >> 0 ) & 0xFF);
    buf[1] = static_cast<char>((value >> 8 ) & 0xFF);
    buf[2] = static_cast<char>((value >> 16) & 0xFF);
    buf[3] = static_cast<char>((value >> 24) & 0xFF);
}

void utls::WriteUInt16BE(char* buf, uint16_t value)
{
    value = AS_BIG_ENDIAN(value);
    buf[0] = static_cast<char>((value >> 0) & 0xFF);
    buf[1] = static_cast<char>((value >> 8) & 0xFF);
}

void utls::WriteInt16BE(char* buf, int16_t value)
{
    value = AS_BIG_ENDIAN(value);
    buf[0] = static_cast<char>((value >> 0) & 0xFF);
    buf[1] = static_cast<char>((value >> 8) & 0xFF);
}

void utls::ReadUInt32BE(InputStream& input, uint32_t& value)
{
    char buf[4];
    input.Read(buf, 4);
    ReadUInt32BE(buf, value);
}

void utls::ReadInt32BE(InputStream& input, int32_t& value)
{
    char buf[4];
    input.Read(buf, 4);
    ReadInt32BE(buf, value);
}

void utls::ReadUInt16BE(InputStream& input, uint16_t& value)
{
    char buf[2];
    input.Read(buf, 2);
    ReadUInt16BE(buf, value);
}

void utls::ReadInt16BE(InputStream& input, int16_t& value)
{
    char buf[2];
    input.Read(buf, 2);
    ReadInt16BE(buf, value);
}

void utls::ReadUInt32BE(const char* buf, uint32_t& value)
{
    value =
          (uint32_t)(buf[0] & 0xFF) << 0
        | (uint32_t)(buf[1] & 0xFF) << 8
        | (uint32_t)(buf[2] & 0xFF) << 16
        | (uint32_t)(buf[3] & 0xFF) << 24;
    value = AS_BIG_ENDIAN(value);
}

void utls::ReadInt32BE(const char* buf, int32_t& value)
{
    value =
          (int32_t)(buf[0] & 0xFF) << 0
        | (int32_t)(buf[1] & 0xFF) << 8
        | (int32_t)(buf[2] & 0xFF) << 16
        | (int32_t)(buf[3] & 0xFF) << 24;
    value = AS_BIG_ENDIAN(value);
}

void utls::ReadUInt16BE(const char* buf, uint16_t& value)
{
    value =
          (uint16_t)(buf[0] & 0xFF) << 0
        | (uint16_t)(buf[1] & 0xFF) << 8;
    value = AS_BIG_ENDIAN(value);
}

void utls::ReadInt16BE(const char* buf, int16_t& value)
{
    value =
          (int16_t)(buf[0] & 0xFF) << 0
        | (int16_t)(buf[1] & 0xFF) << 8;
    value = AS_BIG_ENDIAN(value);
}

void removeTrailingZeroes(string& str)
{
    // Remove trailing zeroes
    const char* cursor = str.data();
    size_t len = str.size();
    while (cursor[len - 1] == '0')
        len--;

    if (cursor[len - 1] == '.')
        len--;

    if (len == 0)
    {
        str.resize(1);
        str[0] = '0';
    }
    else
    {
        str.resize(len);
    }
}

void utls::RecursionGuard::Enter()
{
    s_recursionDepth++;
    if (s_recursionDepth > s_MaxRecursionDepth)
    {
        // avoid stack overflow on documents that have circular cross references, loops
        // or very deeply nested structures, can happen with
        // /Prev entries in trailer and XRef streams (possible via a chain of entries with a loop)
        // /Kids entries that loop back to self or parent
        // deeply nested Dictionary or Array objects (possible with lots of [[[[[[[[]]]]]]]] brackets)
        // mutually recursive loops involving several objects are possible
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidXRef, "Stack overflow");
    }
}

void utls::RecursionGuard::Exit()
{
    s_recursionDepth--;
}


utls::RecursionGuard::RecursionGuard()
{
    Enter();
}

utls::RecursionGuard::~RecursionGuard()
{
    Exit();
}

void utls::RecursionGuard::SetMaxRecursionDepth(unsigned maxRecursionDepth)
{
    s_MaxRecursionDepth = maxRecursionDepth;
}

unsigned utls::RecursionGuard::GetMaxRecursionDepth()
{
    return s_MaxRecursionDepth;
}
