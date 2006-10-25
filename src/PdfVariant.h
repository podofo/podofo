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
 ***************************************************************************/

#ifndef _PDF_VARIANT_H_
#define _PDF_VARIANT_H_

#include <cmath>

#include "PdfDefines.h"
#include "PdfRefCountedBuffer.h"

namespace PoDoFo {

class PdfArray;
class PdfDataType;
class PdfDictionary;
class PdfName;
class PdfOutputDevice;
class PdfString;
class PdfReference;

/**
 * A variant data type which supports all data types supported by the PDF standard.
 * The data can be parsed directly from a string or set by one of the members.
 * One can also convert the variant back to a string after setting the values.
 *
 * TODO: domseichter: Make this class implicitly shared
 */
class PODOFO_API PdfVariant {
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
    PdfVariant( long l );

    /** Construct a PdfVariant that is a real number.
     *  \param d the value of the real number.
     */    
    PdfVariant( double d );

    /** Construct a PdfVariant that is a string.
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

    /** Initalize a PdfVariant object with array data.
     *  The variant will automatically get the datatype
     *  ePdfDataType_Array. This Init call is the fastest
     *  way to create a new PdfVariant that is an array.
     *
     *  \param tList a list of variants
     *
     *  \returns ErrOk on sucess
     */
    PdfVariant( const PdfArray & tList );

    /** Construct a PdfVariant that is a dictionary.
     *  \param rDict the value of the dictionary.
     */        
    PdfVariant( const PdfDictionary & rDict );

    /** Constructs a new PdfVariant which has the same 
     *  contents as rhs.
     *  \param rhs an existing variant which is copied.
     */
    PdfVariant( const PdfVariant & rhs );

    virtual ~PdfVariant();
    
    /** Initialize a PdfVariant object from string data.
     *  \param pszData a string that will be parsed.
     *  \param nLen length of the buffer, if 0 the buffer is assumed to be zero terminated.
     *  \param pLen if not null the length of the string that was 
     *              parsed is returned in this parameter
     *  \returns ErrOk on success
     */
    void Parse( const char* pszData, int nLen = 0, long* pLen = NULL );

    /** \returns true if this PdfVariant is empty.
     *           i.e. m_eDataType == ePdfDataType_Null
     */
    inline bool IsEmpty() const;

    /** Clear all internal member variables and free the memory
     *  they have allocated.
     *  Sets the datatype to ePdfDataType_Null
     */
    void Clear();

    /** \returns the datatype of this object or ePdfDataType_Unknown
     *  if it does not have a value.
     */
    inline const EPdfDataType GetDataType() const;

    /** \returns a human readable string representation of GetDataType()
     *  The returned string must not be free'd.
     */
    const char * GetDataTypeString() const;

    /** \returns true if this variant is a bool (i.e. GetDataType() == ePdfDataType_Bool)
     */
    inline bool IsBool() const { DelayedLoad(); return GetDataType() == ePdfDataType_Bool; }

    /** \returns true if this variant is a number (i.e. GetDataType() == ePdfDataType_Number)
     */
    inline bool IsNumber() const { DelayedLoad(); return GetDataType() == ePdfDataType_Number; }

    /** \returns true if this variant is a real (i.e. GetDataType() == ePdfDataType_Real)
     */
    inline bool IsReal() const { DelayedLoad(); return GetDataType() == ePdfDataType_Real; }

    /** \returns true if this variant is a string (i.e. GetDataType() == ePdfDataType_String)
     */
    inline bool IsString() const { DelayedLoad(); return GetDataType() == ePdfDataType_String; }

    /** \returns true if this variant is a hex-string (i.e. GetDataType() == ePdfDataType_HexString)
     */
    inline bool IsHexString() const { DelayedLoad(); return GetDataType() == ePdfDataType_HexString; }

    /** \returns true if this variant is a name (i.e. GetDataType() == ePdfDataType_Name)
     */
    inline bool IsName() const { DelayedLoad(); return GetDataType() == ePdfDataType_Name; }

    /** \returns true if this variant is an array (i.e. GetDataType() == ePdfDataType_Array)
     */
    inline bool IsArray() const { DelayedLoad(); return GetDataType() == ePdfDataType_Array; }

    /** \returns true if this variant is a dictionary (i.e. GetDataType() == ePdfDataType_Dictionary)
     */
    inline bool IsDictionary() const { DelayedLoad(); return GetDataType() == ePdfDataType_Dictionary; }

    /** \returns true if this variant is a stream (i.e. GetDataType() == ePdfDataType_Stream)
     */
    //inline bool IsStream() const { DelayedLoad(); return GetDataType() == ePdfDataType_Stream; }

    /** \returns true if this variant is null (i.e. GetDataType() == ePdfDataType_Null)
     */
    inline bool IsNull() const { DelayedLoad(); return GetDataType() == ePdfDataType_Null; }

    /** \returns true if this variant is a reference (i.e. GetDataType() == ePdfDataType_Reference)
     */
    inline bool IsReference() const { DelayedLoad(); return GetDataType() == ePdfDataType_Reference; }
       
    /** Write the complete variant to an output device.
     *  This is an overloaded member function.
     *
     *  \param pDevice write the object to this device
     */
    void Write( PdfOutputDevice* pDevice ) const;

    /** Write the complete variant to an output device.
     *  \param pDevice write the object to this device
     *  \param keyStop if not KeyNull and a key == keyStop is found
     *                 writing will stop right before this key!
     *                 if IsDictionary returns true.
     */
    virtual void Write( PdfOutputDevice* pDevice, const PdfName & keyStop ) const;

    /** Converts the current object into a string representation
     *  which can be written directly to a PDF file on disc.
     *  \param rsData the object string is returned in this object.
     */
    void ToString( std::string & rsData ) const;

