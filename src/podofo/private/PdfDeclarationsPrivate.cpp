#include "PdfDeclarationsPrivate.h"
/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */
#include "PdfDeclarationsPrivate.h"

#include <regex>
#include <podofo/private/utfcpp_extensions.h>

#include <podofo/auxiliary/InputStream.h>
#include <podofo/auxiliary/OutputStream.h>

#include <podofo/private/istringviewstream.h>

#include <podofo/private/utfcpp_extensions.h>

#ifdef _WIN32
#include <podofo/private/WindowsLeanMean.h>
#else
 // NOTE: There's no <cstrings>, <strings.h> is a posix header
#include <strings.h>
#endif

#include "PdfTreeNode.h"

#include <podofo/main/PdfCommon.h>
#include <podofo/main/PdfElement.h>
#include <podofo/main/PdfDocument.h>

using namespace std;
using namespace PoDoFo;

constexpr unsigned BUFFER_SIZE = 4096;

thread_local unsigned s_recursionDepth = 0;

static const locale s_cachedLocale("C");

extern PODOFO_IMPORT PdfLogSeverity s_MaxLogSeverity;
extern PODOFO_IMPORT unsigned s_MaxRecursionDepth;
extern PODOFO_IMPORT LogMessageCallback s_LogMessageCallback;

static char getEscapedCharacter(char ch);
static void removeTrailingZeroes(string& str, size_t len);
static bool isStringDelimter(char32_t ch);
static void extractFontHints(string& fontName, bool& isItalic, bool& isBold);
static bool trimSuffix(string& name, const string_view& suffix);
static double modulo(double a, double b);

// Picked as the minimum size for small string optimizations withing GCC, MSVC, Clang
constexpr unsigned FloatFormatDefaultSize = 15;

struct VersionIdentity
{
    PdfName Name;
    PdfVersion Version;
};

static VersionIdentity s_PdfVersions[] = {
    { "1.0"_n, PdfVersion::V1_0 },
    { "1.1"_n, PdfVersion::V1_1 },
    { "1.2"_n, PdfVersion::V1_2 },
    { "1.3"_n, PdfVersion::V1_3 },
    { "1.4"_n, PdfVersion::V1_4 },
    { "1.5"_n, PdfVersion::V1_5 },
    { "1.6"_n, PdfVersion::V1_6 },
    { "1.7"_n, PdfVersion::V1_7 },
    { "2.0"_n, PdfVersion::V2_0 },
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

const PdfName& PoDoFo::GetPdfVersionName(PdfVersion version)
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

bool PoDoFo::IsAccessibiltyProfile(PdfALevel pdfaLevel)
{
    switch (pdfaLevel)
    {
        case PdfALevel::L1A:
        case PdfALevel::L2A:
        case PdfALevel::L3A:
            return true;
        default:
            return false;
    }
}

PdfFilterType PoDoFo::NameToFilter(const string_view& name, bool lenient)
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
    else if (lenient)
    {
        // "Acrobat viewers accept the abbreviated filter names shown in table titled
        // 'Abbreviations for standard filter names' in addition to the standard ones
        // These abbreviated names are intended for use only in the context of inline images
        // (see Section 4.8.6, 'Inline Images'), they should not be used as filter names
        // in any stream object.
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
    }

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

string PoDoFo::ExtractBaseFontName(const string_view& fontName, bool skipTrimSubset)
{
    bool isItalic;
    bool isBold;
    if (skipTrimSubset)
    {
        string name(fontName);
        extractFontHints(name, isItalic, isBold);
        return name;
    }
    else
    {
        string name(fontName.substr(PoDoFo::GetSubsetPrefixLength(fontName)));
        extractFontHints(name, isItalic, isBold);
        return name;
    }
}

string PoDoFo::ExtractFontHints(const string_view& fontName, bool& isItalic, bool& isBold)
{
    string name(fontName);
    extractFontHints(name, isItalic, isBold);
    return name;
}

char PoDoFo::XRefEntryTypeToChar(PdfXRefEntryType type)
{
    switch (type)
    {
        case PdfXRefEntryType::Free:
            return 'f';
        case PdfXRefEntryType::InUse:
            return 'n';
        case PdfXRefEntryType::Unknown:
        case PdfXRefEntryType::Compressed:
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}

PdfXRefEntryType PoDoFo::XRefEntryTypeFromChar(char c)
{
    switch (c)
    {
        case 'f':
            return PdfXRefEntryType::Free;
        case 'n':
            return PdfXRefEntryType::InUse;
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidXRef);
    }
}

int PoDoFo::GetOperandCount(PdfOperator op)
{
    int count;
    if (!TryGetOperandCount(op, count))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidEnumValue, "Invalid operator");

    return count;
}

