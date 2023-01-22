/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_ENCODING_MAP_H
#define PDF_ENCODING_MAP_H

#include "PdfDeclarations.h"
#include "PdfObject.h"
#include "PdfName.h"
#include "PdfCharCodeMap.h"

namespace PoDoFo {

class PdfIndirectObjectList;
class PdfDynamicEncoding;
class PdfDynamicCIDEncoding;
class PdfFont;

/** 
 * A PdfEncodingMap is a low level interface to convert
 * between utf8 and encoded strings in and to determine
 * correct CID mapping
 * \remarks Prefer using PdfEncoding methods instead:
 * don't use this class directly unless you know what
 * you are doing
 */
class PODOFO_API PdfEncodingMap
{
    friend class PdfEncoding;

protected:
    PdfEncodingMap(PdfEncodingMapType type);

public:
    /** Try decode next char code from utf8 string range
     */
    bool TryGetNextCharCode(std::string_view::iterator& it,
        const std::string_view::iterator& end, PdfCharCode& codeUnit) const;

    /**
     * Try get next char code unit from unicode code point
     */
    bool TryGetCharCode(char32_t codePoint, PdfCharCode& codeUnit) const;

    /**
     * Get the char code from a span of unicode code points
     * \param codePoints it can be a single code point or a ligature
     * \return true if the code points match a character code
     */
    bool TryGetCharCode(const unicodeview& codePoints, PdfCharCode& codeUnit) const;

    /**
     * Try get next char code unit from cid
     */
    bool TryGetCharCode(unsigned cid, PdfCharCode& codeUnit) const;

    /** Try decode next cid from from encoded string range
     */
    bool TryGetNextCID(std::string_view::iterator& it,
        const std::string_view::iterator& end, PdfCID& cid) const;

    /** Try decode next code points from encoded string range
     */
    bool TryGetNextCodePoints(std::string_view::iterator& it,
        const std::string_view::iterator& end, std::vector<char32_t>& codePoints) const;

    /** Try get code points from char code unit
     *
     * \remarks it will iterate available code sizes
     */
    bool TryGetCodePoints(const PdfCharCode& codeUnit, std::vector<char32_t>& codePoints) const;

    virtual const PdfEncodingLimits& GetLimits() const = 0;

    /**
     * Type of encoding, may be Simple or CMap
     *
     * Simple: built-in, difference and Type1 implicit encodings
     * CMap: proper CMap or PdfIndentityEncoding and other
     * predefined CMap names as well (ISO 32000-1:2008 Table 118
     * Predefined CJK CMap names, currently not implemented)
     * \remarks This is a low level information. Use PdfEncoding::IsSimpleEncoding()
     * to dermine if the encoding is really a simple one
     */
    PdfEncodingMapType GetType() const { return m_Type; }

    /**
     * True if the encoding is builtin in a font program
     */
    virtual bool IsBuiltinEncoding() const;

    /**
     * True if the encoding has ligatures support
     */
    virtual bool HasLigaturesSupport() const;

    /** Get an export object that will be used during font init
     * \param objects list to use to create document objects
     * \param name name to use
     * \param obj if not null the object will be used instead
     */
    bool TryGetExportObject(PdfIndirectObjectList& objects, PdfName& name, PdfObject*& obj) const;

public:
    virtual ~PdfEncodingMap();

protected:
    /**
     * Try get next char code unit from a utf8 string range
     *
     * \remarks Default implementation just throws
     */
    virtual bool tryGetNextCharCode(std::string_view::iterator& it,
        const std::string_view::iterator& end, PdfCharCode& codeUnit) const;

    /**
     * Try get next char code unit from a ligature
     * \param ligature the span has at least 2 unicode code points
     * \remarks Default implementation just throws
     */
    virtual bool tryGetCharCodeSpan(const unicodeview& ligature, PdfCharCode& codeUnit) const;

    /**
     * Try get char code unit from unicode code point
     */
    virtual bool tryGetCharCode(char32_t codePoint, PdfCharCode& codeUnit) const = 0;

    /**
     * Get code points from a code unit
     *
     * \param wantCID true requires mapping to CID identifier, false for Unicode code points
     */
    virtual bool tryGetCodePoints(const PdfCharCode& codeUnit, std::vector<char32_t>& codePoints) const = 0;

    /** Get an export object that will be used during font init
     *
     * \remarks Default implementation just throws
     */
    virtual void getExportObject(PdfIndirectObjectList& objects, PdfName& name, PdfObject*& obj) const;

    static void AppendUTF16CodeTo(OutputStream& stream, char32_t codePoint, std::u16string& u16tmp);

    static void AppendUTF16CodeTo(OutputStream& stream, const unicodeview& codePoints, std::u16string& u16tmp);

protected:
    virtual void AppendCodeSpaceRange(OutputStream& stream, charbuff& temp) const;

    /** During a WriteToUnicodeCMap append "beginbfchar" and "beginbfrange"
     * entries. "bf" stands for Base Font, see Adobe tecnichal notes #5014
     *
     * To be called by PdfEncoding
     */
    virtual void AppendToUnicodeEntries(OutputStream& stream, charbuff& temp) const = 0;

