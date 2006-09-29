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

#ifndef _PDF_ELEMENT_H_
#define _PDF_ELEMENT_H_

#include "PdfDefines.h"

namespace PoDoFo {

class PdfObject;
class PdfVecObjects;

/** PdfElement is a common base class for all elements
 *  in a PDF file. For example pages, action and annotations.
 *
 *  Every PDF element has one PdfObject and provides an easier
 *  interface to modify the contents of the dictionary. 
 *  
 *  A PdfElement base class can be created from an existing PdfObject
 *  or created from scratch. In the later case, the PdfElement creates
 *  a PdfObject and adds it to a vector of objects.
 *
 *  A PdfElement cannot be created directly. Use one
 *  of the subclasses which implement real functionallity.
 *
 *  \see PdfPage \see PdfAction \see PdfAnnotation
 */
class PdfElement {

 public:

    virtual ~PdfElement();

    /** Get access to the internal object
     *  \returns the internal PdfObject
     */
    inline PdfObject* GetObject();

    /** Get access to the internal object
     *  This is an overloaded member function.
     *
     *  \returns the internal PdfObject
     */
    inline const PdfObject* GetObject() const;

 protected:
    /** Creates a new PdfElement 
     *  \param pszType type entry of the elements object
     *  \param pParent parent vector of objects.
     *                 Add a newly created object to this vector.
     */
    PdfElement( const char* pszType, PdfVecObjects* pParent );

    /** Create a PdfElement from an existing PdfObject
     *  \param pszType type entry of the elements object.
     *                 Throws an exception if the type in the 
     *                 PdfObject differs from pszType.
     *  \param pObject pointer to the PdfObject that is modified
     *                 by this PdfElement
     */
    PdfElement( const char* pszType, PdfObject* pObject );

    /** Convert an enum or index to its string representation
     *  which can be written to the PDF file.
     * 
     *  This is a helper function for various PdfElement 
     *  subclasses that need strings and enums for their
     *  SubTypes keys.
     *
     *  \param i the index or enum value
     *  \param ppTypes an array of strings containing
     *         the string mapping of the index
     *  \param lLen the length of the string array
     *
     *  \returns the string representation or NULL for 
     *           values out of range
     */
    const char* TypeNameForIndex( int i, const char** ppTypes, long lLen ) const;

    /** Convert a string type to an array index or enum.
     * 
     *  This is a helper function for various PdfElement 
     *  subclasses that need strings and enums for their
     *  SubTypes keys.
     *
     *  \param pszType the type as string
     *  \param ppTypes an array of strings containing
     *         the string mapping of the index
     *  \param lLen the length of the string array
     *
     *  \returns the index of the string in the array
     */
    int TypeNameToIndex( const char* pszType, const char** ppTypes, long lLen ) const;

 protected:
    PdfObject* m_pObject;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline PdfObject* PdfElement::GetObject()
{
    return m_pObject;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline const PdfObject* PdfElement::GetObject() const
{
    return m_pObject;
}

};

#endif // PDF_ELEMENT_H_
