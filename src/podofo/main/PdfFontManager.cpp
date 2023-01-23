/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfFontManager.h"

#include <algorithm>

#if defined(_WIN32) && defined(PODOFO_HAVE_WIN32GDI)
#include <podofo/private/WindowsLeanMean.h>
#endif // defined(_WIN32) && defined(PODOFO_HAVE_WIN32GDI)

#include <podofo/private/FreetypePrivate.h>
#include FT_TRUETYPE_TABLES_H
#include <utfcpp/utf8.h>

#include "PdfDictionary.h"
#include "PdfInputDevice.h"
#include "PdfOutputDevice.h"
#include "PdfFont.h"
#include "PdfFontMetricsFreetype.h"
#include "PdfFontMetricsStandard14.h"
#include "PdfFontType1.h"

using namespace std;
using namespace PoDoFo;

#if defined(_WIN32) && defined(PODOFO_HAVE_WIN32GDI)

static std::unique_ptr<charbuff> getFontData(const LOGFONTW& inFont);
static bool getFontData(charbuff& buffer, HDC hdc, HFONT hf);
static void getFontDataTTC(charbuff& buffer, const charbuff& fileBuffer, const charbuff& ttcBuffer);

#endif // defined(_WIN32) && defined(PODOFO_HAVE_WIN32GDI)

static unique_ptr<charbuff> getFontDataFromFile(const string_view& filename, unsigned short faceIndex);
static unique_ptr<charbuff> getFontDataFromBuffer(const bufferview& buffer, unsigned short faceIndex);
static unique_ptr<charbuff> getFontData(FT_Face face);
static PdfFont* matchFont(const mspan<PdfFont*>& fonts, const string_view& fontName, const PdfFontSearchParams& params);
static PdfFont* matchFont(const mspan<PdfFont*>& fonts, const string_view& fontName,
    PdfFontMatchBehaviorFlags matchBehavior = PdfFontMatchBehaviorFlags::None);

#if defined(PODOFO_HAVE_FONTCONFIG)
shared_ptr<PdfFontConfigWrapper> PdfFontManager::m_fontConfig;
#endif

static constexpr unsigned SUBSET_PREFIX_LEN = 6;

PdfFontManager::PdfFontManager(PdfDocument& doc)
    : m_doc(&doc)
{
    m_currentPrefix = "AAAAAA+";
}

PdfFontManager::~PdfFontManager()
{
    this->Clear();
}

void PdfFontManager::Clear()
{
    m_importedFonts.clear();
    m_fonts.clear();
}

PdfFont* PdfFontManager::AddImported(unique_ptr<PdfFont>&& font)
{
    Descriptor descriptor(font->GetMetrics().GetBaseFontName(),
        PdfStandard14FontType::Unknown,
        font->GetEncoding(),
        font->GetMetrics().GetStyle());
    return addImported(m_importedFonts[descriptor], std::move(font));
}

string PdfFontManager::GenerateSubsetPrefix()
{
    for (unsigned i = 0; i < SUBSET_PREFIX_LEN; i++)
    {
        m_currentPrefix[i]++;
        if (m_currentPrefix[i] <= 'Z')
            break;

        m_currentPrefix[i] = 'A';
    }

    return m_currentPrefix;
}

PdfFont* PdfFontManager::addImported(vector<PdfFont*>& fonts, unique_ptr<PdfFont>&& font)
{
    auto fontPtr = font.get();
    fonts.push_back(fontPtr);
    auto inserted = m_fonts.insert({ fontPtr->GetObject().GetIndirectReference(), Storage{ false, std::move(font) }});
    return inserted.first->second.Font.get();
}

