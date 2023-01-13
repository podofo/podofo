/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_FONT_H
#define PDF_FONT_H

#include "PdfDeclarations.h"

#include <ostream>

#include "PdfTextState.h"
#include "PdfName.h"
#include "PdfEncoding.h"
#include "PdfElement.h"
#include "PdfFontMetrics.h"

namespace PoDoFo {

class PdfObject;
class PdfPage;
class PdfWriter;
class PdfCharCodeMap;

using UsedGIDsMap = std::map<unsigned, PdfCID>;

struct PdfFontCreateParams
{
    PdfEncoding Encoding;
    PdfFontCreateFlags Flags = PdfFontCreateFlags::None;
};

struct PdfSplittedString
{
    PdfString String;
    bool IsSeparator = false;
};

/** Before you can draw text on a PDF document, you have to create
 *  a font object first. You can reuse this font object as often
 *  as you want.
 *
 *  Use PdfDocument::CreateFont to create a new font object.
 *  It will choose a correct subclass using PdfFontFactory.
 *
 *  This is only an abstract base class which is implemented
 *  for different font formats.
 */
class PODOFO_API PdfFont : public PdfDictionaryElement
{
    friend class PdfFontFactory;
    friend class PdfFontObject;
    friend class PdfEncoding;
    friend class PdfFontManager;
    friend class PdfFontSimple;

protected:
    /** Create a new PdfFont object which will introduce itself
     *  automatically to every page object it is used on.
     *
     *  \param parent parent of the font object
     *  \param metrics pointer to a font metrics object. The font in the PDF
     *         file will match this fontmetrics object. The metrics object is
     *         deleted along with the font.
     *  \param encoding the encoding of this font
     */
    PdfFont(PdfDocument& doc, const PdfFontMetricsConstPtr& metrics,
        const PdfEncoding& encoding);

private:
    /** Create a PdfFont based on an existing PdfObject
     * To be used only by PdfFontObject!
     */
    PdfFont(PdfObject& obj, const PdfFontMetricsConstPtr& metrics,
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
     * \remarks to be called by PdfFontManager
     * \returns a new PdfFont object or nullptr
     */
    static std::unique_ptr<PdfFont> Create(PdfDocument& doc, const PdfFontMetricsConstPtr& metrics,
        const PdfFontCreateParams& createParams);

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
    /** Try get a replacement font based on this font characteristics
     *  \param substFont the created substitute font
     */
    bool TryGetSubstituteFont(PdfFont*& substFont);
    bool TryGetSubstituteFont(PdfFontCreateFlags initFlags, PdfFont*& substFont);

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
     *  \returns The spacing width
     */
    double GetWordSpacingLength(const PdfTextState& state) const;

    /**
     *  \remarks Doesn't throw if characater glyph could not be found
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
     *
     * If the subsetting is not enabled it's a no-op
     */
    void AddSubsetGIDs(const PdfString& encodedStr);

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

    /** Return the position of the strikeout for the current font
     *  size in PDF units
     *  \returns the underline position in PDF units
     */
    double GetStrikeOutPosition(const PdfTextState& state) const;

    /** Get the width of the strikeout for the current
     *  font size in PDF units
     *  \returns the thickness of the strikeout in PDF units
     */
    double GetStrikeOutThickness(const PdfTextState& state) const;

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

    virtual PdfFontType GetType() const = 0;

    bool IsStandard14Font() const;

    bool IsStandard14Font(PdfStandard14FontType& std14Font) const;

    /** Extract base font name, removing known bold/italic/subset prefixes/suffixes
     */
    static std::string ExtractBaseName(const std::string_view& fontName, bool& isItalic, bool& isBold);

    /** Extract base font name, removing known bold/italic/subset prefixes/suffixes
     */
    static std::string ExtractBaseName(const std::string_view& fontName);

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
    /** True if the font is CID keyed
     */
    bool IsCIDKeyed() const;

    /**
     * True if the font is loaded from a PdfObject
     */
    virtual bool IsObjectLoaded() const;

    /** Check if this is a subsetting font.
     * \returns true if this is a subsetting font
     */
    inline bool IsSubsettingEnabled() const { return m_SubsettingEnabled; }

    inline bool IsEmbeddingEnabled() const { return m_EmbeddingEnabled; }

    /**
     * \returns empty string or a 6 uppercase letter and "+" sign prefix
     *          used for font subsets
     */
    inline const std::string& GetSubsetPrefix() const { return m_SubsetPrefix; }

    /** Returns the identifier of this font how it is known
     *  in the pages resource dictionary.
     *  \returns PdfName containing the identifier (e.g. /Ft13)
     */
    inline const PdfName& GetIdentifier() const { return m_Identifier; }

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

    const UsedGIDsMap& GetUsedGIDs() const { return m_SubsetGIDs; }

    PdfObject& GetDescendantFontObject();

protected:
    void EmbedFontFile(PdfObject& descriptor);
    void EmbedFontFileType1(PdfObject& descriptor, const bufferview& data,
        unsigned length1, unsigned length2, unsigned length3);
    void EmbedFontFileType1CCF(PdfObject& descriptor, const bufferview& data);
    void EmbedFontFileTrueType(PdfObject& descriptor, const bufferview& data);
    void EmbedFontFileOpenType(PdfObject& descriptor, const bufferview& data);

    virtual bool tryMapCIDToGID(unsigned cid, unsigned& gid) const;

    /**
     * Get the raw width of a CID identifier
     */
    double GetCIDLengthRaw(unsigned cid) const;

    void GetBoundingBox(PdfArray& arr) const;

    /** Fill the /FontDescriptor object dictionary
     */
    void FillDescriptor(PdfDictionary& dict) const;

    virtual PdfObject* getDescendantFontObject();

    /** Inititialization tasks for imported/created from scratch fonts
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
     * Perform inititialization tasks for fonts imported or created
     * from scratch
     */
    void InitImported(bool wantEmbed, bool wantSubset);

    /** Add glyph to used in case of subsetting
     *  It either maps them using the font encoding or generate a new code
     *
     * \param gid the gid to add
     * \param codePoints code points mapped by this gid. May be a single
     *      code point or a ligature
     * \return A mapped CID. Return existing CID if already present
     */
    PdfCID AddSubsetGIDSafe(unsigned gid, const unicodeview& codePoints);

    /** Add dynamic charcode from code points.
     *
     * \return A mapped code. Return existing code if already present
     */
    PdfCharCode AddCharCodeSafe(unsigned gid, const unicodeview& codePoints);

    /** Optional function to map a CID to a GID
     *
     * Example for /Type2 CID fonts may have a /CIDToGIDMap
     */
    bool TryMapCIDToGID(unsigned cid, PdfGlyphAccess access, unsigned& gid) const;

private:
    bool tryConvertToGIDs(const std::string_view& utf8Str, PdfGlyphAccess access, std::vector<unsigned>& gids) const;
    bool tryAddSubsetGID(unsigned gid, const unicodeview& codePoints, PdfCID& cid);

    void initBase(const PdfEncoding& encoding);

    double getStringLength(const std::vector<PdfCID>& cids, const PdfTextState& state) const;

    PdfObject& embedFontFileData(PdfObject& descriptor, const PdfName& fontFileName, const bufferview& data);

    static std::unique_ptr<PdfFont> createFontForType(PdfDocument& doc, const PdfFontMetricsConstPtr& metrics,
        const PdfEncoding& encoding, PdfFontFileType type, bool preferNonCID);

    void initWordSpacingLength();

private:
    std::string m_Name;
    std::string m_SubsetPrefix;
    bool m_EmbeddingEnabled;
    bool m_IsEmbedded;
    bool m_SubsettingEnabled;
    UsedGIDsMap m_SubsetGIDs;
    PdfCIDToGIDMapConstPtr m_cidToGidMap;
    double m_WordSpacingLengthRaw;

protected:
    PdfFontMetricsConstPtr m_Metrics;
    std::unique_ptr<PdfEncoding> m_Encoding;
    std::shared_ptr<PdfCharCodeMap> m_DynamicCIDMap;
    std::shared_ptr<PdfCharCodeMap> m_DynamicToUnicodeMap;
    PdfName m_Identifier;
};

};

#endif // PDF_FONT_H

