/***************************************************************************
 *   Copyright (C) 2005 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 *                                                                         *
 *   In addition, as a special exception, the copyright holders give       *
 *   permission to link the code of portions of this program with the      *
 *   OpenSSL library under certain conditions as described in each         *
 *   individual source file, and distribute linked combinations            *
 *   including the two.                                                    *
 *   You must obey the GNU General Public License in all respects          *
 *   for all of the code used other than OpenSSL.  If you modify           *
 *   file(s) with this exception, you may extend this exception to your    *
 *   version of the file(s), but you are not obligated to do so.  If you   *
 *   do not wish to do so, delete this exception statement from your       *
 *   version.  If you delete this exception statement from all source      *
 *   files in the program, then also delete it here.                       *
 ***************************************************************************/

#ifndef _PDF_VARIANT_H_
#define _PDF_VARIANT_H_

#if defined(__BORLANDC__) || defined( __TURBOC__)
#include <math.h>
#else
#include <cmath>
#endif

#include "PdfDefines.h"
#include "PdfRefCountedBuffer.h"
#include "PdfString.h"

namespace PoDoFo {

class PdfArray;
class PdfData;
class PdfDataType;
class PdfDictionary;
class PdfEncrypt;
class PdfName;
class PdfOutputDevice;
class PdfString;
class PdfReference;

/**
 * A variant data type which supports all data types supported by the PDF standard.
 * The data can be parsed directly from a string or set by one of the members.
 * One can also convert the variant back to a string after setting the values.
 *
 * \warning All methods not marked otherwise may trigger a deferred load. This means
 *          that they are unsafe to call while a deferred load is already in progress
 *          (as recursion will occurr).
 *
 * TODO: domseichter: Make this class implicitly shared
 */
class PODOFO_API PdfVariant {
    friend class PdfArray;
    friend class PdfDictionary;

 public:

    static PdfVariant NullValue;

    /** Construct an empty variant type
     *  IsNull() will return true.
     */
    PdfVariant();

    /** Construct a PdfVariant that is a bool.
     *  \param b the boolean value of this PdfVariant
     */
    PdfVariant( bool b );

    /** Construct a PdfVariant that is a number.
     *  \param l the value of the number.
     */
    PdfVariant( pdf_int64 l );

    /** Construct a PdfVariant that is a real number.
     *  \param d the value of the real number.
     */    
    PdfVariant( double d );

    /** Construct a PdfVariant that is a string. The argument
     * string will be escaped where necessary, so it should be
     * passed in unescaped form.
     *
     *  \param rsString the value of the string
     */        
    PdfVariant( const PdfString & rsString );

    /** Construct a PdfVariant that is a name.
     *  \param rName the value of the name
     */        
    PdfVariant( const PdfName & rName );

    /** Construct a PdfVariant that is a name.
     *  \param rRef the value of the name
     */        
    PdfVariant( const PdfReference & rRef );

    /** Construct a PdfVariant object with array data.
     *  The variant will automatically get the datatype
     *  ePdfDataType_Array. This constructor is the fastest
     *  way to create a new PdfVariant that is an array.
     *
     *  \param tList a list of variants
     */
    PdfVariant( const PdfArray & tList );

    /** Construct a PdfVariant that is a dictionary.
     *  \param rDict the value of the dictionary.
     */        
    PdfVariant( const PdfDictionary & rDict );

    /** Construct a PdfVariant that contains raw PDF data.
     *  \param rData raw and valid PDF data.
     */        
    PdfVariant( const PdfData & rData );

    /** Constructs a new PdfVariant which has the same 
     *  contents as rhs.
     *  \param rhs an existing variant which is copied.
     */
    PdfVariant( const PdfVariant & rhs );

    virtual ~PdfVariant();
    
    /** \returns true if this PdfVariant is empty.
     *           i.e. m_eDataType == ePdfDataType_Null
     */
    inline bool IsEmpty() const;

    /** Clear all internal member variables and free the memory
     *  they have allocated.
     *  Sets the datatype to ePdfDataType_Null
     *
     *  This will reset the dirty flag of this object to be clean.
     *  \see IsDirty
     */
    void Clear();

    /** \returns the datatype of this object or ePdfDataType_Unknown
     *  if it does not have a value.
     */
    inline EPdfDataType GetDataType() const;