PdfFont* PdfFontManager::GetLoadedFont(const PdfObject& obj)
{
    // TODO: We should check that the font being loaded
    // is not present in the imported font cache
    if (!obj.IsIndirect())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Object is not indirect");

    auto found = m_fonts.find(obj.GetIndirectReference());
    if (found != m_fonts.end())
    {
        if (!found->second.IsLoaded)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidFontData, "Invalid imported font queried");

        return found->second.Font.get();
    }

    // Create a new font
    unique_ptr<PdfFont> font;
    if (!PdfFont::TryCreateFromObject(const_cast<PdfObject&>(obj), font))
        return nullptr;

    auto inserted = m_fonts.emplace(obj.GetIndirectReference(), Storage{ true, std::move(font) });
    return inserted.first->second.Font.get();
}

PdfFont* PdfFontManager::SearchFont(const string_view& fontPattern, const PdfFontCreateParams& createParams)
{
    return SearchFont(fontPattern, PdfFontSearchParams(), createParams);
}

PdfFont* PdfFontManager::SearchFont(const string_view& fontPattern, const PdfFontSearchParams& searchParams, const PdfFontCreateParams& createParams)
{
    // NOTE: We don't support standard 14 fonts on subset
    PdfStandard14FontType stdFont;
    if (searchParams.AutoSelect != PdfFontAutoSelectBehavior::None
        && PdfFont::IsStandard14Font(fontPattern,
            searchParams.AutoSelect == PdfFontAutoSelectBehavior::Standard14Alt, stdFont))
    {
        return &GetStandard14Font(stdFont, createParams);
    }

    PdfFontSearchParams newSearchParams = searchParams;
    return getImportedFont(fontPattern, adaptSearchParams(fontPattern, newSearchParams), newSearchParams, createParams);
}

PdfFont& PdfFontManager::GetStandard14Font(PdfStandard14FontType stdFont, const PdfFontCreateParams& params)
{
    // Create a special descriptor cache that just specify the standard type and encoding
    Descriptor descriptor(
        { },
        stdFont,
        params.Encoding,
        PdfFontStyle::Regular);
    auto& fonts = m_importedFonts[descriptor];
    if (fonts.size() != 0)
    {
        PODOFO_ASSERT(fonts.size() == 1);
        return *fonts[0];
    }

    auto font = PdfFont::CreateStandard14(*m_doc, stdFont, params);
    return *addImported(fonts, std::move(font));
}

PdfFont& PdfFontManager::GetOrCreateFont(const string_view& fontPath, const PdfFontCreateParams& params)
{
    return GetOrCreateFont(fontPath, 0, params);
}

PdfFont& PdfFontManager::GetOrCreateFont(const string_view& fontPath, unsigned faceIndex, const PdfFontCreateParams& params)
{
    shared_ptr<charbuff> data = getFontDataFromFile(fontPath, faceIndex);
    if (data == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidFontData, "Could not parse a valid font from path {}", fontPath);

    shared_ptr<PdfFontMetrics> metrics = PdfFontMetricsFreetype::FromBuffer(data);
    metrics->SetFilePath(string(fontPath), faceIndex);
    return getOrCreateFontHashed(metrics, params);
}

PdfFont& PdfFontManager::GetOrCreateFontFromBuffer(const bufferview& buffer, const PdfFontCreateParams& createParams)
{
    return GetOrCreateFontFromBuffer(buffer, 0, createParams);
}

PdfFont& PdfFontManager::GetOrCreateFontFromBuffer(const bufferview& buffer, unsigned faceIndex, const PdfFontCreateParams& params)
{
    shared_ptr<charbuff> data = getFontDataFromBuffer(buffer, faceIndex);
    if (data == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidFontData, "Could not parse a valid font from the buffer");

    shared_ptr<PdfFontMetrics> metrics = PdfFontMetricsFreetype::FromBuffer(data);
    return getOrCreateFontHashed(metrics, params);
}