bool PoDoFo::TryGetOperandCount(PdfOperator op, int& count)
{
    switch (op)
    {
        case PdfOperator::w:
            count = 1;
            return true;
        case PdfOperator::J:
            count = 1;
            return true;
        case PdfOperator::j:
            count = 1;
            return true;
        case PdfOperator::M:
            count = 1;
            return true;
        case PdfOperator::d:
            count = 2;
            return true;
        case PdfOperator::ri:
            count = 1;
            return true;
        case PdfOperator::i:
            count = 1;
            return true;
        case PdfOperator::gs:
            count = 1;
            return true;
        case PdfOperator::q:
            count = 0;
            return true;
        case PdfOperator::Q:
            count = 0;
            return true;
        case PdfOperator::cm:
            count = 6;
            return true;
        case PdfOperator::m:
            count = 2;
            return true;
        case PdfOperator::l:
            count = 2;
            return true;
        case PdfOperator::c:
            count = 6;
            return true;
        case PdfOperator::v:
            count = 4;
            return true;
        case PdfOperator::y:
            count = 4;
            return true;
        case PdfOperator::h:
            count = 0;
            return true;
        case PdfOperator::re:
            count = 4;
            return true;
        case PdfOperator::S:
            count = 0;
            return true;
        case PdfOperator::s:
            count = 0;
            return true;
        case PdfOperator::f:
            count = 0;
            return true;
        case PdfOperator::F:
            count = 0;
            return true;
        case PdfOperator::f_Star:
            count = 0;
            return true;
        case PdfOperator::B:
            count = 0;
            return true;
        case PdfOperator::B_Star:
            count = 0;
            return true;
        case PdfOperator::b:
            count = 0;
            return true;
        case PdfOperator::b_Star:
            count = 0;
            return true;
        case PdfOperator::n:
            count = 0;
            return true;
        case PdfOperator::W:
            count = 0;
            return true;
        case PdfOperator::W_Star:
            count = 0;
            return true;
        case PdfOperator::BT:
            count = 0;
            return true;
        case PdfOperator::ET:
            count = 0;
            return true;
        case PdfOperator::Tc:
            count = 1;
            return true;
        case PdfOperator::Tw:
            count = 1;
            return true;
        case PdfOperator::Tz:
            count = 1;
            return true;
        case PdfOperator::TL:
            count = 1;
            return true;
        case PdfOperator::Tf:
            count = 2;
            return true;
        case PdfOperator::Tr:
            count = 1;
            return true;
        case PdfOperator::Ts:
            count = 1;
            return true;
        case PdfOperator::Td:
            count = 2;
            return true;
        case PdfOperator::TD:
            count = 2;
            return true;
        case PdfOperator::Tm:
            count = 6;
            return true;
        case PdfOperator::T_Star:
            count = 0;
            return true;
        case PdfOperator::Tj:
            count = 1;
            return true;
        case PdfOperator::TJ:
            count = 1;
            return true;
        case PdfOperator::Quote:
            count = 1;
            return true;
        case PdfOperator::DoubleQuote:
            count = 3;
            return true;
        case PdfOperator::d0:
            count = 2;
            return true;
        case PdfOperator::d1:
            count = 6;
            return true;
        case PdfOperator::CS:
            count = 1;
            return true;
        case PdfOperator::cs:
            count = 1;
            return true;
        case PdfOperator::SC:
            count = -1;
            return true;
        case PdfOperator::SCN:
            count = -1;
            return true;
        case PdfOperator::sc:
            count = -1;
            return true;
        case PdfOperator::scn:
            count = -1;
            return true;
        case PdfOperator::G:
            count = 1;
            return true;
        case PdfOperator::g:
            count = 1;
            return true;
        case PdfOperator::RG:
            count = 3;
            return true;
        case PdfOperator::rg:
            count = 3;
            return true;
        case PdfOperator::K:
            count = 4;
            return true;
        case PdfOperator::k:
            count = 4;
            return true;
        case PdfOperator::sh:
            count = 1;
            return true;
        case PdfOperator::BI:
            count = 0;
            return true;
        case PdfOperator::ID:
            count = 0;
            return true;
        case PdfOperator::EI:
            count = 0;
            return true;
        case PdfOperator::Do:
            count = 1;
            return true;
        case PdfOperator::MP:
            count = 1;
            return true;
        case PdfOperator::DP:
            count = 2;
            return true;
        case PdfOperator::BMC:
            count = 1;
            return true;
        case PdfOperator::BDC:
            count = 2;
            return true;
        case PdfOperator::EMC:
            count = 0;
            return true;
        case PdfOperator::BX:
            count = 0;
            return true;
        case PdfOperator::EX:
            count = 0;
            return true;
        default:
        case PdfOperator::Unknown:
            count = 0;
            return false;
    }
}

