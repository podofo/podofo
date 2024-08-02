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
class PODOFO_API PdfName final : public PdfDataProvider
{
public:
    /** Constructor to create nullptr strings.
     *  use PdfNames::Null instead of this constructor
     */
    PdfName();

    /** Create a new PdfName object.
     *  \param str the unescaped value of this name. Please specify
     *                 the name without the leading '/'.
     *                 Has to be a zero terminated string.
     */
    PdfName(const std::string_view& str);
    PdfName(const char* str);
    PdfName(const std::string& str);
    PdfName(charbuff&& buff);

    // Delete constructor with nullptr
    PdfName(std::nullptr_t) = delete;

    /** Create a copy of an existing PdfName object.
     *  \param rhs another PdfName object
     */
    PdfName(const PdfName& rhs);

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
        const PdfStatefulEncrypt* encrypt, charbuff& buffer) const override;

    /** \returns the unescaped value of this name object
     *           without the leading slash
     */
    const std::string& GetString() const;

    /** \returns true if the name is empty
     */
    bool IsNull() const;

    /** \returns the raw data of this name object
     */
    const std::string& GetRawData() const;

    /** Assign another name to this object
     *  \param rhs another PdfName object
     */
    const PdfName& operator=(const PdfName& rhs);

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
    void expandUtf8String() const;
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
};

/**
 * A storage class for several known PdfName entries
 */
class PODOFO_API PdfNames final
{
private:
    PdfNames() = delete;

public:
    static const PdfName Contents;
    static const PdfName Flags;
    static const PdfName Length;
    static const PdfName Null;
    static const PdfName Rect;
    static const PdfName Size;
    static const PdfName Subtype;
    static const PdfName Type;
    static const PdfName Filter;
    static const PdfName Parent;
    static const PdfName Kids;
    static const PdfName Count;
    static const PdfName ExtGState;
    static const PdfName ColorSpace;
    static const PdfName Pattern;
    static const PdfName Shading;
    static const PdfName XObject;
    static const PdfName Font;
    static const PdfName Properties;
    static const PdfName AP;
    static const PdfName Names;
    static const PdfName Limits;
};

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
            return hash<string_view>()(name.GetRawData());
        }
    };
}

#endif // PDF_NAME_H