    /** \returns a human readable string representation of GetDataType()
     *  The returned string must not be free'd.
     */
    const char * GetDataTypeString() const;

    /** \returns true if this variant is a bool (i.e. GetDataType() == ePdfDataType_Bool)
     */
    inline bool IsBool() const { return GetDataType() == ePdfDataType_Bool; }

    /** \returns true if this variant is a number (i.e. GetDataType() == ePdfDataType_Number)
     */
    inline bool IsNumber() const { return GetDataType() == ePdfDataType_Number; }

    /** \returns true if this variant is a real (i.e. GetDataType() == ePdfDataType_Real)
     */
    inline bool IsReal() const { return GetDataType() == ePdfDataType_Real; }

    /** \returns true if this variant is a string (i.e. GetDataType() == ePdfDataType_String)
     */
    inline bool IsString() const { return GetDataType() == ePdfDataType_String; }

    /** \returns true if this variant is a hex-string (i.e. GetDataType() == ePdfDataType_HexString)
     */
    inline bool IsHexString() const { return GetDataType() == ePdfDataType_HexString; }

    /** \returns true if this variant is a name (i.e. GetDataType() == ePdfDataType_Name)
     */
    inline bool IsName() const { return GetDataType() == ePdfDataType_Name; }

    /** \returns true if this variant is an array (i.e. GetDataType() == ePdfDataType_Array)
     */
    inline bool IsArray() const { return GetDataType() == ePdfDataType_Array; }

    /** \returns true if this variant is a dictionary (i.e. GetDataType() == ePdfDataType_Dictionary)
     */
    inline bool IsDictionary() const { return GetDataType() == ePdfDataType_Dictionary; }

    /** \returns true if this variant is raw data (i.e. GetDataType() == ePdfDataType_RawData
     */
    inline bool IsRawData() const { return GetDataType() == ePdfDataType_RawData; }

    /** \returns true if this variant is null (i.e. GetDataType() == ePdfDataType_Null)
     */
    inline bool IsNull() const { return GetDataType() == ePdfDataType_Null; }

    /** \returns true if this variant is a reference (i.e. GetDataType() == ePdfDataType_Reference)
     */
    inline bool IsReference() const { return GetDataType() == ePdfDataType_Reference; }
       
    /** Write the complete variant to an output device.
     *  This is an overloaded member function.
     *
     *  \param pDevice write the object to this device
     *  \param eWriteMode additional options for writing this object
     *  \param pEncrypt an encryption object which is used to encrypt this object
     *                  or NULL to not encrypt this object
     */
    void Write( PdfOutputDevice* pDevice, EPdfWriteMode eWriteMode, 
                const PdfEncrypt* pEncrypt = NULL ) const;

    /** Write the complete variant to an output device.
     *  \param pDevice write the object to this device
     *  \param eWriteMode additional options for writing this object
     *  \param pEncrypt an encryption object which is used to encrypt this object
     *                  or NULL to not encrypt this object
     *  \param keyStop if not KeyNull and a key == keyStop is found
     *                 writing will stop right before this key!
     *                 if IsDictionary returns true.
     */
    virtual void Write( PdfOutputDevice* pDevice, EPdfWriteMode eWriteMode, const PdfEncrypt* pEncrypt, const PdfName & keyStop ) const;

    /** Converts the current object into a string representation
     *  which can be written directly to a PDF file on disc.
     *  \param rsData the object string is returned in this object.
     *  \param eWriteMode additional options for writing to a string
     */
    void ToString( std::string & rsData, EPdfWriteMode eWriteMode = ePdfWriteMode_Clean ) const;

    /** Set the value of this object as bool
     *  \param b the value as bool.
     *
     *  This will set the dirty flag of this object.
     *  \see IsDirty
     */
    inline void SetBool( bool b );

    /** Get the value if this object is a bool.
     *  \returns the bool value.
     */
    inline bool GetBool() const;

    /** Set the value of this object as long
     *  \param l the value as long.
     *
     *  This will set the dirty flag of this object.
     *  \see IsDirty
     */
    inline void SetNumber( long l );

    /** Get the value of the object as long.
     *  \return the value of the number
     */
    inline pdf_int64 GetNumber() const;

