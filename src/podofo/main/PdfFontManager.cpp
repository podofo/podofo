/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfFontManager.h"

#include <algorithm>
#include <podofo/private/FileSystem.h>

#if defined(_WIN32) && defined(PODOFO_HAVE_WIN32GDI)
#include <podofo/private/WindowsLeanMean.h>
#endif // defined(_WIN32) && defined(PODOFO_HAVE_WIN32GDI)

#include <podofo/private/FreetypePrivate.h>
#include FT_TRUETYPE_TABLES_H
#include <utf8cpp/utf8.h>

#include "PdfDictionary.h"
#include <podofo/auxiliary/InputDevice.h>
#include <podofo/auxiliary/OutputDevice.h>
#include "PdfFont.h"
#include "PdfFontMetricsFreetype.h"
#include "PdfFontMetricsStandard14.h"
#include "PdfFontType1.h"
#include "PdfResources.h"

using namespace std;
using namespace PoDoFo;

namespace
{
    struct AdaptedFontSearch
    {
        string Pattern;
        PdfFontSearchParams Params;
    };
}

static bool tryAdaptSearchParams(const std::string_view& patternName, const PdfFontSearchParams& params,
    unique_ptr<AdaptedFontSearch>& adaptedParams);

#if defined(_WIN32) && defined(PODOFO_HAVE_WIN32GDI)

static unique_ptr<charbuff> getFontData(const LOGFONTW& inFont);
static bool getFontData(charbuff& buffer, HDC hdc, HFONT hf);
static void getFontDataTTC(charbuff& buffer, const charbuff& fileBuffer, const charbuff& ttcBuffer);

#endif // defined(_WIN32) && defined(PODOFO_HAVE_WIN32GDI)

#if defined(PODOFO_HAVE_FONTCONFIG)
shared_ptr<PdfFontConfigWrapper> PdfFontManager::m_fontConfig;
#endif

static constexpr unsigned SUBSET_PREFIX_LEN = 6;

PdfFontManager::PdfFontManager(PdfDocument& doc)
    : m_doc(&doc)
{
    m_currentPrefix = "AAAAAA+";
}

void PdfFontManager::Clear()
{
    m_cachedQueries.clear();
    m_fonts.clear();
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

PdfFont* PdfFontManager::AddImported(unique_ptr<PdfFont>&& font)
{
    // Explicitly cache the font with its name and font style
    Descriptor descriptor(font->GetMetrics().GetFontName(),
        PdfStandard14FontType::Unknown,
        font->GetEncoding(),
        true,
        font->GetMetrics().GetStyle());
    return addImported(m_cachedQueries[descriptor], std::move(font));
}

PdfFont* PdfFontManager::addImported(vector<PdfFont*>& fonts, unique_ptr<PdfFont>&& font)
{
    auto fontPtr = font.get();
    fonts.push_back(fontPtr);
    m_fonts.insert({ fontPtr->GetObject().GetIndirectReference(), Storage{ false, std::move(font) } });
    return fontPtr;
}

const PdfFont* PdfFontManager::GetLoadedFont(const PdfResources& resources, const string_view& name)
{
    auto fontObj = resources.GetResource(PdfResourceType::Font, name);
    if (fontObj == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidFontData, "A font with name {} was not found", name);

    if (fontObj->IsIndirect())
    {
        auto found = m_fonts.find(fontObj->GetIndirectReference());
        if (found != m_fonts.end())
        {
            if (!found->second.IsLoaded)
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidFontData, "Invalid imported font queried");

            return found->second.Font.get();
        }

        // Create a new font
        unique_ptr<PdfFont> font;
        if (!PdfFont::TryCreateFromObject(const_cast<PdfObject&>(*fontObj), font))
            return nullptr;

        auto inserted = m_fonts.emplace(fontObj->GetIndirectReference(), Storage{ true, std::move(font) });
        return inserted.first->second.Font.get();
    }
    else
    {
        // It's a specification invalid inline font. We must support
        // it anyway, since Adobe is lenient as usual. We create an id
        // for this font and put it in the inline fonts map
        auto obj = &resources.GetObject();
        PdfReference ref;
        do
        {
            // Find the first indirect ancestor object
            ref = obj->GetIndirectReference();
            if (ref.IsIndirect())
                break;

            PODOFO_INVARIANT(obj->GetParent() != nullptr);
            obj = obj->GetParent()->GetOwner();
        } while (obj != nullptr);
        auto inlineFontId = utls::Format("R{}_{}-{}", ref.ObjectNumber(), ref.GenerationNumber(), name);
        auto found = m_inlineFonts.find(inlineFontId);
        if (found != m_inlineFonts.end())
            return found->second.get();


        // Create a new font
        unique_ptr<PdfFont> font;
        if (!PdfFont::TryCreateFromObject(const_cast<PdfObject&>(*fontObj), font))
            return nullptr;

        auto inserted = m_inlineFonts.emplace(inlineFontId, std::move(font));
        return inserted.first->second.get();
    }
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

    return getImportedFont(fontPattern, searchParams, createParams);
}

