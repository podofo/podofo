/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_VARIANT_H
#define PDF_VARIANT_H

#include "PdfDeclarations.h"
#include "PdfReference.h"
#include "PdfName.h"
#include "PdfString.h"

namespace PoDoFo {

class PdfArray;
class PdfData;
class PdfDataProvider;
class PdfDictionary;
class PdfString;

/**
 * A variant data type which supports all data types supported by the PDF standard.
 * The data can be parsed directly from a string or set by one of the members.
 * One can also convert the variant back to a string after setting the values.
 *
 * \warning All methods not marked otherwise may trigger a deferred load. This means
 *          that they are unsafe to call while a deferred load is already in progress
 *          (as recursion will occur).
 *
 */
class PODOFO_API PdfVariant final
{
    friend class PdfArray;
    friend class PdfDictionary;

private:
    PdfVariant(PdfDataType type);

public:

    static PdfVariant Null;

    /** Construct an empty variant type
     *  IsNull() will return true.
     */
    PdfVariant();

    /** Construct a PdfVariant that is a bool.
     *  \param value the boolean value of this PdfVariant
     */
    PdfVariant(bool value);

    /** Construct a PdfVariant that is a number.
     *  \param value the value of the number.
     */
    PdfVariant(int64_t value);

    /** Construct a PdfVariant that is a real number.
     *  \param value the value of the real number.
     */
    PdfVariant(double value);

    /** Construct a PdfVariant that is a string. The argument
     * string will be escaped where necessary, so it should be
     * passed in unescaped form.
     *
     *  \param str the value of the string
     */
    PdfVariant(const PdfString& str);

    /** Construct a PdfVariant that is a name.
     *  \param name the value of the name
     */
    PdfVariant(const PdfName& name);

    /** Construct a PdfVariant that is a name.
     *  \param ref the value of the name
     */
    PdfVariant(const PdfReference& ref);

    /** Construct a PdfVariant object with array data.
     *  The variant will automatically get the datatype
     *  PdfDataType::Array. This constructor is the fastest
     *  way to create a new PdfVariant that is an array.
     *
     *  \param arr a list of variants
     */
    PdfVariant(const PdfArray& arr);
    PdfVariant(PdfArray&& arr) noexcept;

    /** Construct a PdfVariant that is a dictionary.
     *  \param dict the value of the dictionary.
     */
    PdfVariant(const PdfDictionary& dict);
    PdfVariant(PdfDictionary&& dict) noexcept;

    /** Construct a PdfVariant that contains raw PDF data.
     *  \param rData raw and valid PDF data.
     */
    PdfVariant(const PdfData& data);
    PdfVariant(PdfData&& data) noexcept;

    /** Constructs a new PdfVariant which has the same
     *  contents as rhs.
     *  \param rhs an existing variant which is copied.
     */
    PdfVariant(const PdfVariant& rhs);
    PdfVariant(PdfVariant&& rhs) noexcept;

    ~PdfVariant();

    /** \returns a human readable string representation of GetDataType()
     *  The returned string must not be free'd.
     */
    const char* GetDataTypeString() const;

    /** \returns true if this variant is a bool
     */
    bool IsBool() const;

    /** \returns true if this variant is an integer
     */
    bool IsNumber() const;

    /** \returns true if this variant is a real
     *
     *  This method strictly check for a floating point number and return false on integer
     */
    bool IsRealStrict() const;

    /** \returns true if this variant is an integer or a floating point number
     */
    bool IsNumberOrReal() const;

    /** \returns true if this variant is a string
     */
    bool IsString() const;

    /** \returns true if this variant is a name
     */
    bool IsName() const;

    /** \returns true if this variant is an array
     */
    bool IsArray() const;

    /** \returns true if this variant is a dictionary
     */
    bool IsDictionary() const;

    /** \returns true if this variant is raw data
     */
    bool IsRawData() const;

    /** \returns true if this variant is null
     */
    bool IsNull() const;

    /** \returns true if this variant is a reference
     */
    bool IsReference() const;

    /** Converts the current object into a string representation
     *  which can be written directly to a PDF file on disc.
     *  \param str the object string is returned in this object.
     */
    std::string ToString() const;
    void ToString(std::string& str) const;

    /** Get the value if this object is a bool.
     *  \returns the bool value.
     */
    bool GetBool() const;
    bool TryGetBool(bool& value) const;

    /** Get the value of the object as int64_t.
     *
     *  This method is lenient and narrows floating point numbers
     *  \return the value of the number
     */
    int64_t GetNumberLenient() const;
    bool TryGetNumberLenient(int64_t& value) const;

    /** Get the value of the object as int64_t
     *
     *  This method throws if the numer is a floating point number
     *  \return the value of the number
     */
    int64_t GetNumber() const;
    bool TryGetNumber(int64_t& value) const;

    /** Get the value of the object as a floating point
     *
     *  This method is lenient and returns also strictly integral numbers
     *  \return the value of the number
     */
    double GetReal() const;
    bool TryGetReal(double& value) const;

