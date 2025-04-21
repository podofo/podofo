/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_ENCODING_H
#define PDF_ENCODING_H

#include "PdfEncodingMap.h"
#include "PdfString.h"
#include "PdfObject.h"
#include "PdfCIDToGIDMap.h"

namespace PoDoFo
{
    class PdfFont;
    class PdfEncoding;
    class PdfFontSimple;

    /** A PDF string context to iteratively scan a string
     * and collect both CID and unicode codepoints
     */
    class PODOFO_API PdfStringScanContext
    {
        friend class PdfEncoding;

    private:
        PdfStringScanContext(const std::string_view& encodedstr, const PdfEncoding& encoding);

    public:
        bool IsEndOfString() const;

        /** Advance string reading
         * \return true if success
         */
        bool TryScan(PdfCID& cid, std::string& utf8str, CodePointSpan& codepoints);

    private:
        std::string_view::iterator m_it;
        std::string_view::iterator m_end;
        const PdfEncodingMap* m_encoding;
        PdfEncodingLimits m_limits;
        const PdfEncodingMap* m_toUnicode;
    };

    /**
     * A PdfEncoding is in PdfFont to transform a text string
     * into a representation so that it can be displayed in a
     * PDF file.
     *
     * PdfEncoding can also be used to convert strings from a
     * PDF file back into a PdfString.
     */
    class PODOFO_API PdfEncoding final
    {
        friend class PdfEncodingFactory;
        friend class PdfFont;
        friend class PdfFontCID;
        friend class PdfFontCIDTrueType;
        friend class PdfFontSimple;

    public:
        /** Null encoding, when used as an actual encoding a dynamic
         * encoding will be constructed instead
         */
        PdfEncoding();
        PdfEncoding(PdfEncodingMapConstPtr encoding, PdfToUnicodeMapConstPtr toUnicode = nullptr);
        PdfEncoding(const PdfEncoding&) = default;

    private:
        PdfEncoding(unsigned id, PdfEncodingMapConstPtr&& encoding,
            PdfEncodingMapConstPtr&& toUnicode);
        PdfEncoding(unsigned id, bool isObjectLoaded, const PdfEncodingLimits& limits, PdfFont* font,
            PdfEncodingMapConstPtr&& encoding, PdfEncodingMapConstPtr&& toUnicode,
            PdfCIDToGIDMapConstPtr&& cidToGidMap);

        /** Create an encoding from object parsed information
         */
        static PdfEncoding Create(const PdfEncodingLimits& parsedLimits, PdfEncodingMapConstPtr&& encoding,
            PdfEncodingMapConstPtr&& toUnicode, PdfCIDToGIDMapConstPtr&& cidToGidMap);

        /** Create a proxy encoding with a supplied /ToUnicode map
         */
        static PdfEncoding Create(const PdfEncoding& ref, PdfToUnicodeMapConstPtr&& toUnicode);

        /** Encoding shim that mocks an wrap existing encoding. Used by PdfFont
         */
        static std::unique_ptr<PdfEncoding> CreateSchim(const PdfEncoding& encoding, PdfFont& font);

        /** Encoding with an external encoding map storage
         * Used by PdfFont in case of dynamic encoding requested
         */
        static std::unique_ptr<PdfEncoding> CreateDynamicEncoding(const std::shared_ptr<PdfCharCodeMap>& cidMap,
            const std::shared_ptr<PdfCharCodeMap>& toUnicodeMap, PdfFont& font);

    public:
        /**
         * \remarks Doesn't throw if conversion failed, totally or partially
         */
        std::string ConvertToUtf8(const PdfString& encodedStr) const;

        /**
         * \remarks Produces a partial result also in case of failure
         */
        bool TryConvertToUtf8(const PdfString& encodedStr, std::string& str) const;

        /**
         * \remarks It throws if conversion failed, totally or partially
         */
        charbuff ConvertToEncoded(const std::string_view& str) const;

        bool TryConvertToEncoded(const std::string_view& str, charbuff& encoded) const;

        /**
         * \remarks Doesn't throw if conversion failed, totally or partially
         */
        std::vector<PdfCID> ConvertToCIDs(const PdfString& encodedStr) const;

        /**
         * \remarks Produces a partial result also in case of failure
         */
        bool TryConvertToCIDs(const PdfString& encodedStr, std::vector<PdfCID>& cids) const;

        /** Get code point from char code unit
         *
         * \returns the found code point or U'\0' if missing or
         *      multiple matched codepoints
         */
        char32_t GetCodePoint(const PdfCharCode& codeUnit) const;

