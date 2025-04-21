/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_FONT_H
#define PDF_FONT_H

#include "PdfDeclarations.h"

#include "PdfTextState.h"
#include "PdfName.h"
#include "PdfEncoding.h"
#include "PdfElement.h"
#include "PdfFontMetrics.h"

namespace PoDoFo {

class PdfCharCodeMap;

struct PODOFO_API PdfFontCreateParams final
{
    PdfEncoding Encoding;
    PdfFontCreateFlags Flags = PdfFontCreateFlags::None;
};

struct PODOFO_API PdfSplittedString final
{
    PdfString String;
    bool IsSeparator = false;
};

/** Before you can draw text on a PDF document, you have to create
 *  a font object first. You can reuse this font object as often
 *  as you want.
 *
 *  Use methods in PdfFontManager, which you can access with
 *  PdfDocument::GetFonts(), to retrieve a font object.
 *
 *  This is only an abstract base class which is implemented
 *  for different font formats.
 */
class PODOFO_API PdfFont : public PdfDictionaryElement
{
    friend class PdfFontSimple;
    friend class PdfFontCID;
    friend class PdfFontFactory;
    friend class PdfFontObject;
    friend class PdfEncoding;
    friend class PdfFontManager;

private:
    /** Create a new PdfFont object which will introduce itself
     *  automatically to every page object it is used on.
     *
     *  \param parent parent of the font object
     *  \param metrics pointer to a font metrics object. The font in the PDF
     *         file will match this fontmetrics object. The metrics object is
     *         deleted along with the font.
     *  \param encoding the encoding of this font
     */
    PdfFont(PdfDocument& doc, PdfFontType type, PdfFontMetricsConstPtr&& metrics,
        const PdfEncoding& encoding);

    /** Create a PdfFont based on an existing PdfObject
     * To be used only by PdfFontObject!
     */
    PdfFont(PdfObject& obj, PdfFontType type, PdfFontMetricsConstPtr&& metrics,
        const PdfEncoding& encoding);

public:
    virtual ~PdfFont();

public:
    /** Create a new PdfFont from an existing
     *  font in a PDF file.
     *
     *  \param obj a PDF font object
     *  \param font the created font object
     */
    static bool TryCreateFromObject(PdfObject& obj, std::unique_ptr<PdfFont>& font);
    static bool TryCreateFromObject(const PdfObject& obj, std::unique_ptr<const PdfFont>& font);

private:
    /** Create a new PdfFont object
     *
     * \param doc the parent of the created font.
     * \param metrics pointer to a font metrics object. The font in the PDF
     *      file will match this fontmetrics object. The metrics object is
     *      deleted along with the created font. In case of an error, it is deleted
     *      here.
     * \param encoding the encoding of this font.
     * \param flags flags for font init
     * \param isProxy true if the font will substitute another font
     * \remarks to be called by PdfFontManager
     * \returns a new PdfFont object or nullptr
     */
    static std::unique_ptr<PdfFont> Create(PdfDocument& doc, PdfFontMetricsConstPtr&& metrics,
        const PdfFontCreateParams& createParams, bool isProxy = false);

    /**
     * Creates a new standard 14 font object (of class PdfFontStandard14) if
     * the font name (has to include variant) is one of the standard 14 fonts.
     * The font name is to be given as specified (with an ASCII hyphen)
     *
     * \param doc the parent of the created font
     * \param std14Font standard14 font type
     * \param encoding an encoding compatible with the font
     * \param flags flags for font init
     * \remarks to be called by PdfFontManager
     * \returns a new PdfFont object
     */
    static std::unique_ptr<PdfFont> CreateStandard14(PdfDocument& doc, PdfStandard14FontType std14Font,
        const PdfFontCreateParams& createParams);

public:
    /** Try create a replacement font that can be used for rendering or font
     * program embedding based on this font characteristics
     * \param proxyFont the created substitute font
     */
    bool TryCreateProxyFont(PdfFont*& proxyFont) const;
    bool TryCreateProxyFont(PdfFontCreateFlags initFlags, PdfFont*& proxyFont) const;