void PoDoFo::AddToCallStack(PdfError& err, string filepath, unsigned line, string information)
{
    err.AddToCallStack(std::move(filepath), line, std::move(information));
}

unsigned char PoDoFo::GetSubsetPrefixLength(const string_view& fontName)
{
    // NOTE: For some reasons, "^[A-Z]{6}\+" doesn't work
    regex regex = std::regex("^[A-Z][A-Z][A-Z][A-Z][A-Z][A-Z]\\+", regex_constants::ECMAScript);
    smatch matches;
    string name(fontName);
    if (std::regex_search(name, matches, regex))
    {
        // 5.5.3 Font Subsets: Remove EOODIA+ like prefixes
        return 7;
    }

    return 0;
}

// NOTE: This function is condsidered to be slow. Avoid calling it frequently
// https://github.com/podofo/podofo/issues/30
void extractFontHints(string& name, bool& isItalic, bool& isBold)
{
    // TABLE H.3 Names of standard fonts
    isItalic = false;
    isBold = false;

    if (trimSuffix(name, "BoldItalic"))
    {
        isBold = true;
        isItalic = true;
    }

    if (trimSuffix(name, "BoldOblique"))
    {
        isBold = true;
        isItalic = true;
    }

    if (trimSuffix(name, "Bold"))
    {
        isBold = true;
    }

    if (trimSuffix(name, "Italic"))
    {
        isItalic = true;
    }

    if (trimSuffix(name, "Oblique"))
    {
        isItalic = true;
    }

    if (trimSuffix(name, "Regular"))
    {
        // Nothing to set
    }

    // 5.5.2 TrueType Fonts: If the name contains any spaces, the spaces are removed
    name.erase(std::remove(name.begin(), name.end(), ' '), name.end());
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

void PoDoFo::CreateObjectStructElement(PdfDictionaryElement& elem, PdfPage& page, const PdfName& elementType)
{
    PdfDictionary* dict;
    auto structTreeObj = elem.GetDocument().GetCatalog().GetStructTreeRootObject();
    if (structTreeObj == nullptr || !structTreeObj->TryGetDictionary(dict))
        return;

    // Try to find a /Document struct element
    PdfDictionary* elemDict = nullptr;
    const PdfName* name;
    auto kArr = dict->FindKeyAsSafe<PdfArray*>("K");
    if (kArr == nullptr)
    {
        if (!dict->TryFindKeyAs("K", elemDict)
            || !elemDict->TryFindKeyAs("S", name)
            || *name != "Document")
        {
            return;
        }
    }
    else
    {
        bool foundDocumentObj = false;
        for (unsigned i = 0; i < kArr->GetSize(); i++)
        {
            if (kArr->TryFindAtAs(i, elemDict)
                && elemDict->TryFindKeyAs("S", name)
                && *name == "Document")
            {
                foundDocumentObj = true;
                break;
            }
        }

        if (!foundDocumentObj)
            return;
    }

    kArr = elemDict->FindKeyAsSafe<PdfArray*>("K");
    if (kArr == nullptr)
    {
        kArr = &elem.GetDocument().GetObjects().CreateArrayObject().GetArray();
        elemDict->AddKeyIndirect("K"_n, *kArr->GetOwner());
    }

    // Create a struct element for the field
    auto& fieldStructDict = elem.GetDocument().GetObjects().CreateDictionaryObject().GetDictionary();
    kArr->AddIndirect(*fieldStructDict.GetOwner());
    fieldStructDict.AddKey("S"_n, elementType);
    fieldStructDict.AddKeyIndirect("P"_n, *elemDict->GetOwner());
    elemDict = &fieldStructDict.AddKey("K", PdfDictionary()).GetDictionary();
    elemDict->AddKey("Type"_n, PdfName("OBJR"));
    elemDict->AddKeyIndirect("Pg"_n, page.GetObject());
    elemDict->AddKeyIndirect("Obj"_n, elem.GetObject());

    auto parentTreeDict = dict->FindKeyAsSafe<PdfDictionary*>("ParentTree");
    if (parentTreeDict == nullptr)
    {
        parentTreeDict = &elem.GetDocument().GetObjects().CreateDictionaryObject().GetDictionary();
        dict->AddKeyIndirect("ParentTree"_n, *parentTreeDict->GetOwner());
    }

    //  Determine the struct element key
    PdfNumberTreeNode node(nullptr, *parentTreeDict->GetOwner());
    int64_t structParentKey;
    auto last = node.GetLast();
    if (last == node.end())
        structParentKey = 0;
    else
        structParentKey = last->first + 1;

    // Set the struct element key id in the field and in the struct root tree
    node.AddValue(structParentKey, *fieldStructDict.GetOwner());
    elem.GetDictionary().AddKey("StructParent"_n, PdfObject(structParentKey));
}

const locale& utls::GetInvariantLocale()
{
    return s_cachedLocale;
}

string_view utls::GetEnvironmentVariable(const string_view& name)
{
    auto env = std::getenv(name.data());
    if (env == nullptr)
        return string_view();
    else
        return { env };
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

bool utls::IsSpaceLikeChar(char32_t ch)
{
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
        // Feed
        case U'\t':         // CHARACTER TABULATION U+0009
            return true;
        default:
            return false;
    }
}

bool utls::IsNewLineLikeChar(char32_t ch)
{
    switch (ch)
    {
        // Line separators
        case U'\x2028':     // LINE SEPARATOR
        // Paragraph separators
        case U'\x2029':     // PARAGRAPH SEPARATOR
        // Feed
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
}

// Trim the suffix from the name. Returns true if suffix is found
bool trimSuffix(string& name, const string_view& suffix)
{
    size_t found = name.find(suffix);
    if (found == string::npos)
        return false;

    // Try to find prefix including ',' or '-'
    char prevCh;
    size_t patternLenth = suffix.length();
    if (found > 0 && ((prevCh = name[found - 1]) == ',' || prevCh == '-'))
    {
        found--;
        patternLenth++;
    }

    name.erase(found, patternLenth);
    return true;
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
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::IOError, "Failed to read file size");
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

void utls::ReadTo(charbuff& str, const string_view& filepath, size_t maxReadSize)
{
    ifstream istream = utls::open_ifstream(filepath, ios_base::binary);
    ReadTo(str, istream, maxReadSize);
}

void utls::ReadTo(charbuff& str, istream& stream, size_t maxReadSize)
{
    stream.seekg(0, ios::end);
    auto tellg = stream.tellg();
    if (tellg == -1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidStream, "Error reading from stream");

    str.resize(std::min((size_t)tellg, maxReadSize));
    stream.seekg(0, ios::beg);
    stream.read(str.data(), str.size());
    if (stream.fail())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidStream, "Error reading from stream");
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
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::IOError, "Stream I/O error while reading");

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
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::IOError, "Stream I/O error while reading");

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

