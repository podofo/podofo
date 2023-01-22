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

/** A helper class for PdfDifferenceEncoding that
 *  can be used to create a differences array.
 */
class PODOFO_API PdfDifferenceList
{
    struct Difference
    {
        unsigned char Code = 0;
        PdfName Name;
        char32_t MappedCodePoint = U'\0';
    };

    using List = std::vector<Difference>;
    using iterator = std::vector<Difference>::iterator;
    using const_iterator = std::vector<Difference>::const_iterator;

public:
    /** Create a PdfEncodingDifference object.
     */
    PdfDifferenceList();

    PdfDifferenceList(const PdfDifferenceList& rhs) = default;
    PdfDifferenceList& operator=(const PdfDifferenceList& rhs) = default;

    /** Add a difference to the object.
     *
     *  \param nCode unicode code point of the difference (0 to 255 are legal values)
     *  \param unicodeValue actual unicode value for nCode; can be 0
     *
     *  \see AddDifference if you know the name of the code point
     *       use the overload below which is faster
     */
    void AddDifference(unsigned char code, char32_t codePoint);

    /** Add a difference to the object.
     *
     *  \param name unicode code point of the difference (0 to 255 are legal values)
     *  \param name name of the different code point or .notdef if none
     *  \param explicitNames if true, the unicode value is set to nCode as name is meaningless (Type3 fonts)
     */
    void AddDifference(unsigned char code, const PdfName& name, bool explicitNames = false);

    /** Get the mapped code point from a char code
     *
     *  \param code test if the given code is part of the differences
     *  \param codePoint write the associated unicode value of the name to this value
     *
     *  \returns true if the code is part of the difference
     */
    bool TryGetMappedName(unsigned char code, const PdfName*& name) const;
    bool TryGetMappedName(unsigned char code, const PdfName*& name, char32_t& codePoint) const;

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
    size_t GetCount() const;

    const_iterator begin() const { return m_differences.begin(); }

    const_iterator end() const { return m_differences.end(); }

private:
    void addDifference(unsigned char code, char32_t codePoint, const PdfName& name);
    bool contains(unsigned char code, const PdfName*& name, char32_t& codePoint);

    struct DifferenceComparatorPredicate
    {
    public:
        inline bool operator()(const Difference& diff1, const Difference& diff2) const
        {
            return diff1.Code < diff2.Code;
        }
    };

    List m_differences;
};

/** PdfDifferenceEncoding is an encoding, which is based
 *  on either the fonts encoding or a predefined encoding
 *  and defines differences to this base encoding.
 */
class PODOFO_API PdfDifferenceEncoding final : public PdfEncodingMapOneByte
{
public:
    /** Create a new PdfDifferenceEncoding which is based on
     *  a predefined encoding.
     *
     *  \param difference the differences in this encoding
     *  \param baseEncoding the base encoding of this font
     */
    PdfDifferenceEncoding(const PdfDifferenceList& difference,
        const PdfEncodingMapConstPtr& baseEncoding);

    /** Create a new PdfDifferenceEncoding from an existing object
     *  in a PDF file.
     *
     *  \param obj object for the difference encoding
     *  \param metrics an existing font metrics
     */
    static std::unique_ptr<PdfDifferenceEncoding> Create(const PdfObject& obj,
        const PdfFontMetrics& metrics);

    /** Convert a standard character name to a unicode code point
     *
     *  \param name a standard character name
     *  \returns an unicode code point
     */
    static char32_t NameToCodePoint(const PdfName& name);
    static char32_t NameToCodePoint(const std::string_view& name);

    /** Convert an unicode code point to a standard character name
     *
     *  \param codePoint a code point
     *  \returns a standard character name of /.notdef if none could be found
     */
    static PdfName CodePointToName(char32_t codePoint);

    /**
     * Get read-only access to the object containing the actual
     * differences.
     *
     * \returns the container with the actual differences
     */
    inline const PdfDifferenceList& GetDifferences() const { return m_differences; }

protected:
    void getExportObject(PdfIndirectObjectList& objects, PdfName& name, PdfObject*& obj) const override;
    bool tryGetCharCode(char32_t codePoint, PdfCharCode& codeUnit) const override;
    bool tryGetCodePoints(const PdfCharCode& codeUnit, std::vector<char32_t>& codePoints) const override;

private:
    void buildReverseMap();

private:
    PdfDifferenceList m_differences;
    PdfEncodingMapConstPtr m_baseEncoding;
    bool m_reverseMapBuilt;
    std::unordered_map<char32_t, unsigned char> m_reverseMap;
};

};

#endif // PDF_DIFFERENCE_ENCODING_H