    /** Write a string to a PdfObjectStream in a format so that it can
     *  be used with this font.
     *  This is used by PdfPainter::DrawText to display a text string.
     *  The following PDF operator will be Tj
     *
     *  \param stream the string will be appended to stream without any leading
     *                 or following whitespaces.
     *  \param str a unicode or ansi string which will be displayed
     */
    void WriteStringToStream(OutputStream& stream, const std::string_view& str) const;

    /** Get the GID by the codePoint
     *
     *  \param codePoint unicode codepoint
     *  \returns the GID
     *  \remarks throw if not found
     */
    unsigned GetGID(char32_t codePoint, PdfGlyphAccess access) const;
    bool TryGetGID(char32_t codePoint, PdfGlyphAccess access, unsigned& gid) const;

    /** Retrieve the width of a given text string in PDF units when
     *  drawn with the current font
     *  \param str a utf8 string of which the width should be calculated
     *  \returns the width in PDF units
     *  \remarks Doesn't throw if string glyphs could not be partially or totally found
     */
    double GetStringLength(const std::string_view& str, const PdfTextState& state) const;

    /**
     * \param str a utf8 string of which the width should be calculated
     * \remarks Produces a partial result also in case of failures
     */
    bool TryGetStringLength(const std::string_view& str, const PdfTextState& state, double& width) const;

    /** Retrieve the width of a given encoded PdfString in PDF units when
     *  drawn with the current font
     *  \param view a text string of which the width should be calculated
     *  \returns the width in PDF units
     *  \remarks Doesn't throw if string glyphs could not be partially or totally found
     */
    double GetEncodedStringLength(const PdfString& encodedStr, const PdfTextState& state) const;

    /**
     * \remarks Produces a partial result also in case of failures
     */
    bool TryGetEncodedStringLength(const PdfString& encodedStr, const PdfTextState& state, double& length) const;

    /** Scan string decoding unicode codepoints and obtaining glyphs lengths
     * \param lengths lengths of the glyphs
     * \param positions position of the CIDs in the utf8string
     * \remarks Produces a partial result also in case of failures
     */
    bool TryScanEncodedString(const PdfString& encodedStr, const PdfTextState& state, std::string& utf8str,
        std::vector<double>& lengths, std::vector<unsigned>& positions) const;

    /**
     * \returns The word spacing length
     * \remarks This differs from GetSpaceCharLength() as this will
     * be used to determine words splitting, while space char length
     * will be used to visually represent a space
     */
    double GetWordSpacingLength(const PdfTextState& state) const;

    /**
     * \returns The space char length
     * \remarks This differs from GetWordSpacingLength() as this will
     * be used to visually represent a space, while word spacing length
     * will be used to determine words splitting
     */
    double GetSpaceCharLength(const PdfTextState& state) const;

    /**
     *  \remarks Doesn't throw if character glyph could not be found
     */
    double GetCharLength(char32_t codePoint, const PdfTextState& state, bool ignoreCharSpacing = false) const;

    bool TryGetCharLength(char32_t codePoint, const PdfTextState& state, bool ignoreCharSpacing, double& width) const;

    bool TryGetCharLength(char32_t codePoint, const PdfTextState& state, double& width) const;

    double GetDefaultCharLength(const PdfTextState& state, bool ignoreCharSpacing = false) const;

    // TODO: Implement me
    /** Split the given string by white spaces
     * \remarks Also return separator chunks
     */
    //std::vector<PdfSplittedString> SplitEncodedString(const PdfString& str) const;

    /** Add used GIDs to this font for subsetting from an encoded string
     * If the subsetting is not enabled it's a no-op
     * \remarks Can't be called on non proxy fonts
     */
    void AddSubsetCIDs(const PdfString& encodedStr);

    /** True if the font has defines a custom subset and needs CID /Encoding writing
     */
    bool HasCIDSubset() const;

    /**
     * Get the final unscaled width of a CID identifier from the provided /Widths, /W arrays
     */
    double GetCIDWidth(unsigned cid) const;

    /** Retrieve the line spacing for this font
     *  \returns the linespacing in PDF units
     */
    double GetLineSpacing(const PdfTextState& state) const;

    /** Get the width of the underline for the current
     *  font size in PDF units
     *  \returns the thickness of the underline in PDF units
     */
    double GetUnderlineThickness(const PdfTextState& state) const;

