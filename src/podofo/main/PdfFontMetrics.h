/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_FONT_METRICS_H
#define PDF_FONT_METRICS_H

#include "PdfDeclarations.h"

#include "PdfString.h"
#include "PdfCMapEncoding.h"
#include "PdfCIDToGIDMap.h"

FORWARD_DECLARE_FREETYPE();

namespace PoDoFo {

class PODOFO_API FreeTypeFacePtr final : public std::shared_ptr<FT_FaceRec_>
{
public:
    FreeTypeFacePtr();
    FreeTypeFacePtr(FT_Face face);
    FreeTypeFacePtr(const FreeTypeFacePtr&) = default;
    FreeTypeFacePtr& operator=(const FreeTypeFacePtr&) = default;
    void reset(FT_Face face = nullptr);
};

/**
 * This abstract class provides access to font metrics information.
 *
 * The class doesn't know anything about CIDs (Character IDs),
 * it just index glyphs, or GIDs where the terminology applies
 */
class PODOFO_API PdfFontMetrics
{
    friend class PdfFont;
    friend class PdfFontManager;
    friend class PdfFontMetricsFreetype;

protected:
    PdfFontMetrics();

public:
    virtual ~PdfFontMetrics();

    virtual unsigned GetGlyphCount() const = 0;

    /** Get the width of a single glyph id
     *
     *  \param gid id of the glyph
     *  \returns the width of a single glyph id
     */
    double GetGlyphWidth(unsigned gid) const;
    virtual bool TryGetGlyphWidth(unsigned gid, double& width) const = 0;

    /**
     * Some fonts provides a glyph subsitution list, eg. for ligatures.
     * OpenType fonts for example provides GSUB "Glyph Substitution Table"
     * \param gids gids to be substituded
     * \param backwardMap list of gid counts to remap back substituded gids
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
     * \remarks dont' use this method directly unless you know
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

    /** Return the position of the strikeout for the current font
     *  size in PDF units
     *  \returns the underline position in PDF units
     */
    virtual double GetStrikeOutPosition() const = 0;

    /** Get the width of the strikeout for the current
     *  font size in PDF units
     *  \returns the thickness of the strikeout in PDF units
     */
    virtual double GetStrikeOutThickness() const = 0;

    virtual PdfFontFileType GetFontFileType() const = 0;

    bool HasFontFileData() const;

    /** Get an actual font data view
     *
     * The data shall be resident. For font coming from the /FontFile
     * keys, GetFontFileObject() may also be available.
     * \returns a binary buffer of data containing the font data
     */
    bufferview GetOrLoadFontFileData() const;

    /** Get direct access to the internal FreeType handle
     *
     *  \returns the internal freetype handle
     */
    bool TryGetOrLoadFace(FT_Face& face) const;
    FT_Face GetOrLoadFace() const;

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

    /** Get a string with either the actual /BaseFont, /FontName or
     * /FontFamily name, depending on exsistences of such entries
     */
    std::string_view GetFontNameSafe(bool familyFirst = false) const;

    /** Get a semantical base name for the font that can be used to
     * compose the final name, eg. from "AAAAAA+Arial,Bold" to "Arial"
     *
     * The string is constructed either from the actual /BaseFont,
     * /FontName or /FontFamily name, depending on exsistences of
     * such entries
     * \remarks It doesn't correspond to /BaseFont name entry in the font
     */
    std::string_view GetBaseFontNameSafe() const;

    /** Get a semantical base name for the font that can be used to
     * compose the final name, eg. from "AAAAAA+Arial,Bold" to "Arial"
     *
     * \remarks It doesn't correspond to /BaseFont name entry in the font
     */
    virtual std::string_view GetBaseFontName() const = 0;

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

    virtual PdfFontStretch GetFontStretch() const = 0;

    /** Get the weight of this font.
     *  \returns the weight of this font (400 <= x < 700 means normal, x >= 700 means bold)
     */
    unsigned GetWeight() const;
    virtual int GetWeightRaw() const = 0;

    virtual PdfFontDescriptorFlags GetFlags() const = 0;