    /** Set the value of this object as double
     *  \param d the value as double.
     *
     *  This will set the dirty flag of this object.
     *  \see IsDirty
     */
    inline void SetReal( double d );

    /** Get the value of the object as double.
     *  \return the value of the number
     */
    inline double GetReal() const;

    /** Set the string value of this object.
     * \param str the string value
     *
     * This will set the dirty flag of this object.
     * \see IsDirty
     */
    inline void SetString(const PdfString & str);

    /** \returns the value of the object as string.
     */
    inline const PdfString & GetString() const;

    /** \returns the value of the object as name
     */
    inline const PdfName & GetName() const;

    /** Returns the value of the object as array
     *  \returns a array
     */
    inline const PdfArray & GetArray() const;

    /** Returns the value of the object as array
     *  \returns a array
     */
    inline PdfArray & GetArray();

    /** Returns the dictionary value of this object
     *  \returns a PdfDictionary
     */
    inline const PdfDictionary & GetDictionary() const; 

    /** Returns the dictionary value of this object
     *  \returns a PdfDictionary
     */
    inline PdfDictionary & GetDictionary(); 

    /** Get the reference values of this object.
     *  \returns a PdfReference
     */
    inline const PdfReference & GetReference() const;

    /** Get the reference values of this object.
     *  \returns a reference to the PdfData instance.
     */
    inline const PdfData & GetRawData() const;

    /** Get the reference values of this object.
     *  \returns a reference to the PdfData instance.
     */
    inline PdfData & GetRawData();

    /** Assign the values of another PdfVariant to this one.
     *  \param rhs an existing variant which is copied.
     *
     *  This will set the dirty flag of this object.
     *  \see IsDirty
     */
    const PdfVariant & operator=( const PdfVariant & rhs );

    /**
     * Test to see if the value contained by this variant is the same
     * as the value of the other variant.
     */
    bool operator==( const PdfVariant & rhs ) const;

    /**
     * \see operator==
     */
    inline bool operator!=( const PdfVariant & rhs) const;

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
    inline bool IsDirty() const;

    /**
     * Sets this object to immutable,
     * so that no keys can be edited or changed.
     *
     * @param bImmutable if true set the object to be immutable
     *
     * This is used by PdfImmediateWriter and PdfStreamedDocument so 
     * that no keys can be added to an object after setting stream data on it.
     *
     */
    inline void SetImmutable(bool bImmutable);

    /**
     * Retrieve if an object is immutable.
     *
     * This is used by PdfImmediateWriter and PdfStreamedDocument so 
     * that no keys can be added to an object after setting stream data on it.
     *
     * \returns true if the object is immutable
     */
    inline bool GetImmutable() const;

 protected:

    /**
     *  Will throw an exception if called on an immutable object,
     *  so this should be called before actually changing a value!
     * 
     */
    inline void AssertMutable() const;

    /** Sets the dirty flag of this PdfVariant
     *
     *  \param bDirty true if this PdfVariant has been
     *                modified from the outside
     *
     *  \see IsDirty
     */
    inline void SetDirty( bool bDirty );

    /**
     * Dynamically load the contents of this object from a PDF file by calling
     * the virtual method DelayedLoadImpl() if the object is not already loaded.
     *
     * For objects complete created in memory and those that do not support
     * deferred loading this function does nothing, since deferred loading
     * will not be enabled.
     */
    inline void DelayedLoad() const;

    /** Flag the object  incompletely loaded.  DelayedLoad() will be called
     *  when any method that requires more information than is currently
     *  available is loaded.
     *
     *  All constructors initialize a PdfVariant with delayed loading disabled .
     *  If you want delayed loading you must ask for it. If you do so, call
     *  this method early in your ctor and be sure to override DelayedLoadImpl().
     */
    inline void EnableDelayedLoading();

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
    inline virtual void DelayedLoadImpl();

    /** Called after delayed load
     *  \param eDataType Detected data type
     */
    inline virtual void AfterDelayedLoad( EPdfDataType eDataType );

    /**
     * Returns true if delayed loading is disabled, or if it is enabled
     * and loading has completed. External callers should never need to
     * see this, it's an internal state flag only.
     */
    PODOFO_NOTHROW inline bool DelayedLoadDone() const;