    /** During a PdfEncoding::ExportToFont() append "begincidchar"
     * and/or "begincidrange" entries. See Adobe tecnichal notes #5014\
     *
     * To be called by PdfEncoding
     */
    virtual void AppendCIDMappingEntries(OutputStream& stream, const PdfFont& font, charbuff& temp) const = 0;

private:
    /** Try get CID identifier code from code unit
     * \param id the identifier of the CID. The identifier is
     * actually the PdfCID::Id part in the full CID representation
     *
     * To be called by PdfEncoding
     */
    bool TryGetCIDId(const PdfCharCode& codeUnit, unsigned& id) const;

    bool tryGetNextCodePoints(std::string_view::iterator& it, const std::string_view::iterator& end,
        PdfCharCode& codeUnit, std::vector<char32_t>& codePoints) const;

private:
    PdfEncodingMapType m_Type;
};

/**
 * Basic PdfEncodingMap implementation using a PdfCharCodeMap
 */
class PODOFO_API PdfEncodingMapBase : public PdfEncodingMap
{
    friend class PdfDynamicEncodingMap;

protected:
    PdfEncodingMapBase(PdfCharCodeMap&& map, PdfEncodingMapType type);

protected:
    bool tryGetNextCharCode(std::string_view::iterator& it,
        const std::string_view::iterator& end, PdfCharCode& codeUnit) const override;

    bool tryGetCharCodeSpan(const unicodeview& codePoints, PdfCharCode& codeUnit) const override;

    bool tryGetCharCode(char32_t codePoint, PdfCharCode& codeUnit) const override;

    bool tryGetCodePoints(const PdfCharCode& codeUnit, std::vector<char32_t>& codePoints) const override;

    void AppendCodeSpaceRange(OutputStream& stream, charbuff& temp) const override;

    void AppendToUnicodeEntries(OutputStream& stream, charbuff& temp) const override;

    void AppendCIDMappingEntries(OutputStream& stream, const PdfFont& font, charbuff& temp) const override;

public:
    inline const PdfCharCodeMap& GetCharMap() const { return *m_charMap; }

    const PdfEncodingLimits& GetLimits() const override;

private:
    PdfEncodingMapBase(const std::shared_ptr<PdfCharCodeMap>& map, PdfEncodingMapType type);

private:
    std::shared_ptr<PdfCharCodeMap> m_charMap;
};

/**
 * PdfEncodingMap used by encodings like PdfBuiltInEncoding
 * or PdfDifferenceEncoding thats can define all their charset
 * with a single one byte range
 */
class PODOFO_API PdfEncodingMapOneByte : public PdfEncodingMap
{
protected:
    PdfEncodingMapOneByte(const PdfEncodingLimits& limits);

    void AppendToUnicodeEntries(OutputStream& stream, charbuff& temp) const override;

    void AppendCIDMappingEntries(OutputStream& stream, const PdfFont& font, charbuff& temp) const override;

    const PdfEncodingLimits& GetLimits() const override;

private:
    PdfEncodingLimits m_Limits;
};

/**
 * A common base class for built-in encodings which are
 * known by name.
 */
class PODOFO_API PdfBuiltInEncoding : public PdfEncodingMapOneByte
{
protected:
    PdfBuiltInEncoding(const PdfName& name);

public:
    /** Get the name of this encoding.
     *
     *  \returns the name of this encoding.
     */
    inline const PdfName& GetName() const { return m_Name; }

protected:
    bool tryGetCharCode(char32_t codePoint, PdfCharCode& codeUnit) const override;
    bool tryGetCodePoints(const PdfCharCode& codeUnit, std::vector<char32_t>& codePoints) const override;

    /** Gets a table of 256 short values which are the
     *  big endian Unicode code points that are assigned
     *  to the 256 values of this encoding.
     *
     *  This table is used internally to convert an encoded
     *  string of this encoding to and from Unicode.
     *
     *  \returns an array of 256 big endian Unicode code points
     */
    virtual const char32_t* GetToUnicodeTable() const = 0;

private:
    /** Initialize the internal table of mappings from Unicode code points
     *  to encoded byte values.
     */
    void InitEncodingTable();

private:
    PdfName m_Name;         // The name of the encoding
    std::unordered_map<char32_t, char> m_EncodingTable; // The helper table for conversions into this encoding
};

/** Dummy encoding map that will just throw exception
 */
class PODOFO_API PdfNullEncodingMap final : public PdfEncodingMap
{
    friend class PdfEncodingMapFactory;

private:
    PdfNullEncodingMap();

public:
    const PdfEncodingLimits& GetLimits() const override;

protected:
    bool tryGetCharCode(char32_t codePoint, PdfCharCode& codeUnit) const override;

    bool tryGetCodePoints(const PdfCharCode& codeUnit, std::vector<char32_t>& codePoints) const override;

    void AppendToUnicodeEntries(OutputStream& stream, charbuff& temp) const override;

    void AppendCIDMappingEntries(OutputStream& stream, const PdfFont& font, charbuff& temp) const override;
};

/** Convenience typedef for a const /Encoding map entry shared ptr
 */
using PdfEncodingMapConstPtr = std::shared_ptr<const PdfEncodingMap>;

class PdfCMapEncoding;

/** Convenience alias for a const /ToUnicode CMap entry shared ptr
 */
using PdfToUnicodeMapConstPtr = std::shared_ptr<const PdfEncodingMap>;
}

#endif // PDF_ENCODING_MAP_H