PdfFont& PdfFontManager::GetStandard14Font(PdfStandard14FontType stdFont, const PdfFontCreateParams& params)
{
    // Create a special descriptor cache that just specify the standard type and encoding
     // NOTE: We assume font name and style are implicit in the standard font type
    Descriptor descriptor(
        { },
        stdFont,
        params.Encoding,
        false,
        PdfFontStyle::Regular);
    auto& fonts = m_cachedQueries[descriptor];
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
    // NOTE: Canonical seems to handle also case insensitive paths,
    // converting them to actual casing
    auto normalizedPath = fs::canonical(fs::u8path(fontPath)).u8string();
    PathDescriptor descriptor(normalizedPath, faceIndex, params.Encoding);
    auto found = m_cachedPaths.find(descriptor);
    if (found != m_cachedPaths.end())
        return *found->second;

    auto metrics = PdfFontMetrics::Create(fontPath, faceIndex);
    if (metrics == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidFontData, "Invalid or unsupported font");

    auto& ret = getOrCreateFontHashed(std::move(metrics), params);
    m_cachedPaths[descriptor] = &ret;
    return ret;
}

PdfFont& PdfFontManager::GetOrCreateFontFromBuffer(const bufferview& buffer, const PdfFontCreateParams& createParams)
{
    return GetOrCreateFontFromBuffer(buffer, 0, createParams);
}

PdfFont& PdfFontManager::GetOrCreateFont(PdfFontMetricsConstPtr metrics, const PdfFontCreateParams& params)
{
    return getOrCreateFontHashed(std::move(metrics), params);
}

PdfFont* PdfFontManager::GetCachedFont(const PdfReference& ref)
{
    auto found = m_fonts.find(ref);
    if (found == m_fonts.end())
        return nullptr;

    return found->second.Font.get();
}

PdfFont& PdfFontManager::GetOrCreateFontFromBuffer(const bufferview& buffer, unsigned faceIndex, const PdfFontCreateParams& params)
{
    return getOrCreateFontHashed(PdfFontMetrics::CreateFromBuffer(buffer, faceIndex), params);
}

PdfFont& PdfFontManager::getOrCreateFontHashed(PdfFontMetricsConstPtr&& metrics, const PdfFontCreateParams& params)
{
    // TODO: Create a map indexed only on the hash of the font data
    // and search on that. Then remove the following
    Descriptor descriptor(metrics->GetFontName(),
        PdfStandard14FontType::Unknown,
        params.Encoding,
        true,
        metrics->GetStyle());
    auto& fonts = m_cachedQueries[descriptor];
    if (fonts.size() != 0)
        return *fonts[0];

    auto newfont = PdfFont::Create(*m_doc, std::move(metrics), params);
    return *addImported(fonts, std::move(newfont));
}