    /** Create the bounding box vector in PDF units
     *
     *  \param bbox write the bounding box to this vector
     */
    virtual void GetBoundingBox(std::vector<double>& bbox) const = 0;

    /** Get the italic angle of this font.
     *  Used to build the font dictionay
     *  \returns the italic angle of this font.
     */
    virtual double GetItalicAngle() const = 0;

    /** Get the ascent of this font in PDF
     *  units for the current font size.
     *
     * \returns the ascender for this font
     *
     * \see GetAscent
     */
    virtual double GetAscent() const = 0;

    /** Get the descent of this font in PDF
     *  units for the current font size.
     *  This value is usually negative!

     *  \returns the descender for this font
     *
     *  \see GetDescent
     */
    virtual double GetDescent() const = 0;

    /* /Leading (optional, default 0)
     */
    double GetLeading() const;
    virtual double GetLeadingRaw() const = 0;

    /** The vertical coordinate of the top of flat capital letters, measured from the baseline
     */
    virtual double GetCapHeight() const = 0;

    /** The font’s x height: the vertical coordinate of the top of flat nonascending
     * lowercase letters (like the letter x), measured from the baseline, in
     * fonts that have Latin characters (optional, default 0)
     */
    double GetXHeight() const;
    virtual double GetXHeightRaw() const = 0;

    /** The thickness, measured horizontally, of the dominant vertical stems of glyphs in the font
     */
    virtual double GetStemV() const = 0;

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

    bool IsStandard14FontMetrics() const;

    virtual bool IsStandard14FontMetrics(PdfStandard14FontType& std14Font) const;

    /** Returns the matrix mapping glyph space to text space
     */
    virtual const Matrix2D& GetMatrix() const;

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
     * Thi is implemented just for PdfFontMetricsFreetype
     * This map may be unreliable because of ligatures,
     * other kind of character subsitutions, or glyphs
     * mapping to multiple unicode codepoints.
     */
    virtual std::unique_ptr<PdfCMapEncoding> CreateToUnicodeMap(const PdfEncodingLimits& limitHints) const;

    /** Get an implicit encoding, such as the one of standard14 fonts,
     * or the built-in encoding of a Type1 font, if available
     */
    bool TryGetImplicitEncoding(PdfEncodingMapConstPtr &encoding) const;

    PdfCIDToGIDMapConstPtr GetCIDToGIDMap() const;

public:
    const std::string& GetFilePath() const { return m_FilePath; }
    unsigned GetFaceIndex() const { return m_FaceIndex; }

protected:
    virtual const PdfCIDToGIDMapConstPtr& getCIDToGIDMap() const;
    virtual bool getIsBoldHint() const = 0;
    virtual bool getIsItalicHint() const = 0;
    virtual const datahandle& GetFontFileDataHandle() const = 0;
    virtual const FreeTypeFacePtr& GetFaceHandle() const = 0;

private:
    void SetFilePath(std::string&& filepath, unsigned faceIndex);

private:
    void initBaseFontNameSafe();
    static PdfEncodingMapConstPtr getFontType1Encoding(FT_Face face);

private:
    PdfFontMetrics(const PdfFontMetrics& rhs) = delete;
    PdfFontMetrics& operator=(const PdfFontMetrics& rhs) = delete;

private:
    std::string m_FilePath;
    nullable<PdfFontStyle> m_Style;
    std::unique_ptr<std::string> m_BaseFontNameSafe;
    unsigned m_FaceIndex;
};

class PODOFO_API PdfFontMetricsBase : public PdfFontMetrics
{
protected:
    PdfFontMetricsBase();

protected:
    const datahandle& GetFontFileDataHandle() const override final;
    const FreeTypeFacePtr& GetFaceHandle() const override final;
    virtual datahandle getFontFileDataHandle() const = 0;

private:
    bool m_dataInit;
    datahandle m_Data;
    bool m_faceInit;
    FreeTypeFacePtr m_Face;
};

/** Convenience typedef for a const PdfEncoding shared ptr
 */
using PdfFontMetricsConstPtr = std::shared_ptr<const PdfFontMetrics> ;

};

#endif // PDF_FONT_METRICS_H
