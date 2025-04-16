/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_FONT_METRICS_H
#define PDF_FONT_METRICS_H

#include "PdfString.h"
#include "PdfCMapEncoding.h"
#include "PdfCIDToGIDMap.h"
#include <podofo/auxiliary/Matrix.h>
#include <podofo/auxiliary/Corners.h>

FORWARD_DECLARE_FREETYPE();

namespace PoDoFo {

class PdfFontMetrics;

// NOTE: Underlying type may may change in the future
using GlyphMetricsListConstPtr = std::shared_ptr<const std::vector<double>>;

/** Convenience typedef for a const PdfEncoding shared ptr
 */
using PdfFontMetricsConstPtr = std::shared_ptr<const PdfFontMetrics>;

/**
 * This abstract class provides access to font metrics information.
 *
 * The class doesn't know anything about CIDs (Character IDs),
 * it just index glyphs, or GIDs where the terminology applies
 */
class PODOFO_API PdfFontMetrics
{
    friend class PdfFont;
    friend class PdfFontObject;
    friend class PdfFontManager;
    friend class PdfFontMetricsBase;
    friend class PdfFontMetricsFreetype;
    friend class PdfEncodingFactory;
    friend class PdfDifferenceEncoding;
    PODOFO_PRIVATE_FRIEND(class FontTrueTypeSubset);

private:
    PdfFontMetrics();

public:
    virtual ~PdfFontMetrics();

    static std::unique_ptr<const PdfFontMetrics> Create(const std::string_view& filepath, unsigned faceIndex = 0);

    static std::unique_ptr<const PdfFontMetrics> CreateFromBuffer(const bufferview& buffer, unsigned faceIndex = 0);

    /** Get the glyph count
     * \remarks By defaults tt returns the actual number of glyphs in the font program
     */
    unsigned GetGlyphCount() const;

    /** Get the glyph count with the given glyph access
     *  \param access the desired access for retrieving the glyph count
     */
    unsigned GetGlyphCount(PdfGlyphAccess access) const;

    /** Get the width of a single glyph id. It tries to access parsed pdf metrics first,
     * and if unavailable it retrieve it from available font program
     *
     *  \param gid id of the glyph
     *  \returns the width of a single glyph id
     */
    double GetGlyphWidth(unsigned gid) const;
    bool TryGetGlyphWidth(unsigned gid, double& width) const;

    /** Get the width of a single glyph id
     *
     *  \param gid id of the glyph
     *  \param access the desired access for retrieving the metrics
     *  \returns the width of a single glyph id
     */
    double GetGlyphWidth(unsigned gid, PdfGlyphAccess access) const;
    bool TryGetGlyphWidth(unsigned gid, PdfGlyphAccess access, double& width) const;

    /**
     * Some fonts provides a glyph substitution list, eg. for ligatures.
     * OpenType fonts for example provides GSUB "Glyph Substitution Table"
     * \param gids gids to be substituted
     * \param backwardMap list of gid counts to remap back substituted gids
     *     eg. { 32, 102, 105 } gets substituted in { 32, 174 }
     *     the backward map is { 1, 2 }
     */
    virtual void SubstituteGIDs(std::vector<unsigned>& gids,
        std::vector<unsigned char>& backwardMap) const;

    /** Determines if the metrics has a valid Unicode
     * code point to gid map
     */
    virtual bool HasUnicodeMapping() const = 0;

    /** Try to retrieve the mapped gid from Unicode code point
     * \remarks don't use this method directly unless you know
     * what you're doing: use PdfFont::TryGetGID instead
     */
    virtual bool TryGetGID(char32_t codePoint, unsigned& gid) const = 0;

    /** Retrieve the line spacing for this font
     *  \returns the linespacing in PDF units
     */
    virtual double GetLineSpacing() const = 0;

    /** Get the width of the underline for the current
     *  font size in PDF units
     *  \returns the thickness of the underline in PDF units
     */
    virtual double GetUnderlineThickness() const = 0;

    /** Return the position of the underline for the current font
     *  size in PDF units
     *  \returns the underline position in PDF units
     */
    virtual double GetUnderlinePosition() const = 0;

    /** Return the position of the strikethrough for the current font
     *  size in PDF units
     *  \returns the strikethrough position in PDF units
     */
    virtual double GetStrikeThroughPosition() const = 0;

    /** Get the width of the strikethrough for the current
     *  font size in PDF units
     *  \returns the thickness of the strikethrough in PDF units
     */
    virtual double GetStrikeThroughThickness() const = 0;

    virtual PdfFontFileType GetFontFileType() const = 0;

    bool HasFontFileData() const;

    /** Get an actual font data view
     *
     * The data shall be resident. For font coming from the /FontFile
     * keys, GetFontFileObject() may also be available.
     * \returns a binary buffer of data containing the font data
     */
    bufferview GetOrLoadFontFileData() const;