    /** Return the position of the underline for the current font
     *  size in PDF units
     *  \returns the underline position in PDF units
     */
    double GetUnderlinePosition(const PdfTextState& state) const;

    /** Return the position of the strikethrough for the current font
     *  size in PDF units
     *  \returns the strikethrough position in PDF units
     */
    double GetStrikeThroughPosition(const PdfTextState& state) const;

    /** Get the width of the strikethrough for the current
     *  font size in PDF units
     *  \returns the thickness of the strikethrough in PDF units
     */
    double GetStrikeThroughThickness(const PdfTextState& state) const;

    /** Get the ascent of this font in PDF
     *  units for the current font size.
     *
     *  \returns the ascender for this font
     *
     *  \see GetAscent
     */
    double GetAscent(const PdfTextState& state) const;

    /** Get the descent of this font in PDF
     *  units for the current font size.
     *  This value is usually negative!
     *
     *  \returns the descender for this font
     *
     *  \see GetDescent
     */
    double GetDescent(const PdfTextState& state) const;

    virtual bool SupportsSubsetting() const;

    bool IsStandard14Font() const;

    bool IsStandard14Font(PdfStandard14FontType& std14Font) const;

    static std::string_view GetStandard14FontName(PdfStandard14FontType stdFont);

    /** Determine if font name is a Standard14 font
     *
     * By default use both standard names and alternative ones (Arial, TimesNewRoman, CourierNew)
     * \param fontName the unprocessed font name
     */
    static bool IsStandard14Font(const std::string_view& fontName, PdfStandard14FontType& stdFont);

    /** Determine if font name is a Standard14 font
     * \param fontName the unprocessed font name
     */
    static bool IsStandard14Font(const std::string_view& fontName, bool useAltNames, PdfStandard14FontType& stdFont);

public:
    /** True if the font is a composite CIDFont
     */
    bool IsCIDFont() const;

    /**
     * True if the font is loaded from a PdfObject
     */
    virtual bool IsObjectLoaded() const;

    PdfFontType GetType() const { return m_Type; }

    /** Check if this is a subsetting font.
     * \returns true if this is a subsetting font
     */
    inline bool IsSubsettingEnabled() const { return m_SubsettingEnabled; }

    inline bool IsEmbeddingEnabled() const { return m_EmbeddingEnabled; }

    /**
     * \returns empty string or a 6 uppercase letter and "+" sign prefix
     *          used for font subsets
     */
    inline std::string_view GetSubsetPrefix() const;

    /** Returns a reference to the fonts encoding
     *  \returns a PdfEncoding object.
     */
    inline const PdfEncoding& GetEncoding() const { return *m_Encoding; }

    /** Returns a handle to the fontmetrics object of this font.
     *  This can be used for size calculations of text strings when
     *  drawn using this font.
     *  \returns a handle to the font metrics object
     */
    inline const PdfFontMetrics& GetMetrics() const { return *m_Metrics; }

    /** Get the base font name of this font
     *
     *  \returns the font name, usually the /BaseFont key of a type1 or type0 font,
     *  or the /FontName in a font descriptor
     */
    inline const std::string& GetName() const { return m_Name; }

    /** True if the font is substitute for embedding
     */
    inline bool IsProxy() const { return m_IsProxy; }

    PdfObject& GetDescendantFontObject();

protected:
    void EmbedFontFile(PdfObject& descriptor);
    void EmbedFontFileType1(PdfObject& descriptor, const bufferview& data,
        unsigned length1, unsigned length2, unsigned length3);
    void EmbedFontFileCFF(PdfObject& descriptor, const bufferview& data, bool cidKeyed);
    void EmbedFontFileTrueType(PdfObject& descriptor, const bufferview& data);
    void EmbedFontFileOpenType(PdfObject& descriptor, const bufferview& data);

    /** Try to map the CID to a glyph ID using the /FirstChar, /LastChar limits
     */
    bool tryMapCIDToGIDLoadedMetrics(unsigned cid, unsigned& gid) const;

    /** Map the CID to a glyph ID in the font metrics using an Unicode map
     */
    bool tryMapCIDToGIDUnicode(unsigned cid, unsigned& gid) const;

    void GetBoundingBox(PdfArray& arr) const;

    /** Fill the /FontDescriptor object dictionary
     */
    void FillDescriptor(PdfDictionary& dict) const;