        /** Get code point from char code
         *
         * \returns the found code point or U'\0' if missing or
         *      multiple matched codepoints
         * \remarks it will iterate available code sizes
         */
        char32_t GetCodePoint(unsigned charCode) const;

        PdfStringScanContext StartStringScan(const PdfString& encodedStr);

    public:
        /** This return the first char code used in the encoding
         * \remarks Mostly useful for non cid-keyed fonts to export /FirstChar
         */
        const PdfCharCode& GetFirstChar() const;

        /** This return the last char code used in the encoding
         * \remarks Mostly useful for non cid-keyed fonts to export /LastChar
         */
        const PdfCharCode& GetLastChar() const;

        /** Return true if the encoding is a dummy null encoding
         */
        bool IsNull() const;

        /** Return true if the encoding does CID mapping
         */
        bool HasCIDMapping() const;

        /** Return true if the encoding is simple
         * and has a non-CID mapping /Encoding entry
         */
        bool IsSimpleEncoding() const;

        /** Returns true if /FirstChar and /LastChar were parsed from object
         */
        bool HasParsedLimits() const;

        /** Return true if the encoding is a dynamic CID mapping
         */
        bool IsDynamicEncoding() const;

        /**
         * Return an Id to be used in hashed containers
         */
        unsigned GetId() const { return m_Id; }

        /**
         * True if the encoding is constructed from object loaded information
         */
        bool IsObjectLoaded() const { return m_IsObjectLoaded; }

        /** Get actual limits of the encoding
         *
         * May be the limits inferred from /Encoding or the limits inferred by /FirstChar, /LastChar
         */
        const PdfEncodingLimits& GetLimits() const;

        bool HasValidToUnicodeMap() const;

        /** Get the ToUnicode map, throws if missing
         */
        const PdfEncodingMap& GetToUnicodeMap() const;

        /** Get the ToUnicode map, fallback to the normal encoding if missing
         *
         * \param toUnicode the retrieved map
         * \return true if the retrieved map is valid, false otherwise
         */
        bool GetToUnicodeMapSafe(const PdfEncodingMap*& toUnicode) const;

        /** Get the ToUnicode map, fallback to the normal encoding if missing
         *
         * \return the retrieved map
         * \remark As a general rule, we always use this method when converting encoded -> Unicode
         */
        const PdfEncodingMap& GetToUnicodeMapSafe() const;

        const PdfEncodingMap& GetEncodingMap() const { return *m_Encoding; }

        PdfEncodingMapConstPtr GetEncodingMapPtr() const { return m_Encoding; }

        PdfEncodingMapConstPtr GetToUnicodeMapPtr() const;

    public:
        PdfEncoding& operator=(const PdfEncoding&) = default;

    private:
        // These methods will be called by PdfFont
        void ExportToFont(PdfFont& font, const PdfCIDSystemInfo& cidInfo) const;
        void ExportToFont(PdfFont& font) const;
        bool TryGetCIDId(const PdfCharCode& codeUnit, unsigned& cid) const;
        const PdfCIDToGIDMap* GetCIDToGIDMap() const { return m_CIDToGIDMap.get(); }

        static unsigned GetNextId();

    private:
        void exportToFont(PdfFont& font, const PdfCIDSystemInfo* cidInfo) const;
        bool tryExportEncodingTo(PdfDictionary& dictionary, bool wantCidMapping) const;
        bool tryConvertEncodedToUtf8(const std::string_view& encoded, std::string& str) const;
        bool tryConvertEncodedToCIDs(const std::string_view& encoded, std::vector<PdfCID>& cids) const;
        void writeCIDMapping(PdfObject& cmapObj, const PdfFont& font, const PdfCIDSystemInfo& info) const;
        void writeToUnicodeCMap(PdfObject& cmapObj, const PdfFont& font) const;
        bool tryGetCharCode(PdfFont& font, unsigned gid, const unicodeview& codePoints, PdfCharCode& unit) const;

    private:
        unsigned m_Id;
        bool m_IsObjectLoaded;
        PdfEncodingLimits m_ParsedLimits;
        PdfFont* m_Font;
        PdfEncodingMapConstPtr m_Encoding;
        PdfEncodingMapConstPtr m_ToUnicode;
        PdfCIDToGIDMapConstPtr m_CIDToGIDMap;
    };
}

#endif // PDF_ENCODING_H