    /** Get the value of the object as floating point number
     *
     *  This method throws if the numer is integer
     *  \return the value of the number
     */
    double GetRealStrict() const;
    bool TryGetRealStrict(double& value) const;

    /** \returns the value of the object as string.
     */
    const PdfString& GetString() const;
    bool TryGetString(PdfString& str) const;
    bool TryGetString(const PdfString*& str) const;

    /** \returns the value of the object as name
     */
    const PdfName& GetName() const;
    bool TryGetName(PdfName& name) const;
    bool TryGetName(const PdfName*& name) const;

    /** Get the reference values of this object.
     *  \returns a PdfReference
     */
    PdfReference GetReference() const;
    bool TryGetReference(PdfReference& ref) const;

    /** Returns the value of the object as array
     *  \returns a array
     */
    const PdfArray& GetArray() const;
    PdfArray& GetArray();
    bool TryGetArray(const PdfArray*& arr) const;
    bool TryGetArray(PdfArray*& arr);

    /** Returns the dictionary value of this object
     *  \returns a PdfDictionary
     */
    const PdfDictionary& GetDictionary() const;
    PdfDictionary& GetDictionary();
    bool TryGetDictionary(const PdfDictionary*& dict) const;
    bool TryGetDictionary(PdfDictionary*& dict);

    /** Set the value of this object as bool
     *  \param b the value as bool.
     *
     *  This will set the dirty flag of this object.
     *  \see IsDirty
     */
    void SetBool(bool value);

    /** Set the value of this object as int64_t
     *  \param l the value as int64_t.
     *
     *  This will set the dirty flag of this object.
     *  \see IsDirty
     */
    void SetNumber(int64_t value);

    /** Set the value of this object as double
     *  \param d the value as double.
     *
     *  This will set the dirty flag of this object.
     *  \see IsDirty
     */
    void SetReal(double value);

    /** Set the name value of this object
    *  \param d the name value
    *
    *  This will set the dirty flag of this object.
    *  \see IsDirty
    */
    void SetName(const PdfName& name);

    /** Set the string value of this object.
     * \param str the string value
     *
     * This will set the dirty flag of this object.
     * \see IsDirty
     */
    void SetString(const PdfString& str);

    void SetReference(const PdfReference& ref);

    /** Write the complete variant to an output device.
     *  \param device write the object to this device
     *  \param writeMode additional options for writing this object
     *  \param encrypt an encryption object which is used to encrypt this object
     *                  or nullptr to not encrypt this object
     */
    void Write(OutputStreamDevice& device, PdfWriteFlags writeMode,
        const PdfStatefulEncrypt& encrypt, charbuff& buffer) const;

    /** Assign the values of another PdfVariant to this one.
     *  \param rhs an existing variant which is copied.
     *
     *  This will set the dirty flag of this object.
     *  \see IsDirty
     */
    PdfVariant& operator=(const PdfVariant& rhs);
    PdfVariant& operator=(PdfVariant&& rhs) noexcept;

    /**
     * Test to see if the value contained by this variant is the same
     * as the value of the other variant.
     */
    bool operator==(const PdfVariant& rhs) const;

    /**
     * \see operator==
     */
    bool operator!=(const PdfVariant& rhs) const;

public:
    inline PdfDataType GetDataType() const { return m_DataType; }

private:
    void clear();
    void assign(const PdfVariant& rhs);
    bool tryGetDictionary(PdfDictionary*& dict) const;
    bool tryGetArray(PdfArray*& arr) const;
    bool tryGetName(const PdfName*& name) const;
    bool tryGetString(const PdfString*& str) const;

private:
    /**
     * It's an easy mistake to pass a pointer to a PdfVariant when trying to
     * copy a PdfVariant, especially with heap allocators like `new'. This can
     * produce confusing and unexpected results like getting a PdfVariant(bool).
     *
     * A similar issue can arise when the user passes a `char*' and expects a PdfName
     * or PdfString based variant to be created. We can't know which they wanted, so
     * we should fail, especially since the compiler tends to convert pointers to bool
     * for extra confusion value.
     *
     * We provide this overload so that such attempts will fail with an error about
     * a private ctor. If you're reading this, you wrote:
     *
     *  PdfVariant( my_ptr_to_something )
     *
     *... not ...
     *
     *  PdfVariant( *my_ptr_to_something )
     *
     * If you need to modify PdfVariant to legitimately take a pointer in the future,
     * you can do so by providing a template specialization, or by removing this check
     * and replacing it with a couple of overloads specific to PdfObject*, PdfVariant*,
     * and char* (at least).
     */
    template<typename T> PdfVariant(T*);

    /** To reduce memory usage of this very often used class,
     *  we use a union here, as there is always only
     *  one of those members used.
     */
    union Variant
    {
        /** Holds references, strings,
         *  names, dictionaries and arrays
         */
        int64_t Number;
        double Real;
        PdfDataProvider* Data;
        PdfReference Reference;
        bool Bool;
    };

    Variant m_Data;
    PdfDataType m_DataType;
};

};

#endif // PDF_VARIANT_H
