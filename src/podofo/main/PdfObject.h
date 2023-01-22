/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_OBJECT_H
#define PDF_OBJECT_H

#include "PdfVariant.h"
#include "PdfObjectStream.h"

namespace PoDoFo {

class PdfEncrypt;
class PdfIndirectObjectList;
class PdfDictionary;
class PdfArray;
class PdfDocument;
class PdfDataContainer;

/**
 * This class represents a PDF indirect Object in memory
 *
 * It is possible to manipulate the stream which can be appended to the object
 * (if the object is of underlying type dictionary).  A PdfObject is uniquely
 * identified by an object number and a generation number which has to be
 * passed to the constructor.
 *
 * The object can be written to a file easily using the Write() function.
 *
 * \see Write()
 */
class PODOFO_API PdfObject
{
    friend class PdfIndirectObjectList;
    friend class PdfArray;
    friend class PdfDictionary;
    friend class PdfDocument;
    friend class PdfObjectStream;
    friend class PdfDataContainer;
    friend class PdfObjectStreamParser;
    friend class PdfParser;

public:
    static PdfObject Null;

public:

    /** Create a PDF object with an empty PdfDictionary.
     */
    PdfObject();

    virtual ~PdfObject();

    /** Create a PDF object with the passed variant.
     *
     *  \param var the value of the object
     */
    PdfObject(const PdfVariant& var);
    PdfObject(PdfVariant&& var) noexcept;

    /** Construct a PdfObject with bool as value.
     *
     *  \param b the boolean value of this PdfObject
     */
    PdfObject(bool b);

    /** Construct a PdfObject with object with int64_t as value.
     *
     *  \param l the int64_t value of this PdfObject
     */
    PdfObject(int64_t l);

    /** Construct a PdfObject with double as value.
     *
     *  \param d the double value of this PdfObject
     */
    PdfObject(double d);

    /** Construct a PdfObject with PdfString as value.
     *
     *  \param str the string value of this PdfObject
     */
    PdfObject(const PdfString& str);

    /** Construct a PdfObject with PdfName as value.
     *
     *  \param name the value of this PdfObject
     */
    PdfObject(const PdfName& name);

    /** Construct a PdfObject with PdfReference as value.
     *
     *  \param ref the value of the this PdfObject
     */
    PdfObject(const PdfReference& ref);

    /** Construct a PdfObject with PdfArray as value.
     *
     *  \param arr the value of the this PdfObject
     */
    PdfObject(const PdfArray& arr);
    PdfObject(PdfArray&& arr) noexcept;

    /** Construct a PdfObject with PdfDictionary as value.
     *
     *  \param dict the value of the this PdfObject
     */
    PdfObject(const PdfDictionary& dict);
    PdfObject(PdfDictionary&& dict) noexcept;

    /** Creates a copy of an existing PdfObject.
     *  All associated objects and streams will be copied along with the PdfObject.
     *  \param rhs PdfObject to clone
     */
    PdfObject(const PdfObject& rhs);
    PdfObject(PdfObject&& rhs) noexcept;

public:
    /** \returns the datatype of this object or PdfDataType::Unknown
     *  if it does not have a value.
     */
    PdfDataType GetDataType() const;

    /** \returns a human readable string representation of GetDataType()
     *  The returned string must not be free'd.
     */
    const char* GetDataTypeString() const;

    /** \returns true if this variant is a bool
     */
    bool IsBool() const;

    /** \returns true if this variant is a number
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
    void SetBool(bool b);

    /** Set the value of this object as int64_t
     *  \param l the value as int64_t.
     *
     *  This will set the dirty flag of this object.
     *  \see IsDirty
     */
    void SetNumber(int64_t l);

    /** Set the value of this object as double
     *  \param d the value as double.
     *
     *  This will set the dirty flag of this object.
     *  \see IsDirty
     */
    void SetReal(double d);

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

    void ForceCreateStream();

    /** Write the complete object to a file.
     *  \param device write the object to this device
     *  \param encrypt an encryption object which is used to encrypt this object
     *                  or nullptr to not encrypt this object
     *  \param writeMode additional options for writing the object
     *  \param keyStop if not KeyNull and a key == keyStop is found
     *                 writing will stop right before this key!
     */
    void Write(OutputStreamDevice& device, PdfWriteFlags writeMode,
        const PdfEncrypt* encrypt, charbuff& buffer) const;

