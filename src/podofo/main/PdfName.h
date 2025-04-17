/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_NAME_H
#define PDF_NAME_H

#include "PdfBaseDataTypes.h"

namespace PoDoFo {

/** This class represents a PdfName.
 *  Whenever a key is required you have to use a PdfName object.
 *
 *  PdfName are required as keys in PdfObject and PdfVariant objects.
 *
 *  PdfName may have a maximum length of 127 characters.
 *
 *  \see PdfObject \see PdfVariant
 */
class PODOFO_API PdfName final : private PdfDataMember, public PdfDataProvider<PdfName>
{
public:
    /** Null name, corresponds to "/"
     */
    static const PdfName Null;

public:
    /** Constructor to create null name (corresponds to "/")
     *  use PdfName::Null instead of this constructor
     */
    PdfName();

    ~PdfName();

    template<std::size_t N>
    PdfName(const char(&str)[N])
        : PdfDataMember(PdfDataType::Name)
    {
        initFromUtf8String(str, N - 1);
    }

    template<typename T, typename = std::enable_if_t<std::is_same_v<T, const char*>>>
    PdfName(T str)
        : PdfDataMember(PdfDataType::Name)
    {
        initFromUtf8String(str, std::char_traits<char>::length(str));
    }

    /** Create a new PdfName object.
     *  \param str the unescaped value of this name. Please specify
     *      the name without the leading '/'.
     *      Has to be a zero terminated string.
     * \remarks the input string will be checked for all characters
     *      to be inside PdfDocEncoding character set
     */
    PdfName(const std::string_view& str);
    PdfName(const std::string& str);
    PdfName(charbuff&& buff);

    /**
     * This constructor is reserved for read-only string
     * literals, use with caution
     */
    PdfName(const char& str, size_t length);

    /** Create a copy of an existing PdfName object.
     *  \param rhs another PdfName object
     */
    PdfName(const PdfName& rhs);
    PdfName(PdfName&& rhs) noexcept;

    static PdfName FromRaw(const bufferview& rawcontent);

    /** Create a new PdfName object from a string containing an escaped
     *  name string without the leading / .
     *
     *  \param name A string containing the escaped name
     *  \return A new PdfName
     */
    static PdfName FromEscaped(const std::string_view& name);

    /** \return an escaped representation of this name
     *          without the leading / .
     *
     *  There is no corresponding GetEscapedLength(), since
     *  generating the return value is somewhat expensive.
     */
    std::string GetEscapedName() const;

    void Write(OutputStream& stream, PdfWriteFlags writeMode,
        const PdfStatefulEncrypt* encrypt, charbuff& buffer) const;

    /** \returns the unescaped value of this name object
     *           without the leading slash
     */
    std::string_view GetString() const;

    /** \returns true if the name is empty
     */
    bool IsNull() const;

    /** \returns the raw data of this name object
     */
    std::string_view GetRawData() const;

    /** Assign another name to this object
     *  \param rhs another PdfName object
     */
    PdfName& operator=(const PdfName& rhs);
    PdfName& operator=(PdfName&& rhs) noexcept;

    /** compare to PdfName objects.
     *  \returns true if both PdfNames have the same value.
     */
    bool operator==(const PdfName& rhs) const;
    bool operator==(const char* str) const;
    bool operator==(const std::string& str) const;
    bool operator==(const std::string_view& view) const;

    /** compare two PdfName objects.
     *  \returns true if both PdfNames have different values.
     */
    bool operator!=(const PdfName& rhs) const;
    bool operator!=(const char* str) const;
    bool operator!=(const std::string& str) const;
    bool operator!=(const std::string_view& view) const;

    /** Default cast to raw data string view
     *
     * It's used in PdfDictionary lookup 
     */
    operator std::string_view() const;

private:
    // Delete constructor with nullptr
    PdfName(std::nullptr_t) = delete;

    void expandUtf8String();
    void initFromUtf8String(const char* str, size_t length);
    void initFromUtf8String(const std::string_view& view);
    void moveFrom(PdfName&& rhs);

private:
    struct NameData
    {
        // The unescaped name raw data, without leading '/'.
        // It can store also the utf8 expanded string, if coincident
        charbuff Chars;
        std::unique_ptr<std::string> Utf8String;
        bool IsUtf8Expanded;
    };
private:
    bool m_dataAllocated;
    union
    {
        std::shared_ptr<NameData> m_data;
        std::string_view m_Utf8View;       // Holds only global read-only string literal
    };
};

/** Create a PdfName from a string literal without checking for PdfDocEncoding characters
 * \remarks Use with caution: only string literals should be used, not
 *      fixed size char arrays. Only ASCII charset is supported
 */
inline PdfName operator""_n(const char* name, size_t length)
{
    return PdfName(*name, length);
}

// Comparator to enable heterogeneous lookup in
// PdfDictionary with both PdfName and string_view
// See https://stackoverflow.com/a/31924435/213871
struct PODOFO_API PdfNameInequality
{
    using is_transparent = std::true_type;

    bool operator()(const PdfName& lhs, const PdfName& rhs) const
    {
        return lhs.GetRawData() < rhs.GetRawData();
    }
    bool operator()(const PdfName& lhs, const std::string_view& rhs) const
    {
        return lhs.GetRawData() < rhs;
    }
    bool operator()(const std::string_view& lhs, const PdfName& rhs) const
    {
        return lhs < rhs.GetRawData();
    }
};

struct PODOFO_API PdfNameHashing
{
    using is_transparent = std::true_type;

    inline std::size_t operator()(const std::string_view& name) const
    {
        return std::hash<std::string_view>()(name);
    }
    inline std::size_t operator()(const PdfName& name) const
    {
        return std::hash<std::string_view>()(name);
    }
};

struct PODOFO_API PdfNameEquality
{
    using is_transparent = std::true_type;

    inline bool operator()(const PdfName& lhs, const PdfName& rhs) const
    {
        return lhs.GetRawData() == rhs.GetRawData();
    }
    inline bool operator()(const PdfName& lhs, const std::string_view& rhs) const
    {
        return lhs.GetRawData() == rhs;
    }
    inline bool operator()(const std::string_view& lhs, const PdfName& rhs) const
    {
        return lhs == rhs.GetRawData();
    }
};

template<typename TValue>
using PdfNameMap = std::map<PdfName, TValue, PdfNameInequality>;

template<typename TValue>
using PdfNameHashMap = std::unordered_map<PdfName, TValue, PdfNameHashing, PdfNameEquality>;

};

#endif // PDF_NAME_H