    // Rather than having deferred load triggering disabled while deferred
    // loading is in progress, causing public methods to potentially return
    // invalid data, we provide special methods that won't trigger a deferred
    // load for use during deferred loading. They're not for general use and
    // not available for use except by subclasses.
    //
    /** Version of GetDictionary() that doesn't trigger a delayed load
     *  \returns a PdfDictionary
     */
    inline const PdfDictionary & GetDictionary_NoDL() const; 

    /** Version of GetDictionary() that doesn't trigger a delayed load
     *  \returns a PdfDictionary
     */
    inline PdfDictionary & GetDictionary_NoDL(); 

    /** Version of GetArray() that doesn't trigger a delayed load
     *  \returns a PdfArray
     */
    inline const PdfArray & GetArray_NoDL() const;

    /** Version of GetArray() that doesn't trigger a delayed load.
     *  \returns a PdfArray
     */
    inline PdfArray & GetArray_NoDL();

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
    typedef union { 
        /** Holds references, strings, 
         *  names, dictionaries and arrays
         */
        PdfDataType* pData;

        bool       bBoolValue;
        double     dNumber;
        pdf_int64      nNumber;
    } UVariant;

    UVariant     m_Data;

    bool         m_bDirty; ///< Indicates if this object was modified after construction
    bool         m_bImmutable; ///< Indicates if this object may be modified


    /** Datatype of the variant, required to access the correct member of the union UVariant.
     *  The data type is to save RAM space because this class is used in very many instances.
     *  No fixed-underlying-type enum is used, for pre-C++11 compatibility.
     *  The type is unsigned because there's no negative value in the enum PdfDataType and
     *  to cleanly enable the ePdfDataType_Unknown value to be 0xff (as sentinel value).
     */
    pdf_uint8 m_eDataType;

    // No touchy. Only for use by PdfVariant's internal tracking of the delayed
    // loading state. Use DelayedLoadDone() to test this if you need to.
    mutable bool m_bDelayedLoadDone;