PdfFont& PdfFontManager::getOrCreateFontHashed(const shared_ptr<PdfFontMetrics>& metrics, const PdfFontCreateParams& params)
{
    // TODO: Add font hash to Descriptor
    Descriptor descriptor(metrics->GetBaseFontName(),
        PdfStandard14FontType::Unknown,
        params.Encoding,
        metrics->GetStyle());
    auto& fonts = m_importedFonts[descriptor];
    if (fonts.size() != 0)
        return *fonts[0];

    auto newfont = PdfFont::Create(*m_doc, metrics, params);
    return *addImported(fonts, std::move(newfont));
}

string PdfFontManager::adaptSearchParams(const string_view& fontName, PdfFontSearchParams& searchParams)
{
    bool italic;
    bool bold;
    auto ret = PdfFont::ExtractBaseName(fontName, italic, bold);
    if (italic)
        searchParams.Style |= PdfFontStyle::Italic;
    if (bold)
        searchParams.Style |= PdfFontStyle::Bold;
    return ret;
}

// NOTE: baseFontName is already normalized and cleaned from known suffixes
PdfFont* PdfFontManager::getImportedFont(const string_view& fontName, const string_view& baseFontName,
    const PdfFontSearchParams& searchParams, const PdfFontCreateParams& createParams)
{
    auto found = m_importedFonts.find(Descriptor(
        baseFontName,
        PdfStandard14FontType::Unknown,
        createParams.Encoding,
        searchParams.Style));
    if (found != m_importedFonts.end())
        return matchFont(found->second, fontName, searchParams);

    string fontpath;
    unsigned faceIndex;
    auto data = getFontData(baseFontName, searchParams, fontpath, faceIndex);
    if (data == nullptr)
        return nullptr;

    shared_ptr<PdfFontMetrics> metrics = PdfFontMetricsFreetype::FromBuffer(std::move(data));
    metrics->SetFilePath(std::move(fontpath), faceIndex);
    Descriptor descriptor(metrics->GetBaseFontName(),
        PdfStandard14FontType::Unknown,
        createParams.Encoding,
        metrics->GetStyle());
    auto& fonts = m_importedFonts[descriptor];
    PdfFont* font;
    if (fonts.size() != 0 && (font = matchFont(fonts, fontName, searchParams)) != nullptr)
        return font;

    auto newfont = PdfFont::Create(*m_doc, metrics, createParams);
    font = newfont.get();
    // Also try to match newly created font
    if (font == nullptr || (matchFont(mspan<PdfFont*>(&font, 1), fontName, searchParams) == nullptr))
        return nullptr;

    return addImported(fonts, std::move(newfont));
}

PdfFontMetricsConstPtr PdfFontManager::SearchFontMetrics(const string_view& fontPattern, const PdfFontSearchParams& params)
{
    // Early intercept Standard14 fonts
    PdfStandard14FontType stdFont;
    if (params.AutoSelect != PdfFontAutoSelectBehavior::None
        && PdfFont::IsStandard14Font(fontPattern,
            params.AutoSelect == PdfFontAutoSelectBehavior::Standard14Alt, stdFont))
    {
        return PdfFontMetricsStandard14::GetInstance(stdFont);
    }

    PdfFontSearchParams newParams = params;
    string fontpath;
    unsigned faceIndex;
    auto fontData = getFontData(adaptSearchParams(fontPattern, newParams), newParams, fontpath, faceIndex);
    if (fontData == nullptr)
        return nullptr;

    auto ret = PdfFontMetricsFreetype::FromBuffer(std::move(fontData));
    ret->SetFilePath(std::move(fontpath), faceIndex);
    return ret;
}

