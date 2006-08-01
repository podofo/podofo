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

    /** Get access to the interna object
     *  \returns the internal PdfObject
     */
    inline PdfObject* Object();

    /** Get access to the interna object
     *  This is an overloaded member function.
     *
     *  \returns the internal PdfObject
     */
    inline const PdfObject* Object() const;

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

 protected:
    PdfObject* m_pObject;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline PdfObject* PdfElement::Object()
{
    return m_pObject;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline const PdfObject* PdfElement::Object() const
{
    return m_pObject;
}

};

#endif // PDF_ELEMENT_H_