    // Helper for ctor
    PODOFO_NOTHROW void Init();

#if defined(PODOFO_EXTRA_CHECKS)
protected:
    PODOFO_NOTHROW bool DelayedLoadInProgress() const { return m_bDelayedLoadInProgress; }
private:
    mutable bool m_bDelayedLoadInProgress;
#endif
};


// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfVariant::DelayedLoad() const
{
#if defined(PODOFO_EXTRA_CHECKS)
    // Whoops! Delayed loading triggered during delayed loading. Someone probably
    // used a public method that calls DelayedLoad() from a delayed load.
    if (m_bDelayedLoadInProgress)
        PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Recursive DelayedLoad() detected" );
#endif
    if( !m_bDelayedLoadDone)
    {
#if defined(PODOFO_EXTRA_CHECKS)
        m_bDelayedLoadInProgress = true;
#endif
        const_cast<PdfVariant*>(this)->DelayedLoadImpl();
        // Nothing was thrown, so if the implementer of DelayedLoadImpl()
        // following the rules we're done.
        m_bDelayedLoadDone = true;
#if defined(PODOFO_EXTRA_CHECKS)
        m_bDelayedLoadInProgress = false;
#endif
        const_cast<PdfVariant*>(this)->AfterDelayedLoad( ( EPdfDataType )m_eDataType );
    }
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfVariant::IsEmpty() const
{
    DelayedLoad();

    return (m_eDataType == ePdfDataType_Null);
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
EPdfDataType PdfVariant::GetDataType() const
{
    DelayedLoad();

    return static_cast<EPdfDataType>(m_eDataType);
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfVariant::SetBool( bool b )
{
    DelayedLoad();

    if( !IsBool() )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
    }

    AssertMutable();
    m_Data.bBoolValue = b;
    SetDirty( true );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfVariant::GetBool() const
{
    DelayedLoad();

    if( !IsBool() )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
    }

    return m_Data.bBoolValue;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfVariant::SetNumber( long l ) 
{
    DelayedLoad();

    if( !IsReal() && !IsNumber() )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
    }

    AssertMutable();
    if ( IsReal() )
        m_Data.dNumber = static_cast<double>(l);
    else
        m_Data.nNumber = l;
    SetDirty( true );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
pdf_int64 PdfVariant::GetNumber() const
{
    DelayedLoad();

    if( !IsReal() && !IsNumber() )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
    }

    if ( IsReal() )
        return static_cast<long>(floor( m_Data.dNumber ));
    else
        return m_Data.nNumber;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfVariant::SetReal( double d ) 
{
    DelayedLoad();

    if( !IsReal() && !IsNumber() )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
    }

    AssertMutable();
    if ( IsReal() )
        m_Data.dNumber = d;
    else
        m_Data.nNumber = static_cast<long>(floor( d ));
    SetDirty( true );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfVariant::GetReal() const
{
    DelayedLoad();

    if( !IsReal() && !IsNumber() )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
    }

    if ( IsReal() )
        return m_Data.dNumber;
    else
        return static_cast<double>(m_Data.nNumber);
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif // __GNUC__
const PdfData & PdfVariant::GetRawData() const
{
    DelayedLoad();

    if( !IsRawData() )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
    }
    // Do not change this to an reinterpret_cast
    // We need a c-style casts here to avoid crashes
    // because a reinterpret_cast might point to a different position.
    return *((PdfData*)m_Data.pData);
}
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif // __GNUC__


PdfData & PdfVariant::GetRawData()
{
    DelayedLoad();

    if( !IsRawData() )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
    }
    // Do not change this to an reinterpret_cast
    // We need a c-style casts here to avoid crashes
    // because a reinterpret_cast might point to a different position.
    return *((PdfData*)m_Data.pData);
}

// -----------------------------------------------------
//
// -----------------------------------------------------
void PdfVariant::SetString(const PdfString &str)
{
    DelayedLoad();

    if (!IsString())
    {
        PODOFO_RAISE_ERROR(ePdfError_InvalidDataType);
    }

    AssertMutable();
    *((PdfString*)m_Data.pData) = str;
    SetDirty(true);
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfString & PdfVariant::GetString() const
{
    DelayedLoad();

    if( !IsString() && !IsHexString() )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
    }
    // Do not change this to an reinterpret_cast
    // We need a c-style casts here to avoid crashes
    // because a reinterpret_cast might point to a different position.
    return *((PdfString*)m_Data.pData);
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfName & PdfVariant::GetName() const
{
    DelayedLoad();

    if( !IsName() )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
    }

    // Do not change this to an reinterpret_cast
    // We need a c-style casts here to avoid crashes
    // because a reinterpret_cast might point to a different position.
    return *((PdfName*)m_Data.pData);
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfArray & PdfVariant::GetArray() const
{
    DelayedLoad();
    return GetArray_NoDL();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfArray & PdfVariant::GetArray_NoDL() const
{
    // Test against eDataType directly not GetDataType() since
    // we don't want to trigger a delayed load (and if required one has
    // already been triggered).
    if( m_eDataType != ePdfDataType_Array )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
    }

    // Do not change this to an reinterpret_cast
    // We need a c-style casts here to avoid crashes
    // because a reinterpret_cast might point to a different position.
    return *((PdfArray*)m_Data.pData);
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfArray & PdfVariant::GetArray()
{
    DelayedLoad();
    return GetArray_NoDL();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfArray & PdfVariant::GetArray_NoDL()
{
    // Test against eDataType directly not GetDataType() since
    // we don't want to trigger a delayed load (and if required one has
    // already been triggered).
    if( m_eDataType != ePdfDataType_Array )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
    }

    // Do not change this to an reinterpret_cast
    // We need a c-style casts here to avoid crashes
    // because a reinterpret_cast might point to a different position.
    return *((PdfArray*)m_Data.pData);
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfDictionary & PdfVariant::GetDictionary() const
{
    DelayedLoad();
    return GetDictionary_NoDL();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfDictionary & PdfVariant::GetDictionary_NoDL() const
{
    // Test against eDataType directly not GetDataType() since
    // we don't want to trigger a delayed load (and if required one has
    // already been triggered).
    if( m_eDataType != ePdfDataType_Dictionary )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
    }

    // Do not change this to an reinterpret_cast
    // We need a c-style casts here to avoid crashes
    // because a reinterpret_cast might point to a different position.
    return *((PdfDictionary*)m_Data.pData);
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfDictionary & PdfVariant::GetDictionary()
{
    DelayedLoad();
    return GetDictionary_NoDL();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfDictionary & PdfVariant::GetDictionary_NoDL()
{
    // Test against eDataType directly not GetDataType() since
    // we don't want to trigger a delayed load (and if required one has
    // already been triggered).
    if( m_eDataType != ePdfDataType_Dictionary )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
    }

    // Do not change this to an reinterpret_cast
    // We need a c-style casts here to avoid crashes
    // because a reinterpret_cast might point to a different position.
    return *((PdfDictionary*)m_Data.pData);
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfReference & PdfVariant::GetReference() const
{
    DelayedLoad();

    if( !IsReference() )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
    }

    // Do not change this to an reinterpret_cast
    // We need a c-style casts here to avoid crashes
    // because a reinterpret_cast might point to a different position.
    return *((PdfReference*)m_Data.pData);
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfVariant::DelayedLoadDone() const
{
    return m_bDelayedLoadDone;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfVariant::EnableDelayedLoading()
{
    m_bDelayedLoadDone = false;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfVariant::DelayedLoadImpl()
{
    // Default implementation of virtual void DelayedLoadImpl() throws, since delayed
    // loading should not be enabled except by types that support it.
    PODOFO_RAISE_ERROR( ePdfError_InternalLogic );
}

// -----------------------------------------------------
//
// -----------------------------------------------------
void PdfVariant::AfterDelayedLoad( EPdfDataType eDataType )
{
    ( void )eDataType;
    // Do nothing
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfVariant::operator!=( const PdfVariant & rhs) const
{
    return !(*this == rhs);
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfVariant::IsDirty() const
{
    // If this is a object with
    // stream, the streams dirty
    // flag might be set.
    if( m_bDirty )
        return m_bDirty;

    switch( m_eDataType ) 
    {
        case ePdfDataType_Array:
        case ePdfDataType_Dictionary:
            // Arrays and Dictionaries
            // handle dirty status by themselfes
            return m_Data.pData->IsDirty();

        case ePdfDataType_Bool:
        case ePdfDataType_Number:
        case ePdfDataType_Real:
        case ePdfDataType_HexString:
        case ePdfDataType_String:
        case ePdfDataType_Name:
        case ePdfDataType_RawData:
        case ePdfDataType_Reference:
        case ePdfDataType_Null:
        case ePdfDataType_Unknown:
        default:
            return m_bDirty;
    };
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfVariant::SetDirty( bool bDirty ) 
{
    m_bDirty = bDirty;

    if( !m_bDirty ) 
    {
        // Propogate new dirty state to subclasses
        switch( m_eDataType ) 
        {
            case ePdfDataType_Array:
            case ePdfDataType_Dictionary:
                // Arrays and Dictionaries
                // handle dirty status by themselfes
                m_Data.pData->SetDirty( m_bDirty );

            case ePdfDataType_Bool:
            case ePdfDataType_Number:
            case ePdfDataType_Real:
            case ePdfDataType_HexString:
            case ePdfDataType_String:
            case ePdfDataType_Name:
            case ePdfDataType_RawData:
            case ePdfDataType_Reference:
            case ePdfDataType_Null:
            case ePdfDataType_Unknown:
            default:
                break;
        };    
    }
}


// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfVariant::SetImmutable(bool bImmutable)
{
    m_bImmutable = bImmutable;

    switch( m_eDataType ) 
    {
        case ePdfDataType_Array:
        case ePdfDataType_Dictionary:
            // Arrays and Dictionaries
            // handle dirty status by themselfes
            m_Data.pData->SetImmutable( m_bImmutable );
            
        case ePdfDataType_Bool:
        case ePdfDataType_Number:
        case ePdfDataType_Real:
        case ePdfDataType_HexString:
        case ePdfDataType_String:
        case ePdfDataType_Name:
        case ePdfDataType_RawData:
        case ePdfDataType_Reference:
        case ePdfDataType_Null:
        case ePdfDataType_Unknown:
        default:
            break;
    };    
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline bool PdfVariant::GetImmutable() const 
{
    return m_bImmutable;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfVariant::AssertMutable() const
{
    if(m_bImmutable) 
    {
        PODOFO_RAISE_ERROR( ePdfError_ChangeOnImmutable );
    }
}

};

#endif // _PDF_VARIANT_H_