ssize_t utls::ftell(FILE* file)
{
#if defined(_WIN64)
    return _ftelli64(file);
#else
    return std::ftell(file);
#endif
}

ssize_t utls::fseek(FILE* file, ssize_t offset, int origin)
{
#if defined(_WIN64)
    return _fseeki64(file, offset, origin);
#else
    return std::fseek(file, offset, origin);
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

string utls::GetCharHexString(const bufferview& buff)
{
    string ret(buff.size() * 2, '\0');
    for (unsigned i = 0; i < buff.size(); i++)
        utls::WriteCharHexTo(ret.data() + i * 2, buff[i]);

    return ret;
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
    utf8::u16bechariterable iterable(buffer.data(), buffer.size(), true);
    utf8::utf16to8_lenient(iterable.begin(), iterable.end(), std::back_inserter(utf8str));
}

void utls::ReadUtf16LEString(const bufferview& buffer, string& utf8str)
{
    utf8::u16lechariterable iterable(buffer.data(), buffer.size(), true);
    utf8::utf16to8_lenient(iterable.begin(), iterable.end(), std::back_inserter(utf8str));
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
    // The default size should be large enough to format all
    // numbers with fixed notation. See https://stackoverflow.com/a/52045523/213871
    str.resize(FloatFormatDefaultSize);
    auto result = std::to_chars(str.data(), str.data() + FloatFormatDefaultSize, value, chars_format::fixed, precision);
    removeTrailingZeroes(str, result.ptr - str.data());
}

void utls::FormatTo(string& str, double value, unsigned short precision)
{
    str.resize(FloatFormatDefaultSize);
    auto result = std::to_chars(str.data(), str.data() + FloatFormatDefaultSize, value, chars_format::fixed, precision);
    if (result.ec == errc::value_too_large)
    {
        // See https://stackoverflow.com/a/52045523/213871
        // 24 recommended - 5 (unnecessary) exponent = 19
        constexpr unsigned DoubleFormatDefaultSize = 19;
        str.resize(DoubleFormatDefaultSize);
        result = std::to_chars(str.data(), str.data() + DoubleFormatDefaultSize, value, chars_format::fixed, precision);
    }
    removeTrailingZeroes(str, result.ptr - str.data());
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

void utls::Replace(string& str, const string_view& from, const string_view& to)
{
    size_t start_pos = str.find(from);
    if (start_pos == string_view::npos)
        return;

    str.replace(start_pos, from.length(), to);
    return;
}

void utls::ByteSwap(u16string& str)
{
    for (unsigned i = 0; i < str.length(); i++)
        str[i] = (char16_t)utls::ByteSwap((uint16_t)str[i]);
}

int utls::NormalizePageRotation(double angle)
{
    constexpr double ADOBE_EPSILON = 0.5;

    // Normalize the rotation between [0,360)
    double normalized = utls::NormalizeCircularRange(angle, 0, 360);

    // NOTE: Adobe Reader seems to go nuts here, looking in the
    // neighbourhood of orthogonal rotations. It's unclear if
    // the following computation can be made more efficient,
    // but the page rotation is cached anyway. See
    // BasicTest::TestNormalizeRangeRotations() for the complete
    // lists with tests of expected values verified in Adobe
    // products

    // Round the angle with [0, 90, 180, 270, 360] as possible values
    int rounded = (int)std::round(normalized / 90) * 90;
    switch (rounded)
    {
        case 0:
            if (normalized < ADOBE_EPSILON)
                return 0;
            else
                return 90;
        case 180:
            if (normalized < 180 - ADOBE_EPSILON)
                return 90;

            if (normalized < 180 + ADOBE_EPSILON)
                return 180;
            else
                return 270;
        case 90:
        case 270:
            return rounded;
        case 360:
            if (normalized >= 360 - ADOBE_EPSILON)
                return 0;
            else
                return 270;
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InternalLogic);
    }
}

double utls::NormalizeCircularRange(double value, double start, double end)
{
    PODOFO_ASSERT(start < end);

    // Slightly shorten the range
    double range = end - start - numeric_limits<double>::epsilon();
    return start + modulo(value - start, range);
}

// Returns the remainder of the division a / b, so that the result of the
// subtraction of the *remainder* from the *divisor* can be subtracted from
// the *dividend* so it will be *floor* of the actual division
double modulo(double a, double b)
{
    return std::fmod((std::fmod(a, b)) + b, b);
}

void utls::NormalizeCoordinates(double& x1, double& y1, double& x2, double& y2)
{
    double temp;
    if (x1 > x2)
    {
        temp = x1;
        x1 = x2;
        x2 = temp;
    }

    if (y1 > y2)
    {
        temp = y1;
        y1 = y2;
        y2 = temp;
    }
}

void utls::SerializeEncodedString(OutputStream& stream, const string_view& encoded, bool wantHex, bool skipDelimiters)
{
    if (!skipDelimiters)
        stream.Write(wantHex ? '<' : '(');

    if (encoded.size() > 0)
    {
        const char* cursor = encoded.data();
        size_t len = encoded.size();

        if (wantHex)
        {
            char ch;
            char data[2];
            while (len-- != 0)
            {
                ch = *cursor;
                utls::WriteCharHexTo(data, ch);
                stream.Write(string_view(data, 2));
                cursor++;
            }
        }
        else
        {
            char ch;
            while (len-- != 0)
            {
                ch = *cursor;
                char escaped = getEscapedCharacter(ch);
                if (escaped == '\0')
                {
                    stream.Write(ch);
                }
                else
                {
                    stream.Write('\\');
                    stream.Write(escaped);
                }

                cursor++;
            }
        }
    }

    if (!skipDelimiters)
        stream.Write(wantHex ? '>' : ')');
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
    LPWSTR str{ nullptr };
    DWORD strLength = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM
        | FORMAT_MESSAGE_IGNORE_INSERTS
        | FORMAT_MESSAGE_ALLOCATE_BUFFER,
        NULL, // (not used with FORMAT_MESSAGE_FROM_SYSTEM)
        rc,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPWSTR>(&str),
        0,
        NULL);

    if (strLength == 0)
        return string();

    // Assign buffer to smart pointer with custom deleter so that memory gets released
    // in case String's c'tor throws an exception.
    auto deleter = [](void* p) { ::LocalFree(p); };
    unique_ptr<WCHAR, decltype(deleter)> ptrBuffer(str, deleter);
    return utf8::utf16to8(u16string_view((char16_t*)str, strLength));
}