    /** Get a handle to a PDF stream object.
     *  If the PDF object does not have a stream,
     *  one will be created.
     *  \returns a PdfObjectStream object
     */
    PdfObjectStream& GetOrCreateStream();

    /** Get a handle to a const PDF stream object.
     * Throws if there's no stream
     */
    const PdfObjectStream& MustGetStream() const;

    /** Get a handle to a const PDF stream object.
     * Throws if there's no stream
     */
    PdfObjectStream& MustGetStream();

    /** Check if this object has a PdfObjectStream object
     *  appended.
     *
     *  \returns true if the object has a stream
     */
    bool HasStream() const;

    bool IsIndirect() const;

    const PdfVariant& GetVariant() const;

public:
    /** This operator is required for sorting a list of
     *  PdfObject instances. It compares the object number. If object numbers
     *  are equal, the generation number is compared.
     */
    bool operator<(const PdfObject& rhs) const;

    /** The equality operator with PdfObject checks for parent document and
     * indirect reference first
     */
    bool operator==(const PdfObject& rhs) const;

    /** The disequality operator with PdfObject checks for parent document and
     * indirect reference first
     */
    bool operator!=(const PdfObject& rhs) const;

    /** The equality operator with PdfVariant checks equality with variant object only
     */
    bool operator==(const PdfVariant& rhs) const;

    /** The disequality operator with PdfVariant checks disequality with variant object only
     */
    bool operator!=(const PdfVariant& rhs) const;

    /** Copy an existing PdfObject.
     *  All associated objects and streams will be copied along with the PdfObject.
     *  \param rhs PdfObject to copy from
     *  \returns a reference to this object
     */
    PdfObject& operator=(const PdfObject& rhs);
    PdfObject& operator=(PdfObject&& rhs) noexcept;

    operator const PdfVariant& () const;

public:
    /** The dirty flag is set if this variant
     *  has been modified after construction.
     *
     *  Usually the dirty flag is also set
     *  if you call any non-const member function
     *  (e.g. GetDictionary()) as PdfVariant cannot
     *  determine if you actually changed the dictionary
     *  or not.
     *
     *  \returns true if the value is dirty and has been
     *                modified since construction
     */
    inline bool IsDirty() const { return m_IsDirty; }

    /** Get the document of this object.
     *  \return the owner (if it wasn't changed anywhere, creator) of this object
     */
    inline PdfDocument* GetDocument() const { return m_Document; }

    /** Get the document of this object.
     *  \return the owner (if it wasn't changed anywhere, creator) of this object
     */
    PdfDocument& MustGetDocument() const;

    /** Get an indirect reference to this object.
     *  \returns a PdfReference pointing to this object.
     */
    inline const PdfReference& GetIndirectReference() const { return m_IndirectReference; }

    inline const PdfDataContainer* GetParent() const { return m_Parent; }

    /**
     * Returns true if delayed loading is disabled, or if it is enabled
     * and loading has completed. External callers should never need to
     * see this, it's an internal state flag only.
     */
    inline bool IsDelayedLoadDone() const { return m_IsDelayedLoadDone; }

    const PdfObjectStream* GetStream() const;
    PdfObjectStream* GetStream();

protected:
    PdfObject(PdfVariant&& var, const PdfReference& indirectReference, bool isDirty);

    /**
     * Dynamically load the contents of this object from a PDF file by calling
     * the virtual method DelayedLoadImpl() if the object is not already loaded.
     *
     * For objects complete created in memory and those that do not support
     * deferred loading this function does nothing, since deferred loading
     * will not be enabled.
     */
    void DelayedLoad() const;

    /** Load all data of the object if delayed loading is enabled.
     *
     * Never call this method directly; use DelayedLoad() instead.
     *
     * You should override this to control deferred loading in your subclass.
     * Note that this method should not load any associated streams, just the
     * base object.
     *
     * The default implementation throws. It should never be called, since
     * objects that do not support delayed loading should not enable it.
     *
     * While this method is not `const' it may be called from a const context,
     * so be careful what you mess with.
     */
    virtual void DelayedLoadImpl();

    virtual void DelayedLoadStreamImpl();

    /** Sets the dirty flag of this PdfVariant
     *
     *  \see IsDirty
     */
    void SetDirty();