    /** Get the actual font file object from a /FontFile like key, if available
     *
     * For font data coming from a file imported font, see GetFontFileData()
     * \returns a binary buffer of data containing the font data
     */
    virtual const PdfObject* GetFontFileObject() const;

    /** Get /Length1 value for the font file, if available
     */
    virtual unsigned GetFontFileLength1() const = 0;

    /** Get /Length2 value for the font file, if available
     */
    virtual unsigned GetFontFileLength2() const = 0;

    /** Get /Length3 value for the font file, if available
     */
    virtual unsigned GetFontFileLength3() const = 0;

    /** Get the actual /FontName, eg. "AAAAAA+Arial,Bold", if available
     *
     *  By default returns empty string
     *  \returns the postscript name of the font or empty string if no postscript name is available.
     */
    virtual std::string_view GetFontName() const = 0;

    /** Get the actual /FontName, eg. "AAAAAA+Arial,Bold", if available
     *  By default returns GetFontName()
     */
    virtual std::string_view GetFontNameRaw() const;

    /** Get the actual /FontFamily, eg. "Times", if available
     */
    virtual std::string_view GetFontFamilyName() const = 0;

    /** Get a family font name, either from /FontFamily or constructed
     * from available /BaseFont, /FontName (eg. "AAAAAA+Arial,Bold" becomes "Arial")
     *
     * \remarks It doesn't correspond to /BaseFont name entry in the font
     */
    std::string_view GeFontFamilyNameSafe() const;

    /** Get the length of the subset prefix (eg. 7 for "AAAAAA+") if present
     */
    virtual unsigned char GetSubsetPrefixLength() const;

    /**
     * Get a an approximate PostScript name, from available /BaseFont, /FontName
     * (eg. "AAAAAA+Arial-Bold" becomes "Arial-Bold")
     * By default returns GetFontName()
     */
    std::string_view GetPostScriptNameRough() const;

    virtual PdfFontStretch GetFontStretch() const = 0;

    /** Get the weight of this font.
     *  \returns the weight of this font (400 <= x < 700 means normal, x >= 700 means bold)
     */
    unsigned GetWeight() const;
    virtual int GetWeightRaw() const = 0;

    virtual bool TryGetFlags(PdfFontDescriptorFlags& value) const = 0;
    PdfFontDescriptorFlags GetFlags() const;

    /** Create the bounding box vector in PDF units
     *
     *  \param bbox write the bounding box to this vector
     */
    virtual bool TryGetBoundingBox(Corners& value) const = 0;
    Corners GetBoundingBox() const;

    /** Get the italic angle of this font.
     *  Used to build the font dictionary
     *  \returns the italic angle of this font.
     */
    virtual bool TryGetItalicAngle(double& value) const = 0;
    double GetItalicAngle() const;

    /** Get the ascent of this font in PDF
     *  units for the current font size.
     *
     * \returns the ascender for this font
     *
     * \see GetAscent
     */
    virtual bool TryGetAscent(double& value) const = 0;
    double GetAscent() const;

    /** Get the descent of this font in PDF
     *  units for the current font size.
     *  This value is usually negative!

     *  \returns the descender for this font
     *
     *  \see GetDescent
     */
    virtual bool TryGetDescent(double& value) const = 0;
    double GetDescent() const;

    /** The vertical coordinate of the top of flat capital letters, measured from the baseline
     */
    virtual bool TryGetCapHeight(double& value) const = 0;
    double GetCapHeight() const;

    /** The thickness, measured horizontally, of the dominant vertical stems of glyphs in the font
     */
    virtual bool TryGetStemV(double& value) const = 0;
    double GetStemV() const;

    /* /Leading (optional, default 0)
     */
    double GetLeading() const;
    virtual double GetLeadingRaw() const = 0;

    /** The font’s x height: the vertical coordinate of the top of flat nonascending
     * lowercase letters (like the letter x), measured from the baseline, in
     * fonts that have Latin characters (optional, default 0)
     */
    double GetXHeight() const;
    virtual double GetXHeightRaw() const = 0;

    /** The thickness, measured vertically, of the dominant horizontal
     * stems of glyphs in the font (optional, default 0)
     */
    double GetStemH() const;
    virtual double GetStemHRaw() const = 0;

    /** /AvgWidth (optional, default 0)
     */
    double GetAvgWidth() const;
    virtual double GetAvgWidthRaw() const = 0;

    /** /MaxWidth (optional, default 0)
     */
    double GetMaxWidth() const;
    virtual double GetMaxWidthRaw() const = 0;

    /** /MissingWidth or /DW in CID fonts (optional default 1000 in CID fonts, 0 otherwise)
     */
    double GetDefaultWidth() const;
    virtual double GetDefaultWidthRaw() const = 0;

    /** Get whether the font style is bold.
     *
     * This is a logical value that can be inferred
     * from several characteristics
     */
    PdfFontStyle GetStyle() const;

    virtual bool IsObjectLoaded() const;

    bool IsStandard14FontMetrics() const;

    virtual bool IsStandard14FontMetrics(PdfStandard14FontType& std14Font) const;

