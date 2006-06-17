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
#include "PdfName.h"
#include "PdfReference.h"
#include "PdfString.h"

namespace PoDoFo {

class PdfVariant;

typedef std::vector<PdfVariant>      TVariantList;
typedef TVariantList::iterator       TIVariantList;
typedef TVariantList::const_iterator TCIVariantList;

/**
 * A variant data type which supports all data types supported by the PDF standard.
 * The data can be parsed directly from a string or set by one of the members.
 * One can also convert the variant back to a string after setting the values.
 */
class PdfVariant {
 public:
    /** Construct an empty variant type
     */
    PdfVariant();

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
    PdfError Init( const char* pszData, int nLen = 0, long* pLen = NULL );

    /** Initialize a PdfVariant with data. This function does not parse
     *  the data, but assumes that it is already in the correct format
     *  for the specified datatype.
     *  If you want to initialize with an key /FlateDecode for example
     *  the call would look like: Init( "FlateDecode", ePdfDataType_Name );
     *  No leading slash is needed, as the data type is specified
     *  by the enum.
     *  
     *  This function is much faster than the Init version which 
     *  has to parse all of the string data. It is not faster
     *  for array and reference types though, as these will be parsed anyways.
     *
     *  \param pszData string containing the data
     *  \param eDataType data type of pszData
     *
     *  \returns ErrOk on sucess
     */
    PdfError Init( const char* pszData, EPdfDataType eDataType );

    /** Initalize a PdfVariant object with array data.
     *  The variant will automatically get the datatype
     *  ePdfDataType_Array. This Init call is the fastest
     *  way to create a new PdfVariant that is an array.
     *
     *  \param tList a list of variants
     *
     *  \returns ErrOk on sucess
     */
    PdfError Init( const TVariantList & tList );

    /** \returns true if this PdfVariant is empty.
     *           i.e. m_eDataType == ePdfDataType_Unknown
     */
    inline bool IsEmpty() const;

    /** Clear all internal member variables and free the memory
     *  they have allocated.
     */
    void Clear();

    /** \returns the datatype of this object or ePdfDataType_Unknow 
     *  if it does not have a value.
     */
    inline const EPdfDataType GetDataType() const;

    /** Set the data type of this PdfVariant object.
     *  You will have to call one of the set methods next
     *  to set a value for this variant. Do not call init()
     *  after this function.
     *
     *  \param eDataType the data type of this variant.
     */        
    inline void SetDataType( EPdfDataType eDataType );
       
    /** Converts the current object into a string representation
     *  which can be written directly to a PDF file on disc.
     *  \param rsData the object string is returned in this object.
     *  \returns ErrOk on success
     */
    PdfError ToString( std::string & rsData ) const;

    /** Get the value of the object as bool.
     *  \param pBool pointer to a bool where the value can be stored
     *  \returns an error if GetDataType() != ePdfDataType_Bool
     */
    PdfError GetBool( bool* pBool ) const;

    /** Get the value of the object as long.
     *  \param pNum pointer to a long where the number can be stored
     *  \returns an error if GetDataType() != ePdfDataType_Number
     */
    PdfError GetNumber( long* pNum ) const;

    /** Set the value of this variant.
     *  \param lNum new value of this variant.
     *  \returns an error if GetDataType() != ePdfDataType_Number
     */
    PdfError SetNumber( long lNum );

    /** Get the value of the object as double.
     *  \param pNum pointer to a double where the number can be stored
     *  \returns an error if GetDataType() != ePdfDataType_Real
     */
    PdfError GetNumber( double* pNum ) const;

    /** Set the value of this variant.
     *  \param dNum new value of this variant.
     *  \returns an error if GetDataType() != ePdfDataType_Number
     */
    PdfError SetNumber( double dNum );

    /** Set the value of this variant.
     *  \param rsName new value of this variant.
     *  \returns an error if GetDataType() != ePdfDataType_Name
     */
    PdfError SetName( const PdfName & rsName );

    /** Set the value of this variant.
     *  \param rsString new value of this variant.
     *  \returns an error if GetDataType() != ePdfDataType_String or ePdfDataType_HexString
     */
    PdfError SetString( const PdfString & rsString );

    /** \returns the value of the object as string.
     */
    const PdfString & GetString() const;

    /** \returns the value of the object as name
     */
    const PdfName & GetName() const;

    /** Returns the value of the object as array
     *  \returns a array
     */
    const TVariantList & GetArray() const;

    /** Set the array value.
     *  \returns an error if GetDataType() != ePdfDataType_Array
     */
    PdfError SetArray( const TVariantList & vArray );

    /** Returns the dictionary value of this object
     *  \returns a PdfObject
     */
    const PdfObject & GetDictionary() const; 

    /** Get the reference values of this object.
     *  \returns a PdfReference
     */
    const PdfReference & GetReference() const;

    /** Set the variants reference values
     *  \param ref PdfReference
     *  \returns an error if GetDataType() != ePdfDataType_Reference
     */
    PdfError SetReference( const PdfReference & ref );

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

    /** Stores the values of arrays.
     */
    TVariantList m_vecArray;

    /** Holds a dictionary
     */
    PdfObject*   m_pDictionary;

    /** if not 0 the object is padded with spaces
     *  to this length
     */
    int         m_nPadding;
};

bool PdfVariant::IsEmpty() const
{
    return (m_eDataType == ePdfDataType_Unknown);
}

const EPdfDataType PdfVariant::GetDataType() const
{
    return m_eDataType;
}

void PdfVariant::SetDataType( EPdfDataType eDataType )
{
    m_eDataType = eDataType;
}

void PdfVariant::SetPaddingLength( long lLength )
{
    m_nPadding = lLength;
}

};

#endif // _PDF_VARIANT_H_