// NOTE: baseFontName is already normalized and cleaned from known suffixes
PdfFont* PdfFontManager::getImportedFont(const string_view& pattern,
    const PdfFontSearchParams& searchParams, const PdfFontCreateParams& createParams)
{
    auto& fonts = m_cachedQueries[Descriptor(
        pattern,
        PdfStandard14FontType::Unknown,
        createParams.Encoding,
        searchParams.Style != nullptr,
        searchParams.Style == nullptr ? PdfFontStyle::Regular : *searchParams.Style)];
    if (fonts.size() != 0)
    {
        if (searchParams.FontSelector == nullptr)
            return fonts[0];
        else
            searchParams.FontSelector(fonts);
    }

    unique_ptr<AdaptedFontSearch> adaptedSearch;
    unique_ptr<const PdfFontMetrics> metrics;
    if (tryAdaptSearchParams(pattern, searchParams, adaptedSearch))
        metrics = searchFontMetrics(adaptedSearch->Pattern, adaptedSearch->Params, nullptr, false);
    else
        metrics = searchFontMetrics(pattern, searchParams, nullptr, false);

    if (metrics == nullptr)
        return nullptr;

    auto ret = AddImported(PdfFont::Create(*m_doc, std::move(metrics), createParams));
    fonts.push_back(ret);
    return ret;
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

    unique_ptr<AdaptedFontSearch> adaptedSearch;
    if (tryAdaptSearchParams(fontPattern, params, adaptedSearch))
        return searchFontMetrics(adaptedSearch->Pattern, adaptedSearch->Params, nullptr, false);
    else
        return searchFontMetrics(fontPattern, params, nullptr, false);
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

        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Invalid font directory {}", fontDir);
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

PdfFontMetricsConstPtr PdfFontManager::SearchFontMetrics(const string_view& fontPattern, const PdfFontSearchParams& params,
    const PdfFontMetrics& metrics, bool skipNormalization)
{
    PODOFO_ASSERT(params.MatchBehavior == PdfFontMatchBehaviorFlags::None);
    return searchFontMetrics(fontPattern, params, &metrics, skipNormalization);
}

unique_ptr<const PdfFontMetrics> PdfFontManager::searchFontMetrics(const string_view& fontName,
    const PdfFontSearchParams& params, const PdfFontMetrics* refMetrics, bool skipNormalization)
{
    string path;
    unsigned faceIndex = 0;
#ifdef PODOFO_HAVE_FONTCONFIG
    PdfFontConfigSearchParams fcParams;
    fcParams.FontFamilyPattern = params.FontFamilyPattern;
    fcParams.Style = params.Style;
    fcParams.Flags = (params.MatchBehavior & PdfFontMatchBehaviorFlags::SkipMatchPostScriptName) == PdfFontMatchBehaviorFlags::None
        ? PdfFontConfigSearchFlags::None
        : PdfFontConfigSearchFlags::SkipMatchPostScriptName;

    auto& fc = GetFontConfigWrapper();
    path = fc.SearchFontPath(fontName, fcParams, faceIndex);
#endif

    unique_ptr<const PdfFontMetrics> ret = nullptr;
    if (!path.empty())
        ret = PdfFontMetrics::CreateFromFile(path, faceIndex, refMetrics, skipNormalization);

    if (ret == nullptr)
    {
#if defined(_WIN32) && defined(PODOFO_HAVE_WIN32GDI)
        // Try to use WIN32 GDI to find the font
        auto data = getWin32FontData(fontName, params);
        if (data != nullptr)
        {
            // NOTE: The font has been already extracted from collections at this point
            unique_ptr<FT_FaceRec_, decltype(&FT_Done_Face)> face(FT::CreateFaceFromBuffer(*data), FT_Done_Face);

            ret = PdfFontMetrics::CreateFromFace(face.get(), std::move(data), refMetrics, skipNormalization);
            (void)face.release();
        }
#endif
    }

    return ret;
}

void PdfFontManager::EmbedFonts()
{
    // Collect fonts to embed from cached queries
    set<PdfReference> fontToEmbeds;
    for (auto& pair : m_cachedQueries)
    {
        for (auto& font : pair.second)
            fontToEmbeds.insert(font->GetObject().GetIndirectReference());
    }

    // Embed fonts now in deterministic order (note set<T> will guarantee this)
    for (auto& ref : fontToEmbeds)
        m_fonts[ref].Font->EmbedFont();

    // Clear imported font cache
    // TODO: Don't clean standard14 and full embedded fonts
    m_cachedQueries.clear();
}

#if defined(_WIN32) && defined(PODOFO_HAVE_WIN32GDI)

PdfFont& PdfFontManager::GetOrCreateFont(HFONT font, const PdfFontCreateParams& params)
{
    if (font == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Font must be non null");

    LOGFONTW logFont;
    if (::GetObjectW(font, sizeof(LOGFONTW), &logFont) == 0)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidFontData, "Invalid font");

    string fontName;
    utf8::utf16to8((char16_t*)logFont.lfFaceName, (char16_t*)logFont.lfFaceName + LF_FACESIZE, std::back_inserter(fontName));
    if (fontName.empty())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidFontData, "Could not retrieve fontname for font!");

    PdfFontStyle style = PdfFontStyle::Regular;
    if (logFont.lfItalic != 0)
        style |= PdfFontStyle::Italic;
    if (logFont.lfWeight >= FW_BOLD)
        style |= PdfFontStyle::Bold;

    // Explicitly search the cached fonts with the given name and font style
    auto found = m_cachedQueries.find(Descriptor(
        fontName,
        PdfStandard14FontType::Unknown,
        params.Encoding,
        true,
        style));
    if (found != m_cachedQueries.end())
        return *found->second[0];

    auto data = ::getFontData(logFont);
    if (data == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidFontData, "Could not retrieve buffer for font!");

    // NOTE: The font has been already extracted from collections at this point
    unique_ptr<FT_FaceRec_, decltype(&FT_Done_Face)> face(FT::CreateFaceFromBuffer(*data), FT_Done_Face);

    auto metrics = PdfFontMetrics::CreateFromFace(face.get(), std::move(data), nullptr, false);
    (void)face.release();
    if (metrics == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidFontData, "Could not create valid font!");

    return getOrCreateFontHashed(std::move(metrics), params);
}

