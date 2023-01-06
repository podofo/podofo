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

namespace PoDoFo
{
    class PdfFont;
    class PdfEncoding;
    class PdfFontSimple;

    enum class PdfEncodingExportFlags
    {
        None = 0,
        SkipToUnicode = 1,  ///< Skip exporting a /ToUnicode entry
    };

    /** A PDF string context to interatively scan a string
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
        bool TryScan(PdfCID& cid, std::string& utf8str, std::vector<codepoint>& codepoints);

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
    class PODOFO_API PdfEncoding
    {
        friend class PdfEncodingFactory;
        friend class PdfEncodingShim;
        friend class PdfDynamicEncoding;
        friend class PdfFont;
        friend class PdfFontSimple;

    public:
        /** Null encoding
         */
        PdfEncoding();
        PdfEncoding(const PdfEncodingMapConstPtr& encoding, const PdfToUnicodeMapConstPtr& toUnicode = nullptr);
        virtual ~PdfEncoding();

    private:
        PdfEncoding(size_t id, const PdfEncodingMapConstPtr& encoding, const PdfEncodingMapConstPtr& toUnicode = nullptr);
        PdfEncoding(const PdfObject& fontObj, const PdfEncodingMapConstPtr& encoding, const PdfEncodingMapConstPtr& toUnicode);

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

        void ExportToFont(PdfFont& font, PdfEncodingExportFlags flags = { }) const;

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
        virtual bool IsDynamicEncoding() const;

        /**
         * Return an Id to be used in hashed containers.
         * Id 0 has a special meaning to create a PdfDynamicEncoding
         *  \see PdfDynamicEncoding
         */
        size_t GetId() const { return m_Id; }

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

        inline const PdfEncodingMap& GetEncodingMap() const { return *m_Encoding; }

        inline const PdfEncodingMapConstPtr GetEncodingMapPtr() const { return m_Encoding; }

        const PdfEncodingMapConstPtr GetToUnicodeMapPtr() const;

    public:
        PdfEncoding& operator=(const PdfEncoding&) = default;
        PdfEncoding(const PdfEncoding&) = default;

    protected:
        virtual PdfFont & GetFont() const;

    private:
        // This method is to be called by PdfFont
        bool TryGetCIDId(const PdfCharCode& codeUnit, unsigned& cid) const;
        static size_t GetNextId();

    private:
        bool tryExportObjectTo(PdfDictionary& dictionary, bool wantCidMapping) const;
        bool tryConvertEncodedToUtf8(const std::string_view& encoded, std::string& str) const;
        bool tryConvertEncodedToCIDs(const std::string_view& encoded, std::vector<PdfCID>& cids) const;
        void writeCIDMapping(PdfObject& cmapObj, const PdfFont& font, const std::string_view& baseFont) const;
        void writeToUnicodeCMap(PdfObject& cmapObj) const;
        bool tryGetCharCode(PdfFont& font, unsigned gid, const unicodeview& codePoints, PdfCharCode& unit) const;

    private:
        size_t m_Id;
        PdfEncodingMapConstPtr m_Encoding;
        PdfEncodingMapConstPtr m_ToUnicode;
        PdfEncodingLimits m_ParsedLimits;
    };
}

ENABLE_BITMASK_OPERATORS(PoDoFo::PdfEncodingExportFlags);

#endif // PDF_ENCODING_H