void PdfFontManager::AddFontDirectory(const string_view& path)
{
#ifdef PODOFO_HAVE_FONTCONFIG
    auto& fc = GetFontConfigWrapper();
    fc.AddFontDirectory(path);
#endif
#if defined(_WIN32) && defined(PODOFO_HAVE_WIN32GDI)
    string fontDir(path);
    if (fontDir[fontDir.size() - 1] != '\\')
        fontDir.push_back('\\');

    WIN32_FIND_DATAW findData;
    u16string pattern = utf8::utf8to16(fontDir);
    pattern.push_back(L'*');
    HANDLE foundH = FindFirstFileW((wchar_t*)pattern.c_str(), &findData);
    if (foundH == INVALID_HANDLE_VALUE)
    {
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
            return;

        throw runtime_error(utls::Format("Invalid font directory {}", fontDir));
    }

    do
    {
        if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
        {
            u16string filePath = utf8::utf8to16(fontDir);
            filePath.append((char16_t*)findData.cFileName);
            // Add the font resource
            // NOTE: Ignore errors
            (void)AddFontResourceExW((wchar_t*)filePath.c_str(), FR_PRIVATE, 0);
        }
    }
    while (FindNextFileW(foundH, &findData) != 0);
#endif
}

unique_ptr<charbuff> PdfFontManager::getFontData(const string_view& fontName,
    const PdfFontSearchParams& params, string& fontpath, unsigned& fontFaceIndex)
{
    string path;
    unsigned faceIndex;
    PdfFontConfigSearchParams fcParams;
    fcParams.Style = params.Style;
    fcParams.Flags = (params.MatchBehavior & PdfFontMatchBehaviorFlags::MatchPostScriptName) == PdfFontMatchBehaviorFlags::None
        ? PdfFontConfigSearchFlags::None
        : PdfFontConfigSearchFlags::MatchPostScriptName;

#ifdef PODOFO_HAVE_FONTCONFIG
    auto& fc = GetFontConfigWrapper();
    path = fc.SearchFontPath(fontName, fcParams, faceIndex);
#endif

    unique_ptr<charbuff> ret;
    if (!path.empty())
        ret = ::getFontDataFromFile(path, faceIndex);

    if (ret != nullptr)
    {
        fontpath = path;
        fontFaceIndex = faceIndex;
    }
    else
    {
        fontFaceIndex = 0;
#if defined(_WIN32) && defined(PODOFO_HAVE_WIN32GDI)
        ret = getWin32FontData(fontName, params);
#endif
    }

    return ret;
}

PdfFont& PdfFontManager::GetOrCreateFont(FT_Face face, const PdfFontCreateParams& params)
{
    string fontName = FT_Get_Postscript_Name(face);
    if (fontName.empty())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidFontData, "Could not retrieve fontname for font!");

    string baseFontName = PdfFont::ExtractBaseName(fontName);
    bool italic = (face->style_flags & FT_STYLE_FLAG_ITALIC) != 0;
    bool bold = (face->style_flags & FT_STYLE_FLAG_BOLD) != 0;
    PdfFontStyle style = PdfFontStyle::Regular;
    if (italic)
        style |= PdfFontStyle::Italic;
    if (bold)
        style |= PdfFontStyle::Bold;

    auto found = m_importedFonts.find(Descriptor(
        baseFontName,
        PdfStandard14FontType::Unknown,
        params.Encoding,
        style));
    if (found != m_importedFonts.end())
        return *matchFont(found->second, fontName);

    shared_ptr<PdfFontMetricsFreetype> metrics = PdfFontMetricsFreetype::FromFace(face);
    return getOrCreateFontHashed(metrics, params);
}

void PdfFontManager::EmbedFonts()
{
    // Embed all imported fonts
    for (auto& pair : m_importedFonts)
    {
        for (auto& font : pair.second)
            font->EmbedFont();
    }

    // Clear imported font cache
    // TODO: Don't clean standard14 and full embedded fonts
    m_importedFonts.clear();
}

#if defined(_WIN32) && defined(PODOFO_HAVE_WIN32GDI)