// Returned font data is also extracted from collections
unique_ptr<charbuff> PdfFontManager::getWin32FontData(
    const string_view& fontName, const PdfFontSearchParams& params)
{
    u16string fontnamew;
    utf8::utf8to16(fontName.begin(), fontName.end(), std::back_inserter(fontnamew));

    // The length of this fontname must not exceed LF_FACESIZE,
    // including the terminating NULL
    if (fontnamew.length() >= LF_FACESIZE)
        return nullptr;

    LOGFONTW lf{ };
    // NOTE: ANSI_CHARSET should give a consistent result among
    // different locale configurations but sometimes don't match fonts.
    // We prefer OEM_CHARSET over DEFAULT_CHARSET because it configures
    // the mapper in a way that will match more fonts
    lf.lfCharSet = OEM_CHARSET;
    lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfQuality = DEFAULT_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;

    if (params.Style.has_value())
    {
        lf.lfWeight = (*params.Style & PdfFontStyle::Bold) != (PdfFontStyle)0 ? FW_BOLD : 0;
        lf.lfItalic = (*params.Style & PdfFontStyle::Italic) != (PdfFontStyle)0;
    }

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
    if (m_fontConfig == nullptr)
        m_fontConfig.reset(new PdfFontConfigWrapper());

    return *m_fontConfig;
}

#endif // PODOFO_HAVE_FONTCONFIG

PdfFontManager::Descriptor::Descriptor(const string_view& name, PdfStandard14FontType stdType,
    const PdfEncoding& encoding, bool hasFontStyle, PdfFontStyle style) :
    Name(name),
    StdType(stdType),
    EncodingId(encoding.GetId()),
    HasFontStyle(hasFontStyle),
    Style(style) { }

PdfFontManager::PathDescriptor::PathDescriptor(const std::string_view& filepath, unsigned faceIndex, const PdfEncoding& encoding)
    : FilePath(filepath), FaceIndex(faceIndex), EncodingId(encoding.GetId()) { }

size_t PdfFontManager::HashElement::operator()(const Descriptor& elem) const
{
    size_t hash = 0;
    utls::hash_combine(hash, elem.Name, elem.StdType,
        elem.EncodingId, elem.HasFontStyle, (size_t)elem.Style);
    return hash;
}

