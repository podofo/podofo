/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_FONT_CACHE_H
#define PDF_FONT_CACHE_H

#include "PdfDeclarations.h"

#include "PdfFont.h"
#include "PdfEncodingFactory.h"

#ifdef PODOFO_HAVE_FONTCONFIG
#include "PdfFontConfigWrapper.h"
#endif

#if defined(_WIN32) && defined(PODOFO_HAVE_WIN32GDI)
// To have LOGFONTW available
typedef struct HFONT__* HFONT;
#endif

namespace PoDoFo {

class PdfResources;

struct PODOFO_API PdfFontSearchParams final
{
    nullable<PdfFontStyle> Style;
    PdfFontAutoSelectBehavior AutoSelect = PdfFontAutoSelectBehavior::None;
    PdfFontMatchBehaviorFlags MatchBehavior = PdfFontMatchBehaviorFlags::None;
    ///< A font family name specific pattern, to be alternatively used when postscript name match failed
    std::string FontFamilyPattern;

    ///< A function to select the font in case multiple fonts with same characteristics found. Default return first
    std::function<PdfFont* (const std::vector<PdfFont*>)> FontSelector;
};

/**
 * This class assists PdfDocument
 * with caching font information.
 *
 * Additional to font caching, this class is also
 * responsible for font matching.
 *
 * PdfFont is an actual font that can be used in
 * a PDF file (i.e. it does also font embedding)
 * and PdfFontMetrics provides only metrics information.
 *
 * \see PdfDocument
 */
class PODOFO_API PdfFontManager final
{
    friend class PdfDocument;
    friend class PdfMemDocument;
    friend class PdfStreamedDocument;
    friend class PdfFont;
    friend class PdfCommon;
    friend class PdfResources;

public:
    /** Get a font from the cache. If the font does not yet
     *  exist, add it to the cache.
     *
     *  \param fontPattern a search font pattern
     *  \param params font creation params
     *
     *  \returns a PdfFont object or nullptr if the font could
     *           not be created or found.
     */
    PdfFont* SearchFont(const std::string_view& fontPattern,
        const PdfFontSearchParams& searchParams = { }, const PdfFontCreateParams& createParams = { });

    PdfFont* SearchFont(const std::string_view& fontPattern, const PdfFontCreateParams& createParams);

    PdfFont& GetStandard14Font(PdfStandard14FontType stdFont,
        const PdfFontCreateParams& params = { });

    PdfFont& GetOrCreateFont(const std::string_view& fontPath, unsigned faceIndex,
        const PdfFontCreateParams& params = { });

    PdfFont& GetOrCreateFontFromBuffer(const bufferview& buffer, unsigned faceIndex,
        const PdfFontCreateParams& params = { });

    PdfFont& GetOrCreateFont(const std::string_view& fontPath,
        const PdfFontCreateParams& params = { });

    PdfFont& GetOrCreateFontFromBuffer(const bufferview& buffer,
        const PdfFontCreateParams& params = { });

    PdfFont& GetOrCreateFont(PdfFontMetricsConstPtr metrics,
        const PdfFontCreateParams& params = { });

    /** Try getting the font from the cached font map
     * Can return nullptr
     */
    PdfFont* GetCachedFont(const PdfReference& ref);

    /** Try to search for fontmetrics from the given fontname and parameters
     *
     * \returns the found metrics. Null if not found
     */
    static PdfFontMetricsConstPtr SearchFontMetrics(const std::string_view& fontPattern,
        const PdfFontSearchParams& params = { });

#if defined(_WIN32) && defined(PODOFO_HAVE_WIN32GDI)
    PdfFont& GetOrCreateFont(HFONT font, const PdfFontCreateParams& params = { });
#endif

#ifdef PODOFO_HAVE_FONTCONFIG
    /**
     * Set wrapper for the fontconfig library.
     * Useful to avoid initializing Fontconfig multiple times.
     *
     * This setter can be called until first use of Fontconfig
     * as the library is initialized at first use.
     */
    static void SetFontConfigWrapper(const std::shared_ptr<PdfFontConfigWrapper>& fontConfig);

    static PdfFontConfigWrapper& GetFontConfigWrapper();
#endif // PODOFO_HAVE_FONTCONFIG