PdfFont& PdfFontManager::GetOrCreateFont(HFONT font, const PdfFontCreateParams& params)
{
    if (font == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Font must be non null");

    LOGFONTW logFont;
    if (::GetObjectW(font, sizeof(LOGFONTW), &logFont) == 0)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidFontData, "Invalid font");

    // CHECK-ME: Normalize name or not?
    string fontName;
    utf8::utf16to8((char16_t*)logFont.lfFaceName, (char16_t*)logFont.lfFaceName + LF_FACESIZE, std::back_inserter(fontName));
    if (fontName.empty())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidFontData, "Could not retrieve fontname for font!");

    auto baseFontName = PdfFont::ExtractBaseName(fontName);
    PdfFontStyle style = PdfFontStyle::Regular;
    if (logFont.lfItalic != 0)
        style |= PdfFontStyle::Italic;
    if (logFont.lfWeight >= FW_BOLD)
        style |= PdfFontStyle::Bold;

    auto found = m_importedFonts.find(Descriptor(
        baseFontName,
        PdfStandard14FontType::Unknown,
        params.Encoding,
        style));
    if (found != m_importedFonts.end())
        return *matchFont(found->second, fontName);

    shared_ptr<charbuff> data = ::getFontData(logFont);
    if (data == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidFontData, "Could not retrieve buffer for font!");

    shared_ptr<PdfFontMetricsFreetype> metrics = PdfFontMetricsFreetype::FromBuffer(data);
    return getOrCreateFontHashed(metrics, params);
}

std::unique_ptr<charbuff> PdfFontManager::getWin32FontData(
    const string_view& fontName, const PdfFontSearchParams& params)
{
    u16string fontnamew;
    utf8::utf8to16(fontName.begin(), fontName.end(), std::back_inserter(fontnamew));

    // The length of this fontname must not exceed LF_FACESIZE,
    // including the terminating NULL
    if (fontnamew.length() >= LF_FACESIZE)
        return nullptr;

    LOGFONTW lf;
    lf.lfHeight = 0;
    lf.lfWidth = 0;
    lf.lfEscapement = 0;
    lf.lfOrientation = 0;
    lf.lfWeight = (params.Style & PdfFontStyle::Bold) != (PdfFontStyle)0 ? FW_BOLD : 0;
    lf.lfItalic = (params.Style & PdfFontStyle::Italic) != (PdfFontStyle)0;
    lf.lfUnderline = 0;
    lf.lfStrikeOut = 0;
    // NOTE: ANSI_CHARSET should give a consistent result among
    // different locale configurations but sometimes dont' match fonts.
    // We prefer OEM_CHARSET over DEFAULT_CHARSET because it configures
    // the mapper in a way that will match more fonts
    lf.lfCharSet = OEM_CHARSET;
    lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfQuality = DEFAULT_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;

    memset(lf.lfFaceName, 0, LF_FACESIZE * sizeof(char16_t));
    memcpy(lf.lfFaceName, fontnamew.c_str(), fontnamew.length() * sizeof(char16_t));
    return ::getFontData(lf);
}

#endif // defined(_WIN32) && defined(PODOFO_HAVE_WIN32GDI)

#ifdef PODOFO_HAVE_FONTCONFIG