#endif // _WIN322

unsigned char utls::GetCharCodeSize(unsigned code)
{
    if (code==0) {
        return 1;
    }
    return (unsigned char)(std::log(code) / std::log(256)) + 1;
}

unsigned utls::GetCharCodeMaxValue(unsigned char codeSize)
{
    return (unsigned)(std::pow(2, codeSize * CHAR_BIT)) - 1;
}

unsigned utls::FSSUTFEncode(unsigned codePoint)
{
    if (codePoint <= 0x7F)
    {
        return codePoint & 0xFF;
    }
    if (codePoint <= 0x7FF)
    {
        return ((0xC0 | (codePoint >> 6)) << 8)
              | (0x80 | (codePoint & 0x3F));
    }
    if (codePoint <= 0xFFFF)
    {
        return ((0xE0 | (codePoint >> 12)) << 16)
             | ((0x80 | ((codePoint >> 6) & 0x3F)) << 8)
             |  (0x80 | (codePoint & 0x3F));
    }
    if (codePoint <= 0x10FFFF)
    {
        return ((0xF0 | (codePoint >> 18)) << 24)
             | ((0x80 | ((codePoint >> 12) & 0x3F)) << 16)
             | ((0x80 | ((codePoint >> 6) & 0x3F)) << 8)
             |  (0x80 | (codePoint & 0x3F));
    }


    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Code larger than maximum encodable 0x10FFFF");
}