    void resetDirty();

    /** Set the owner of this object, i.e. the PdfIndirectObjectList to which
     *  this object belongs.
     *
     *  \param objects a vector of pdf objects
     */
    void SetDocument(PdfDocument* document);

    void SetVariantOwner();

    void FreeStream();

    PdfObjectStream& getOrCreateStream();

    void forceCreateStream();

    PdfObjectStream* getStream();

    void DelayedLoadStream() const;

    void delayedLoadStream() const;

    void EnableDelayedLoadingStream();

    inline void SetIndirectReference(const PdfReference& reference) { m_IndirectReference = reference; }

    /** Flag the object  incompletely loaded.  DelayedLoad() will be called
     *  when any method that requires more information than is currently
     *  available is loaded.
     *
     *  All constructors initialize a PdfVariant with delayed loading disabled .
     *  If you want delayed loading you must ask for it. If you do so, call
     *  this method early in your ctor and be sure to override DelayedLoadImpl().
     */
    void EnableDelayedLoading();

private:
    // Assign function that doesn't set dirty
    void Assign(const PdfObject& rhs);

    void SetParent(PdfDataContainer& parent);

private:
    void assign(const PdfObject& rhs);

    void moveFrom(PdfObject& rhs);

    void ResetDirty();

    void setDirty();

    // See PdfVariant.h for a detailed explanation of this member, which is
    // here to prevent accidental construction of a PdfObject of integer type
    // when passing a pointer. */
    template<typename T>
    PdfObject(T*) = delete;

    void copyStreamFrom(const PdfObject& obj);

    void moveStreamFrom(PdfObject& obj);

    // Shared initialization between all the ctors
    void initObject();

protected:
    PdfVariant m_Variant;

private:
    PdfReference m_IndirectReference;
    PdfDocument* m_Document;
    PdfDataContainer* m_Parent;
    bool m_IsDirty; // Indicates if this object was modified after construction

    mutable bool m_IsDelayedLoadDone;
    mutable bool m_IsDelayedLoadStreamDone;
    std::unique_ptr<PdfObjectStream> m_Stream;
    // Tracks whether deferred loading is still pending (in which case it'll be
    // false). If true, deferred loading is not required or has been completed.
};

    /** Templatized object type getter helper
     */
    template <typename T>
    struct Object
    {
        static T Get(const PdfObject& obj)
        {
            (void)obj;
            static_assert(always_false<T>, "Unsupported type");
            return T{ };
        }

        static bool TryGet(const PdfObject& obj, T& value)
        {
            (void)obj;
            (void)value;
            static_assert(always_false<T>, "Unsupported type");
            return false;
        }
    };

    template <>
    struct Object<bool>
    {
        static bool Get(const PdfObject& obj)
        {
            return obj.GetBool();
        }

        static bool TryGet(const PdfObject& obj, bool& value)
        {
            return obj.TryGetBool(value);
        }
    };

    template <>
    struct Object<int64_t>
    {
        static int64_t Get(const PdfObject& obj)
        {
            return obj.GetNumber();
        }

        static bool TryGet(const PdfObject& obj, int64_t& value)
        {
            return obj.TryGetNumber(value);
        }
    };

    template <>
    struct Object<double>
    {
        static double Get(const PdfObject& obj)
        {
            return obj.GetReal();
        }

        static bool TryGet(const PdfObject& obj, double& value)
        {
            return obj.TryGetReal(value);
        }
    };

    template <>
    struct Object<PdfReference>
    {
        static PdfReference Get(const PdfObject& obj)
        {
            return obj.GetReference();
        }

        static bool TryGet(const PdfObject& obj, PdfReference& value)
        {
            return obj.TryGetReference(value);
        }
    };

    template <>
    struct Object<PdfName>
    {
        static PdfName Get(const PdfObject& obj)
        {
            return obj.GetName();
        }

        static bool TryGet(const PdfObject& obj, PdfName& value)
        {
            return obj.TryGetName(value);
        }
    };

    template <>
    struct Object<PdfString>
    {
        static PdfString Get(const PdfObject& obj)
        {
            return obj.GetString();
        }

        static bool TryGet(const PdfObject& obj, PdfString& value)
        {
            return obj.TryGetString(value);
        }
    };


};

#endif // PDF_OBJECT_H
