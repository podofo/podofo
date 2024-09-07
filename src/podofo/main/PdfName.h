/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_NAME_H
#define PDF_NAME_H

#include "PdfDeclarations.h"

#include "PdfDataProvider.h"

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
class PODOFO_API PdfName final : public PdfDataProvider<PdfName>
{
    friend PdfName PODOFO_API operator""_n(const char*, size_t);

public:
    /** Null name, corresponds to "/"
     */
    static const PdfName Null;

public:
    /** Constructor to create null name (corresponds to "/")
     *  use PdfName::Null instead of this constructor
     */
    PdfName();

    template<std::size_t N>
    PdfName(const char(&str)[N])
    {
        initFromUtf8String(str, N - 1);
    }

    template<typename T, typename = std::enable_if_t<std::is_same_v<T, const char*>>>
    PdfName(T str)
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

    /** Create a copy of an existing PdfName object.
     *  \param rhs another PdfName object
     */
    PdfName(const PdfName& rhs) = default;

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
    PdfName& operator=(const PdfName& rhs) = default;

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

    /** compare two PdfName objects.
     *  Used for sorting in lists
     *  \returns true if this object is smaller than rhs
     */
    bool operator<(const PdfName& rhs) const;

    /** Default cast to raw data string view
     *
     * It's used in PdfDictionary lookup 
     */
    operator std::string_view() const;

private:
    // Constructor for read-only string literals
    PdfName(const char* str, size_t length);

    // Delete constructor with nullptr
    PdfName(std::nullptr_t) = delete;

    void expandUtf8String();
    void initFromUtf8String(const char* str, size_t length);
    void initFromUtf8String(const std::string_view& view);

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
    std::shared_ptr<NameData> m_data;
    std::string_view m_dataView;
};

/** Create a PdfName from a string literal without checking for PdfDocEncoding characters
 * \remarks Use with caution: only string literals should be used, not
 *      fixed size char arrays. Only ASCII charset is supported
 */
inline PdfName operator""_n(const char* name, size_t length)
{
    return PdfName(name, length);
}

};

namespace std
{
    /** Overload hasher for PdfName
     */
    template<>
    struct hash<PoDoFo::PdfName>
    {
        size_t operator()(const PoDoFo::PdfName& name) const noexcept
        {
            return hash<string_view>()(name);
        }
    };
}

#endif // PDF_NAME_H