    /**
     * Embed all imported fonts
     * \remarks This is called by PdfDocument before saving, so
     * it's usually not necessary to call it manually
     */
    void EmbedFonts();

    // These methods are reserved to use to selected friend classes
private:
    PdfFontManager(PdfDocument& doc);

private:
    const PdfFont* GetLoadedFont(const PdfResources& resources, const std::string_view& name);

    /**
     * Empty the internal font cache.
     * This should be done whenever a new document
     * is created or opened.
     */
    void Clear();

    PdfFont* AddImported(std::unique_ptr<PdfFont>&& font);

    /** Returns a new ABCDEF+ like font subset prefix
     */
    std::string GenerateSubsetPrefix();

    static void AddFontDirectory(const std::string_view& path);

    /** NOTE: This overload doesn't perform normalization or Std14 font search
     * \param skipNormalization the font search is not normalized for embedding purpouses
     */
    static PdfFontMetricsConstPtr SearchFontMetrics(const std::string_view& fontPattern,
        const PdfFontSearchParams& params, const PdfFontMetrics& metrics, bool skipNormalization);

private:
    /** A private structure, which represents a cached font
     */
    struct Descriptor
    {
        Descriptor(const std::string_view& name, PdfStandard14FontType stdType,
            const PdfEncoding& encoding, bool hasFontStyle, PdfFontStyle style);

        Descriptor(const Descriptor& rhs) = default;

        const std::string Name;               ///< Name of the font or pattern
        const PdfStandard14FontType StdType;
        const unsigned EncodingId;
        const bool HasFontStyle;
        const PdfFontStyle Style;
    };

    struct PathDescriptor
    {
        PathDescriptor(const std::string_view& filepath, unsigned faceIndex, const PdfEncoding& encoding);

        PathDescriptor(const PathDescriptor& rhs) = default;

        const std::string FilePath;
        const unsigned FaceIndex;
        const unsigned EncodingId;
    };

    struct HashElement
    {
        size_t operator()(const Descriptor& elem) const;
        size_t operator()(const PathDescriptor& elem) const;
    };

    struct EqualElement
    {
        bool operator()(const Descriptor& lhs, const Descriptor& rhs) const;
        bool operator()(const PathDescriptor& lhs, const PathDescriptor& rhs) const;
    };

    using CachedPaths = std::unordered_map<PathDescriptor, PdfFont*, HashElement, EqualElement>;
    using CachedQueries = std::unordered_map<Descriptor, std::vector<PdfFont*>, HashElement, EqualElement>;

    struct Storage
    {
        bool IsLoaded;
        std::unique_ptr<PdfFont> Font;
    };

    using FontMap = std::unordered_map<PdfReference, Storage>;

private:
    static std::unique_ptr<const PdfFontMetrics> searchFontMetrics(const std::string_view& fontName,
        const PdfFontSearchParams& params, const PdfFontMetrics* refMetrics, bool skipNormalization);
    PdfFont* getImportedFont(const std::string_view& pattern,
        const PdfFontSearchParams& searchParams, const PdfFontCreateParams& createParams);
    PdfFont* addImported(std::vector<PdfFont*>& fonts, std::unique_ptr<PdfFont>&& font);
    PdfFont& getOrCreateFontHashed(PdfFontMetricsConstPtr&& metrics, const PdfFontCreateParams& params);

#if defined(_WIN32) && defined(PODOFO_HAVE_WIN32GDI)
    static std::unique_ptr<charbuff> getWin32FontData(const std::string_view& fontName,
        const PdfFontSearchParams& params);
#endif

private:
    PdfFontManager(const PdfFontManager&) = delete;
    PdfFontManager& operator=(const PdfFontManager&) = delete;

private:
    PdfDocument* m_doc;
    std::string m_currentPrefix;

    // Map of cached font queries
    CachedQueries m_cachedQueries;

    // Map of cached font paths
    CachedPaths m_cachedPaths;

    // Map of all indirect fonts
    FontMap m_fonts;

    // Map of all invalid inline fonts
    std::unordered_map<std::string, std::unique_ptr<PdfFont>> m_inlineFonts;

#ifdef PODOFO_HAVE_FONTCONFIG
    static std::shared_ptr<PdfFontConfigWrapper> m_fontConfig;
#endif
};

};

#endif // PDF_FONT_CACHE_H