    /** Returns the matrix mapping glyph space to text space
     */
    virtual const Matrix& GetMatrix() const;

    /** Determine if the metrics are for Adobe Type1 like font
     */
    bool IsType1Kind() const;

    /** Determine if the metrics are TrueType like font
     */
    bool IsTrueTypeKind() const;

    /** Determine if the font is non symbolic according to the PDF definition
     *
     * The font is symbolic if "contains glyphs outside the Standard Latin character set"
     */
    bool IsPdfSymbolic() const;

    /** Determine if the font is symbolic according to the PDF definition
     *
     * The font is symbolic if "uses the Standard Latin character set or a subset of it."
     */
    bool IsPdfNonSymbolic() const;

    /** Create a best effort /ToUnicode map based on the
     * character unicode maps of the font
     *
     * This is implemented just for PdfFontMetricsFreetype
     * This map may be unreliable because of ligatures,
     * other kind of character substitutions, or glyphs
     * mapping to multiple unicode codepoints.
     */
    virtual std::unique_ptr<PdfCMapEncoding> CreateToUnicodeMap(const PdfEncodingLimits& limitHints) const;

public:
    const std::string& GetFilePath() const { return m_FilePath; }
    unsigned GetFaceIndex() const { return m_FaceIndex; }

protected:
    virtual PdfFontType GetFontType() const;

    /** Get a semantical base name for the font that can be used to
     * compose the final name, eg. from "AAAAAA+Arial,Bold" to "Arial"
     *
     * \remarks It doesn't correspond to /BaseFont name entry in the font
     */
    virtual std::string_view GetBaseFontName() const = 0;

    virtual bool getIsBoldHint() const = 0;
    virtual bool getIsItalicHint() const = 0;
    virtual const datahandle& GetFontFileDataHandle() const = 0;
    virtual FT_Face GetFaceHandle() const = 0;

    virtual unsigned GetGlyphCountFontProgram() const;
    virtual bool TryGetGlyphWidthFontProgram(unsigned gid, double& width) const;

    bool HasParsedWidths() const;

    /** Retrieve the parsed width from a /W or /Widths entry, if available
     */
    GlyphMetricsListConstPtr GetParsedWidths() const { return m_ParsedWidths; }

    void SetParsedWidths(GlyphMetricsListConstPtr&& parsedWidths);

    /** Get a built-in CID to GID map, such as when no /Encoding is defined
     */
    virtual PdfCIDToGIDMapConstPtr GetBuiltinCIDToGIDMap() const;

private:
    static std::unique_ptr<const PdfFontMetrics> CreateFromFile(const std::string_view& filepath, unsigned faceIndex,
        const PdfFontMetrics* metrics, bool skipNormalization);

    static std::unique_ptr<const PdfFontMetrics> CreateFromBuffer(const bufferview& buffer, unsigned faceIndex,
        const PdfFontMetrics* metrics, bool skipNormalization);

    static std::unique_ptr<PdfFontMetrics> CreateFromFace(FT_Face face, std::unique_ptr<charbuff>&& buffer,
        const PdfFontMetrics* metrics, bool skipNormalization);

    /** Create a new font metrics by merging characteristics from this instance
     */
    std::unique_ptr<const PdfFontMetrics> CreateMergedMetrics(bool skipNormalization) const;

    /** Get an implicit encoding, such as the one of standard14 fonts,
     * or the built-in encoding of a Type1 font, if available
     */
    PdfEncodingMapConstPtr GetImplicitEncoding(PdfCIDToGIDMapConstPtr& cidToGidMap) const;
    PdfEncodingMapConstPtr GetImplicitEncoding() const;

    static PdfEncodingMapConstPtr getFontType1ImplicitEncoding(FT_Face face);
    void initFamilyFontNameSafe();
    PdfEncodingMapConstPtr getImplicitEncoding(bool tryFetchCidToGidMap, PdfCIDToGIDMapConstPtr& cidToGidMap) const;

private:
    PdfFontMetrics(const PdfFontMetrics& rhs) = delete;
    PdfFontMetrics& operator=(const PdfFontMetrics& rhs) = delete;

private:
    std::string m_FilePath;
    std::string m_FamilyFontNameSafe;
    GlyphMetricsListConstPtr m_ParsedWidths;
    nullable<PdfFontStyle> m_Style;
    unsigned m_FaceIndex;
};

class PODOFO_API PdfFontMetricsBase : public PdfFontMetrics
{
    friend class PdfFontMetricsStandard14;
    friend class PdfFontMetricsObject;

private:
    PdfFontMetricsBase();

public:
    ~PdfFontMetricsBase();

protected:
    const datahandle& GetFontFileDataHandle() const override final;
    FT_Face GetFaceHandle() const override final;
    virtual datahandle getFontFileDataHandle() const = 0;

private:
    bool m_dataInit;
    bool m_faceInit;
    datahandle m_Data;
    FT_Face m_Face;
};



};

#endif // PDF_FONT_METRICS_H