    /** Set the value of this object as bool
     *  \param b the value as bool.
     */
    inline void SetBool( bool b );

    /** Get the value if this object is a bool.
     *  \returns the bool value.
     */
    inline bool GetBool() const;

    /** Set the value of this object as long
     *  \param l the value as long.
     */
    inline void SetNumber( long l );

    /** Get the value of the object as long.
     *  \return the value of the number
     */
    inline long GetNumber() const;

    /** Set the value of this object as double
     *  \param d the value as double.
     */
    inline void SetReal( double d );

    /** Get the value of the object as double.
     *  \return the value of the number
     */
    inline double GetReal() const;

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
     *  \returns a PdfReference
     */
    inline PdfReference & GetReference();

    /** Assign the values of another PdfVariant to this one.
     *  \param rhs an existing variant which is copied.
     */
    const PdfVariant & operator=( const PdfVariant & rhs );

    /** The PdfVariant object is padded with spaces to lLength
     *  if this property is set when it is written using ToString.
     *  
     *  \param lLength padding length
     *  \see ToString
     */
    inline void SetPaddingLength( long lLength );

 protected:

    /**
     *  For subclasses that implement loading on demand
     */
    inline virtual void DelayedLoad() const {};

 private:
    void DetermineDataType( const char* pszData, long nLen, EPdfDataType* eDataType, long* pLen = NULL );

 private:
    /** To reduce memory usage of this very often used class,
     *  we use a union here, as there is always only
     *  one of those members used.
     */
    typedef union { 
        bool       bBoolValue;
        double     dNumber;
        long       nNumber;
    } UVariant;

    UVariant     m_Data;

    /** Datatype of the variant.
     *  required to access the correct member of 
     *  the union UVariant.
     */
    EPdfDataType m_eDataType;

    /** Holds references, strings, 
     *  names, dictionaries and arrays
     */
    PdfDataType* m_pData;

    /** if not 0 the object is padded with spaces
     *  to this length
     */
    int         m_nPadding;
};

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
const EPdfDataType PdfVariant::GetDataType() const
{
    DelayedLoad();

    return m_eDataType;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfVariant::SetPaddingLength( long lLength )
{
    m_nPadding = lLength;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfVariant::SetBool( bool b )
{
    DelayedLoad();

    if( !IsBool() )
    {
        RAISE_ERROR( ePdfError_InvalidDataType );
    }

    m_Data.bBoolValue = b;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfVariant::GetBool() const
{
    DelayedLoad();

    if( !IsBool() )
    {
        RAISE_ERROR( ePdfError_InvalidDataType );
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
        RAISE_ERROR( ePdfError_InvalidDataType );
    }

    if ( IsReal() )
        m_Data.dNumber = static_cast<double>(l);
    else
        m_Data.nNumber = l;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
long PdfVariant::GetNumber() const
{
    DelayedLoad();

    if( !IsReal() && !IsNumber() )
    {
        RAISE_ERROR( ePdfError_InvalidDataType );
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
        RAISE_ERROR( ePdfError_InvalidDataType );
    }

    if ( IsReal() )
        m_Data.dNumber = d;
    else
        m_Data.nNumber = static_cast<long>(floor( d ));
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfVariant::GetReal() const
{
    DelayedLoad();

    if( !IsReal() && !IsNumber() )
    {
        RAISE_ERROR( ePdfError_InvalidDataType );
    }

    if ( IsReal() )
        return m_Data.dNumber;
    else
        return static_cast<double>(m_Data.nNumber);
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfString & PdfVariant::GetString() const
{
    DelayedLoad();

    if( !IsString() )
    {
        RAISE_ERROR( ePdfError_InvalidDataType );
    }

    return *(reinterpret_cast<PdfString* const>(m_pData));
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfName & PdfVariant::GetName() const
{
    DelayedLoad();

    if( !IsName() )
    {
        RAISE_ERROR( ePdfError_InvalidDataType );
    }

    return *(reinterpret_cast<PdfName*>(m_pData));
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfArray & PdfVariant::GetArray() const
{
    DelayedLoad();

    if( !IsArray() )
    {
        RAISE_ERROR( ePdfError_InvalidDataType );
    }

    return *(reinterpret_cast<PdfArray* const>(m_pData));
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfArray & PdfVariant::GetArray()
{
    DelayedLoad();

    if( !IsArray() )
    {
        RAISE_ERROR( ePdfError_InvalidDataType );
    }
    
    return *(reinterpret_cast<PdfArray* const>(m_pData));
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfDictionary & PdfVariant::GetDictionary() const
{
    DelayedLoad();

    if( !IsDictionary() )
    {
        RAISE_ERROR( ePdfError_InvalidDataType );
    }

    return *(reinterpret_cast<PdfDictionary* const>(m_pData));
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfDictionary & PdfVariant::GetDictionary()
{
    DelayedLoad();

    if( !IsDictionary() )
    {
        RAISE_ERROR( ePdfError_InvalidDataType );
    }

    return *(reinterpret_cast<PdfDictionary* const>(m_pData));
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfReference & PdfVariant::GetReference() const
{
    DelayedLoad();

    if( !IsReference() )
    {
        RAISE_ERROR( ePdfError_InvalidDataType );
    }

    return *(reinterpret_cast<PdfReference* const>(m_pData));
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline PdfReference & PdfVariant::GetReference()
{
    DelayedLoad();

    if( !IsReference() )
    {
        RAISE_ERROR( ePdfError_InvalidDataType );
    }

    return *(reinterpret_cast<PdfReference* const>(m_pData));
}

};

#endif // _PDF_VARIANT_H_