vector<utls::FSSUTFRange> utls::GetFSSUTFRanges(unsigned char codeSize)
{
    // According to https://www.unicode.org/versions/corrigendum1.html
    // Table 3.1B. Legal UTF-8 Byte Sequences

    if (codeSize == 1)
        return vector<FSSUTFRange> { FSSUTFRange{ 0x00, 0x7F } };
    else if (codeSize == 2)
        return vector<FSSUTFRange> { FSSUTFRange{ 0xC280, 0xDFBF } };
    else if (codeSize == 3)
        return vector<FSSUTFRange> { FSSUTFRange{ 0xE0A080, 0xE0BFBF }, FSSUTFRange{ 0xE18080, 0xEFBFBF } };
    else if (codeSize == 4)
        return vector<FSSUTFRange> { FSSUTFRange{ 0xF0908080, 0xF0BFBFBF }, FSSUTFRange{ 0xF1808080, 0xF3BFBFBF }, FSSUTFRange{ 0xF4808080, 0xF48FBFBF } };

    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Code size larger than maximum supported 4");
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

void utls::WriteUInt24BE(OutputStream& output, PoDoFo::uint24_t value)
{
    char buf[3];
    WriteUInt24BE(buf, value);
    output.Write(buf, 3);
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
    std::memcpy(buf, &value, sizeof(value));
}

void utls::WriteInt32BE(char* buf, int32_t value)
{
    value = AS_BIG_ENDIAN(value);
    std::memcpy(buf, &value, sizeof(value));
}

void utls::WriteUInt24BE(char* buf, uint24_t value)
{
    value = AS_BIG_ENDIAN(value);
    std::memcpy(buf, &value, sizeof(value));
}

void utls::WriteUInt16BE(char* buf, uint16_t value)
{
    value = AS_BIG_ENDIAN(value);
    std::memcpy(buf, &value, sizeof(value));
}

void utls::WriteInt16BE(char* buf, int16_t value)
{
    value = AS_BIG_ENDIAN(value);
    std::memcpy(buf, &value, sizeof(value));
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

void utls::ReadUInt24BE(InputStream& input, uint24_t& value)
{
    char buf[3];
    input.Read(buf, 3);
    ReadUInt24BE(buf, value);
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
          (uint32_t)((uint8_t)buf[3] << 0)
        | (uint32_t)((uint8_t)buf[2] << 8)
        | (uint32_t)((uint8_t)buf[1] << 16)
        | (uint32_t)((uint8_t)buf[0] << 24);
}

void utls::ReadInt32BE(const char* buf, int32_t& value)
{
    value =
          (int32_t)((uint8_t)buf[3] << 0)
        | (int32_t)((uint8_t)buf[2] << 8)
        | (int32_t)((uint8_t)buf[1] << 16)
        | (int32_t)((int8_t)buf[0]  << 24);
}

void utls::ReadUInt24BE(const char* buf, uint24_t& value)
{
    uint32_t intValue =
          (int32_t)((uint8_t)buf[2] << 0)
        | (int32_t)((uint8_t)buf[1] << 8)
        | (int32_t)((uint8_t)buf[0] << 16);
    value = AS_BIG_ENDIAN((uint24_t)intValue);
}

void utls::ReadUInt16BE(const char* buf, uint16_t& value)
{
    value =
          (uint16_t)((uint8_t)buf[1] << 0)
        | (uint16_t)((uint8_t)buf[0] << 8);
}

void utls::ReadInt16BE(const char* buf, int16_t& value)
{
    value =
          (int16_t)((uint8_t)buf[1] << 0)
        | (int16_t)((int8_t)buf[0]  << 8);
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
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::MaxRecursionReached, "Stack overflow");
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

void removeTrailingZeroes(string& str, size_t len)
{
    // Remove trailing zeroes
    const char* cursor = str.data();
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

char getEscapedCharacter(char ch)
{
    switch (ch)
    {
        case '\n':           // Line feed (LF)
            return 'n';
        case '\r':           // Carriage return (CR)
            return 'r';
        case '\t':           // Horizontal tab (HT)
            return 't';
        case '\b':           // Backspace (BS)
            return 'b';
        case '\f':           // Form feed (FF)
            return 'f';
        case '(':
            return '(';
        case ')':
            return ')';
        case '\\':
            return '\\';
        default:
            return '\0';
    }
}

uint24_t::uint24_t()
    : value{ } { }

uint24_t::uint24_t(unsigned value)
{
#ifdef PODOFO_IS_LITTLE_ENDIAN
    this->value[0] = (value >> 0 ) & 0xFF;
    this->value[1] = (value >> 8 ) & 0xFF;
    this->value[2] = (value >> 16) & 0xFF;
#else // PODOFO_IS_LITTLE_ENDIAN
    this->value[0] = (value >> 16) & 0xFF;
    this->value[1] = (value >> 8 ) & 0xFF;
    this->value[2] = (value >> 0 ) & 0xFF;
#endif // PODOFO_IS_LITTLE_ENDIAN
}

uint24_t::operator unsigned() const
{
#ifdef PODOFO_IS_LITTLE_ENDIAN
    return (unsigned)(this->value[0] | this->value[1] << 8 | this->value[2] << 16);
#else // PODOFO_IS_LITTLE_ENDIAN
    return (unsigned)(this->value[2] | this->value[1] << 8 | this->value[0] << 16);
#endif // PODOFO_IS_LITTLE_ENDIAN
}
