/***************************************************************************
 *   Copyright (C) 2006 by Dominik Seichter                                *
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

#ifndef _PDF_ARRAY_H_
#define _PDF_ARRAY_H_

#include "PdfDefines.h"
#include "PdfDataType.h"
#include "PdfObject.h"

namespace PoDoFo {

/** This class represents a PdfArray
 *  Use it for all arrays that are written to a PDF file.
 *  
 *  A PdfArray can hold any PdfVariant.
 *
 *  \see PdfVariant
 */
class PODOFO_API PdfArray : public std::vector<PdfObject>, public PdfDataType {
 public:

    /** Create an empty array 
     */
    PdfArray();

    /** Create an array and add one value to it.
     *  The value is copied.
     *
     *  \param var add this object to the array.
     */
    PdfArray( const PdfObject & var );

    /** Deep copy an existing PdfArray
     *
     *  \param rhs the array to copy
     */
    PdfArray( const PdfArray & rhs );

    virtual ~PdfArray();

    /** 
     *  \returns the size of the array
     */
    inline size_t GetSize() const;

    /** Remove all elements from the array
     */
    inline void Clear();

    /** Write the array to an output device.
     *  This is an overloaded member function.
     *
     *  \param pDevice write the object to this device
     *  \param pEncrypt an encryption object which is used to encrypt this object
     *                  or NULL to not encrypt this object
     */
    void Write( PdfOutputDevice* pDevice, const PdfEncrypt* pEncrypt = NULL ) const;

    /** Utility method to determine if the array contains
     *  contains any objects of ePdfDataType_String whose
     *  value is the passed string.
     *  \param cmpString the string to compare against
     *  \returns true if success, false if not
     */
    bool ContainsString( const std::string& cmpString ) const;
    
    /** Utility method to return the actual index in the
     *  array which contains an object of ePdfDataType_String whose
     *  value is the passed string.
     *  \param cmpString the string to compare against
     *  \returns true if success, false if not
     */
    size_t GetStringIndex( const std::string& cmpString ) const;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfArray::Clear() 
{
    this->clear();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
size_t PdfArray::GetSize() const
{
    return this->size();
}

typedef PdfArray                 TVariantList;
typedef PdfArray::iterator       TIVariantList;
typedef PdfArray::const_iterator TCIVariantList;

};

#endif // _PDF_ARRAY_H_