    /** Try getting a map that can be used to produce a replacement CID /Encoding object
     * \remarks needed when exporting substitute fonts
     */
    bool TryGetSubstituteCIDEncoding(std::unique_ptr<PdfEncodingMap>& cidEncodingMap) const;

    PdfCIDSystemInfo GetCIDSystemInfo() const;

    /** Get an ordered list of CID/GID info entries
     */
    std::vector<PdfCharGIDInfo> GetCharGIDInfos();

    virtual PdfObject* getDescendantFontObject();

    /** Initialization tasks for imported/created from scratch fonts
     */
    virtual void initImported();

    virtual void embedFont();

    virtual void embedFontSubset();

private:
    PdfFont(const PdfFont& rhs) = delete;

private:
    /** Embeds pending font into PDF page
     */
    void EmbedFont();

    /**
     * Perform initialization tasks for fonts imported or created
     * from scratch
     */
    void InitImported(bool wantEmbed, bool wantSubset, bool isProxy);

    /** Add glyph to used in case of subsetting
     *  It either maps them using the font encoding or generate a new code
     * 
     * \remarks Can't be called on proxy fonts
     * \param gid the gid to add
     * \param codePoints code points mapped by this gid. May be a single
     *      code point or a ligature
     * \param The mapped CID. Return an existing CID if already present
     * \returns true if the font provides the mapping for the codepoints
     */
    bool TryAddSubsetGID(unsigned gid, const unicodeview& codePoints, PdfCID& cid);

    /** Add dynamic charcode from code points.
     *
     * \return A mapped code. Return existing code if already present
     */
    PdfCharCode AddCharCodeSafe(unsigned gid, const unicodeview& codePoints);

    /** Optional function to map a CID to a GID
     *
     * Example for /Type2 CID fonts may have a /CIDToGIDMap
     */
    bool TryMapCIDToGID(unsigned cid, PdfGID& gid) const;
    bool TryMapCIDToGID(unsigned cid, PdfGlyphAccess access, unsigned& gid) const;

private:
    struct CIDSubsetInfo
    {
        PdfGID Gid;                     ///< The GID mapped from the source CID in the map
        PdfCharCodeList Codes;          ///< The codes that maps to the CID in the map
    };

    using CIDSubsetMap = std::map<unsigned, CIDSubsetInfo>;

    bool tryConvertToGIDs(const std::string_view& utf8Str, PdfGlyphAccess access, std::vector<unsigned>& gids) const;
    bool tryAddSubsetGID(unsigned gid, const unicodeview& codePoints, PdfCID& cid);

    void initBase(const PdfEncoding& encoding);

    double getStringLength(const std::vector<PdfCID>& cids, const PdfTextState& state) const;

    void embedFontFileData(PdfObject& descriptor, const PdfName& fontFileName,
        const std::function<void(PdfDictionary& dict)>& dictWriter, const bufferview& data);

    static std::unique_ptr<PdfFont> createFontForType(PdfDocument& doc, PdfFontMetricsConstPtr&& metrics,
        const PdfEncoding& encoding, bool preferNonCID);

    void initSpaceDescriptors();

    void pushSubsetInfo(unsigned cid, const PdfGID& gid, const PdfCharCode& code);

private:
    std::string m_Name;
    std::string m_SubsetPrefix;
    PdfFontType m_Type;
    bool m_EmbeddingEnabled;
    bool m_IsEmbedded;
    bool m_SubsettingEnabled;
    bool m_IsProxy;
    std::unique_ptr<CIDSubsetMap> m_subsetCIDMap;
    std::unique_ptr<std::unordered_map<unsigned, unsigned>> m_subsetGIDToCIDMap;
    const PdfCIDToGIDMap* m_fontProgCIDToGIDMap;
    double m_WordSpacingLengthRaw;
    double m_SpaceCharLengthRaw;

protected:
    PdfFontMetricsConstPtr m_Metrics;
    std::unique_ptr<PdfEncoding> m_Encoding;
    std::shared_ptr<PdfCharCodeMap> m_DynamicCIDMap;
    std::shared_ptr<PdfCharCodeMap> m_DynamicToUnicodeMap;
};

};

#endif // PDF_FONT_H

