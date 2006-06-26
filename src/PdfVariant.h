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

#include "PdfDefines.h"

#include "PdfReference.h"

namespace PoDoFo {

class PdfArray;
class PdfName;
class PdfString;

/**
 * A variant data type which supports all data types supported by the PDF standard.
 * The data can be parsed directly from a string or set by one of the members.
 * One can also convert the variant back to a string after setting the values.
 *
 * TODO: domseichter: Make this class implicitly shared
 */
class PdfVariant {
 public:
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
     *  \param rObj the value of the dictionary.
     */        
    PdfVariant( const PdfObject & rObj );

    /** Constructs a new PdfVariant which has the same 
     *  contents as rhs.
     *  \param rhs an existing variant which is copied.
     */
    PdfVariant( const PdfVariant & rhs );

    ~PdfVariant();
    
    /** Initialize a PdfVariant object from string data.
     *  \param pszData a string that will be parsed.
     *  \param nLen length of the buffer, if 0 the buffer is assumed to be zero terminated.
     *  \param pLen if not null the length of the string that was 
     *              parsed is returned in this parameter
     *  \returns ErrOk on success
     */
    PdfError Parse( const char* pszData, int nLen = 0, long* pLen = NULL );

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

    /** \returns true if this variant is a stream (i.e. GetDataType() == ePdfDataType_Stream)
     */
    inline bool IsStream() const { return GetDataType() == ePdfDataType_Stream; }

    /** \returns true if this variant is null (i.e. GetDataType() == ePdfDataType_Null)
     */
    inline bool IsNull() const { return GetDataType() == ePdfDataType_Null; }

    /** \returns true if this variant is a reference (i.e. GetDataType() == ePdfDataType_Reference)
     */
    inline bool IsReference() const { return GetDataType() == ePdfDataType_Reference; }
       
    /** Converts the current object into a string representation
     *  which can be written directly to a PDF file on disc.
     *  \param rsData the object string is returned in this object.
     *  \returns ErrOk on success
     */
    PdfError ToString( std::string & rsData ) const;

    /** Get the value if this object is a bool.
     *  \returns the bool value.
     */
    inline bool GetBool() const;

    /** Get the value of the object as long.
     *  \return the value of the number
     */
    inline long GetNumber() const;

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

    /** Returns the dictionary value of this object
     *  \returns a PdfObject
     */
    inline const PdfObject & GetDictionary() const; 

    /** Get the reference values of this object.
     *  \returns a PdfReference
     */
    inline const PdfReference & GetReference() const;

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

 private:
    PdfError GetDataType( const char* pszData, long nLen, EPdfDataType* eDataType, long* pLen = NULL );

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

    /** Holds the reference value
     */
    PdfReference m_reference;

    /** Holds the values of strings
     */
    PdfString * m_pString;

    /** Holds the value of names
     */
    PdfName* m_pName;

    /** Holds a dictionary
     */
    PdfObject*   m_pDictionary;

    /** Stores the values of arrays.
     */
    PdfArray*  m_pArray;

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
    return (m_eDataType == ePdfDataType_Null);
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const EPdfDataType PdfVariant::GetDataType() const
{
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
bool PdfVariant::GetBool() const
{
    return m_Data.bBoolValue;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
long PdfVariant::GetNumber() const
{
    return m_Data.nNumber;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfVariant::GetReal() const
{
    return m_Data.dNumber;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfString & PdfVariant::GetString() const
{
    return *m_pString;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfName & PdfVariant::GetName() const
{
    return *m_pName;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfArray & PdfVariant::GetArray() const
{
    return *m_pArray;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfObject & PdfVariant::GetDictionary() const
{
    return *m_pDictionary;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfReference & PdfVariant::GetReference() const
{
    return m_reference;
}

};

#endif // _PDF_VARIANT_H_