size_t PdfFontManager::HashElement::operator()(const PathDescriptor& elem) const
{
    size_t hash = 0;
    utls::hash_combine(hash, elem.FilePath, elem.FaceIndex, elem.EncodingId);
    return hash;
}

bool PdfFontManager::EqualElement::operator()(const Descriptor& lhs, const Descriptor& rhs) const
{
    return lhs.EncodingId == rhs.EncodingId
        && lhs.Name == rhs.Name
        && lhs.StdType == rhs.StdType
        && lhs.HasFontStyle == rhs.HasFontStyle
        && lhs.Style == rhs.Style;
}

bool PdfFontManager::EqualElement::operator()(const PathDescriptor& lhs, const PathDescriptor& rhs) const
{
    return lhs.FilePath == rhs.FilePath
        && lhs.FaceIndex == rhs.FaceIndex
        && lhs.EncodingId == rhs.EncodingId;
}

#if defined(_WIN32) && defined(PODOFO_HAVE_WIN32GDI)

// Returned font data is also extracted from collections
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
            // The font is not in a TTC collection, just use the
            // whole font buffer as returned by GetFontData
            buffer.resize(fileLen);
            sucess = GetFontData(hdc, 0, 0, buffer.data(), (DWORD)fileLen) != GDI_ERROR;
        }
        else
        {
            // Handle TTC font collections
            charbuff fileBuffer(fileLen);
            if (GetFontData(hdc, 0, 0, fileBuffer.data(), fileLen) == GDI_ERROR)
            {
                sucess = false;
                goto Exit;
            }

            charbuff ttcBuffer(ttcLen);
            if (GetFontData(hdc, ttcf_const, 0, ttcBuffer.data(), ttcLen) == GDI_ERROR)
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

// This function will receive the device context for the
// TrueType Collection font, it will then extract necessary,
// tables and create the correct buffer.
void getFontDataTTC(charbuff& buffer, const charbuff& fileBuffer, const charbuff& ttcBuffer)
{
    uint16_t numTables = FROM_BIG_ENDIAN(*(uint16_t*)(fileBuffer.data() + 4));
    unsigned outLen = 12 + 16 * numTables;
    const char* entry = fileBuffer.data() + 12;

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
    memcpy(buffer.data(), fileBuffer.data(), 12 + 16 * numTables);
    uint32_t dstDataOffset = 12 + 16 * numTables;

    // process tables
    const char* srcEntry = fileBuffer.data() + 12;
    char* dstEntry = buffer.data() + 12;
    for (unsigned i = 0; i < numTables; i++)
    {
        // read source entry
        uint32_t offset = FROM_BIG_ENDIAN(*(uint32_t*)(srcEntry + 8));
        uint32_t length = FROM_BIG_ENDIAN(*(uint32_t*)(srcEntry + 12));
        length = (length + 3) & ~3;

        // adjust offset
        *(uint32_t*)(dstEntry + 8) = AS_BIG_ENDIAN(dstDataOffset);

        //copy data
        memcpy(buffer.data() + dstDataOffset, ttcBuffer.data() + offset, length);
        dstDataOffset += length;

        // adjust table entry pointers for loop
        srcEntry += 16;
        dstEntry += 16;
    }
}

#endif // defined(_WIN32) && defined(PODOFO_HAVE_WIN32GDI)

bool tryAdaptSearchParams(const string_view& fontName, const PdfFontSearchParams& params,
    unique_ptr<AdaptedFontSearch>& adaptedParams)
{
    if ((params.MatchBehavior & PdfFontMatchBehaviorFlags::NormalizePattern)
        == PdfFontMatchBehaviorFlags::None)
    {
        return false;
    }

    adaptedParams.reset(new AdaptedFontSearch{ (string)fontName, params });

    bool italic;
    bool bold;
    adaptedParams->Pattern = PoDoFo::ExtractFontHints(fontName, italic, bold);
    PdfFontStyle style = PdfFontStyle::Regular;
    if (italic)
        style |= PdfFontStyle::Italic;
    if (bold)
        style |= PdfFontStyle::Bold;

    // Alter search style only if italic/bold was extracted from the name
    if (style != PdfFontStyle::Regular)
        adaptedParams->Params.Style = style;

    return true;
}