void PdfFontManager::SetFontConfigWrapper(const shared_ptr<PdfFontConfigWrapper>& fontConfig)
{
    if (m_fontConfig == fontConfig)
        return;

    if (fontConfig == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Fontconfig wrapper can't be null");

    m_fontConfig = fontConfig;
}

PdfFontConfigWrapper& PdfFontManager::GetFontConfigWrapper()
{
    auto ret = ensureInitializedFontConfig();
    return *ret;
}

shared_ptr<PdfFontConfigWrapper> PdfFontManager::ensureInitializedFontConfig()
{
    auto ret = m_fontConfig;
    if (ret == nullptr)
    {
        ret.reset(new PdfFontConfigWrapper());
        m_fontConfig = ret;
    }

    return ret;
}

#endif // PODOFO_HAVE_FONTCONFIG

PdfFontManager::Descriptor::Descriptor(const string_view& fontname, PdfStandard14FontType stdType,
    const PdfEncoding& encoding, PdfFontStyle style) :
    FontName(fontname),
    StdType(stdType),
    EncodingId(encoding.GetId()),
    Style(style) { }

size_t PdfFontManager::HashElement::operator()(const Descriptor& elem) const
{
    size_t hash = 0;
    utls::hash_combine(hash, elem.FontName, elem.StdType, elem.EncodingId, (size_t)elem.Style);
    return hash;
}

bool PdfFontManager::EqualElement::operator()(const Descriptor& lhs, const Descriptor& rhs) const
{
    return lhs.EncodingId == rhs.EncodingId && lhs.Style == rhs.Style && lhs.FontName == rhs.FontName;
}

unique_ptr<charbuff> getFontDataFromFile(const string_view& filename, unsigned short faceIndex)
{
    FT_Face face;
    if (!FT::TryCreateFaceFromFile(filename, faceIndex, face))
    {
        // throw an exception
        PoDoFo::LogMessage(PdfLogSeverity::Error, "Error when loading the face from filename {}",
            filename);
        return nullptr;
    }

    unique_ptr<FT_FaceRec_, decltype(&FT_Done_Face)> face_(face, FT_Done_Face);
    return getFontData(face);
}

unique_ptr<charbuff> getFontDataFromBuffer(const bufferview& buffer, unsigned short faceIndex)
{
    FT_Face face;
    if (!FT::TryCreateFaceFromBuffer(buffer, faceIndex, face))
    {
        // throw an exception
        PoDoFo::LogMessage(PdfLogSeverity::Error, "Error when loading the face from buffer");
        return nullptr;
    }

    unique_ptr<FT_FaceRec_, decltype(&FT_Done_Face)> face_(face, FT_Done_Face);
    return getFontData(face);
}

unique_ptr<charbuff> getFontData(FT_Face face)
{
    unique_ptr<charbuff> buffer;
    FT_ULong length = 0;
    FT_Error rc = FT_Load_Sfnt_Table(face, 0, 0, nullptr, &length);
    if (rc != 0)
        goto Error;

    buffer = std::make_unique<charbuff>(length);
    rc = FT_Load_Sfnt_Table(face, 0, 0, reinterpret_cast<FT_Byte*>(buffer->data()), &length);
    if (rc != 0)
        goto Error;

    return buffer;

Error:
    PoDoFo::LogMessage(PdfLogSeverity::Error, "FreeType returned the error {} when calling FT_Load_Sfnt_Table for font",
        (int)rc);
    return nullptr;
}

PdfFont* matchFont(const mspan<PdfFont*>& fonts, const string_view& fontName, const PdfFontSearchParams& params)
{
    // NOTE: Matching base name is implied by the main lookup
    return matchFont(fonts, fontName, params.MatchBehavior);
}

PdfFont* matchFont(const mspan<PdfFont*>& fonts, const string_view& fontName, PdfFontMatchBehaviorFlags matchBehavior)
{
    PODOFO_ASSERT(fonts.size() != 0);
    if (matchBehavior == PdfFontMatchBehaviorFlags::MatchExactName)
    {
        for (auto font : fonts)
        {
            if (font->GetMetrics().GetFontName() == fontName)
                return font;
        }
    }
    else // !matchExactName
    {
        // NOTE: Font is assumed to be already matched
        if (fonts.size() == 1)
            return fonts[0];

        // If there are more than one font, return one that has
        // base font name equal to font name, if present
        for (auto font : fonts)
        {
            if (font->GetName() == font->GetMetrics().GetBaseFontName())
                return font;
        }

        // Otherwise just return the first font
        return fonts[0];
    }

    return nullptr;
}

#if defined(_WIN32) && defined(PODOFO_HAVE_WIN32GDI)

unique_ptr<charbuff> getFontData(const LOGFONTW& inFont)
{
    bool success = false;
    charbuff buffer;
    HDC hdc = ::CreateCompatibleDC(nullptr);
    HFONT hf = CreateFontIndirectW(&inFont);
    if (hf != nullptr)
    {
        success = getFontData(buffer, hdc, hf);
        DeleteObject(hf);
    }
    ReleaseDC(0, hdc);

    if (success)
        return std::make_unique<charbuff>(std::move(buffer));
    else
        return nullptr;
}

bool getFontData(charbuff& buffer, HDC hdc, HFONT hf)
{
    HGDIOBJ oldFont = SelectObject(hdc, hf);
    bool sucess = false;

    // try get data from true type collection
    constexpr DWORD ttcf_const = 0x66637474;
    unsigned fileLen = GetFontData(hdc, 0, 0, nullptr, 0);
    unsigned ttcLen = GetFontData(hdc, ttcf_const, 0, nullptr, 0);

    if (fileLen != GDI_ERROR)
    {
        if (ttcLen == GDI_ERROR)
        {
            buffer.resize(fileLen);
            sucess = GetFontData(hdc, 0, 0, buffer.data(), (DWORD)fileLen) != GDI_ERROR;
        }
        else
        {
            charbuff fileBuffer(fileLen);
            if (GetFontData(hdc, ttcf_const, 0, fileBuffer.data(), fileLen) == GDI_ERROR)
            {
                sucess = false;
                goto Exit;
            }

            charbuff ttcBuffer(ttcLen);
            if (GetFontData(hdc, 0, 0, ttcBuffer.data(), ttcLen) == GDI_ERROR)
            {
                sucess = false;
                goto Exit;
            }

            getFontDataTTC(buffer, fileBuffer, ttcBuffer);
            sucess = true;
        }
    }

Exit:
    // clean up
    SelectObject(hdc, oldFont);
    return sucess;
}

// This function will recieve the device context for the
// TrueType Collection font, it will then extract necessary,
// tables and create the correct buffer.
void getFontDataTTC(charbuff& buffer, const charbuff& fileBuffer, const charbuff& ttcBuffer)
{
    uint16_t numTables = FROM_BIG_ENDIAN(*(uint16_t*)(ttcBuffer.data() + 4));
    unsigned outLen = 12 + 16 * numTables;
    const char* entry = ttcBuffer.data() + 12;

    //us: see "http://www.microsoft.com/typography/otspec/otff.htm"
    for (unsigned i = 0; i < numTables; i++)
    {
        uint32_t length = FROM_BIG_ENDIAN(*(uint32_t*)(entry + 12));
        length = (length + 3) & ~3;
        entry += 16;
        outLen += length;
    }

    buffer.resize(outLen);

    // copy font header and table index (offsets need to be still adjusted)
    memcpy(buffer.data(), ttcBuffer.data(), 12 + 16 * numTables);
    uint32_t dstDataOffset = 12 + 16 * numTables;

    // process tables
    const char* srcEntry = ttcBuffer.data() + 12;
    char* dstEntry = buffer.data() + 12;
    for (unsigned i = 0; i < numTables; i++)
    {
        // read source entry
        uint32_t offset = FROM_BIG_ENDIAN(*(uint32_t*)(srcEntry + 8));
        uint32_t length = FROM_BIG_ENDIAN(*(uint32_t*)(srcEntry + 12));
        length = (length + 3) & ~3;

        // adjust offset
        // U can use FromBigEndian() also to convert _to_ big endian
        *(uint32_t*)(dstEntry + 8) = FROM_BIG_ENDIAN(dstDataOffset);

        //copy data
        memcpy(buffer.data() + dstDataOffset, fileBuffer.data() + offset, length);
        dstDataOffset += length;

        // adjust table entry pointers for loop
        srcEntry += 16;
        dstEntry += 16;
    }
}

#endif // defined(_WIN32) && defined(PODOFO_HAVE_WIN32GDI)
