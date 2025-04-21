/**
 * SPDX-FileCopyrightText: (C) 2008 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_DIFFERENCE_ENCODING_H
#define PDF_DIFFERENCE_ENCODING_H

#include "PdfEncodingMap.h"
#include "PdfArray.h"

namespace PoDoFo {

class PdfFontMetrics;
struct CodePointMapNode;

struct PdfDifferenceMapping
{
    PdfName Name;
    unsigned char Code = 0;
    CodePointSpan CodePoints;
};

/** A helper class for PdfDifferenceEncoding that
 *  can be used to create a differences array.
 */
class PODOFO_API PdfDifferenceList final
{
    friend class PdfDifferenceEncoding;
public:
    using const_iterator = std::vector<PdfDifferenceMapping>::const_iterator;

public:
    /** Create a PdfEncodingDifference object.
     */
    PdfDifferenceList();

    PdfDifferenceList(const PdfDifferenceList& rhs) = default;
    PdfDifferenceList& operator=(const PdfDifferenceList& rhs) = default;

    /** Add a difference to the encoding
     *
     * The added name is determined by the "Adobe Glyph List for New Fonts"
     * https://github.com/adobe-type-tools/agl-aglfn/blob/master/aglfn.txt
     * \param code code unit of the difference (0 to 255 are legal values)
     * \param codePoint actual unicode code point
     *
     */
    void AddDifference(unsigned char code, char32_t codePoint);

    /** Add a difference to the encoding
     *
     * The added name is determined by the "Adobe Glyph List for New Fonts"
     * https://github.com/adobe-type-tools/agl-aglfn/blob/master/aglfn.txt
     *  \param code code unit of the difference (0 to 255 are legal values)
     *  \param codePoints a span of unicode code points
     *
     */
    void AddDifference(unsigned char code, const codepointview& codePoints);

    /** Get the mapped code point from a char code
     *
     *  \param code test if the given code is part of the differences
     *  \param codePoints write the associated unicode values of the name to this value
     *
     *  \returns true if the code is part of the difference
     */
    bool TryGetMappedName(unsigned char code, const PdfName*& name) const;
    bool TryGetMappedName(unsigned char code, const PdfName*& name, CodePointSpan& codePoints) const;

    /** Convert the PdfEncodingDifference to an array
     *
     *  \param arr write to this array
     */
    void ToArray(PdfArray& arr) const;

    /** Get the number of differences in this object.
     *  If the user added .notdef as a difference it is
     *  counted, even it is no real difference in the final encoding.
     *
     *  \returns the number of differences in this object
     */
    unsigned GetCount() const;

    const_iterator begin() const { return m_differences.begin(); }

    const_iterator end() const { return m_differences.end(); }

private:
    /**
     * \param explicitNames if true, the unicode value is set to nCode as name is meaningless (Type3 fonts)
     */
    void AddDifference(unsigned char code, const std::string_view& name, bool explicitNames);

    void addDifference(unsigned char code, const codepointview& codepoints, const PdfName& name);

    struct DifferenceComparatorPredicate
    {
    public:
        bool operator()(const PdfDifferenceMapping& diff1, const PdfDifferenceMapping& diff2) const
        {
            return diff1.Code < diff2.Code;
        }
    };

    using DifferenceList = std::vector<PdfDifferenceMapping>;

private:
    DifferenceList m_differences;
};

/** PdfDifferenceEncoding is an encoding, which is based
 *  on either the fonts encoding or a predefined encoding
 *  and defines differences to this base encoding.
 */
class PODOFO_API PdfDifferenceEncoding final : public PdfEncodingMapOneByte
{
    friend class PdfDifferenceList;

public:
    /** Create a new PdfDifferenceEncoding which is based on
     *  a predefined encoding.
     *
     *  \param difference the differences in this encoding
     *  \param baseEncoding the base encoding of this font
     */
    PdfDifferenceEncoding(PdfEncodingMapConstPtr baseEncoding,
        PdfDifferenceList differences);

    ~PdfDifferenceEncoding();

public:
    /** Create a new PdfDifferenceEncoding from an existing object
     *
     *  \param obj object for the difference encoding
     *  \param metrics an existing font metrics
     */
    static bool TryCreateFromObject(const PdfObject& obj, const PdfFontMetrics& metrics,
        std::unique_ptr<PdfDifferenceEncoding>& encoding);

    /** Create a new PdfDifferenceEncoding from an existing object
     *
     * \param obj object for the difference encoding
     * \param metrics an existing font metrics
     * \returns On success, returns a non null PdfDifferenceEncoding
     * \remarks throws on failure
     */
    static std::unique_ptr<PdfDifferenceEncoding> CreateFromObject(const PdfObject& obj, const PdfFontMetrics& metrics);

    /** Try to convert a standard character name to a unicode code points
     *
     * \param name a standard character name.
     *   See https://github.com/adobe-type-tools/agl-aglfn/ for known names
     * \param codepoints the returned unicode code points span
     */
    static bool TryGetCodePointsFromCharName(const std::string_view& name, CodePointSpan& codepoints);

    /**
     * Get read-only access to the object containing the actual
     * differences.
     *
     * \returns the container with the actual differences
     */
    const PdfDifferenceList& GetDifferences() const { return m_differences; }

protected:
    PdfCIDToGIDMapConstPtr GetIntrinsicCIDToGIDMap(const PdfDictionary& fontDict, const PdfFontMetrics& metrics) const override;

    void getExportObject(PdfIndirectObjectList& objects, PdfName& name, PdfObject*& obj) const override;
    bool tryGetCharCode(char32_t codePoint, PdfCharCode& codeUnit) const override;
    bool tryGetCharCodeSpan(const unicodeview& codePoints, PdfCharCode& codeUnit) const override;
    bool tryGetNextCharCode(std::string_view::iterator& it,
        const std::string_view::iterator& end, PdfCharCode& codeUnit) const override;
    bool tryGetCodePoints(const PdfCharCode& codeUnit, const unsigned* cidId, CodePointSpan& codePoints) const override;

private:
    static bool TryGetCodePointsFromCharName(std::string_view charName, CodePointSpan& codepoints, const PdfName*& actualName);

    PdfCIDToGIDMapConstPtr getIntrinsicCIDToGIDMapType1(const PdfFontMetrics& metrics) const;
    PdfCIDToGIDMapConstPtr getIntrinsicCIDToGIDMapTrueType(const PdfFontMetrics& metrics) const;
    void buildReverseMap();

private:
    PdfEncodingMapConstPtr m_baseEncoding;
    PdfDifferenceList m_differences;
    CodePointMapNode* m_reverseMap;
};

};

#endif // PDF_DIFFERENCE_ENCODING_H
