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

#ifndef _PDF_XOBJECT_H_
#define _PDF_XOBJECT_H_

#include "PdfDefines.h"
#include "PdfArray.h"
#include "PdfCanvas.h"
#include "PdfElement.h"
#include "PdfRect.h"

namespace PoDoFo {

class PdfDictionary;
class PdfImageRef;
class PdfObject;

/** A XObject is a content stream with several drawing commands and data
 *  which can be used throughout a PDF document.
 *
 *  You can draw on a XObject like you would draw onto a page and can draw
 *  this XObject later again using a PdfPainter.
 * 
 *  \see PdfPainter
 */
class PdfXObject : public PdfElement, public PdfCanvas {
 public:
    /** Create a new XObject with a specified dimension
     *  in a given vector of PdfObjects
     * 
     *  \param rRect the size of the XObject
     *  \param pParent the parent vector of the XObject
     */
    PdfXObject( const PdfRect & rRect, PdfVecObjects* pParent );
    
    /** Create a XObject from an existing PdfObject
     *  
     *  \param pObject an existing object which has to be
     *                 a XObject
     */
    PdfXObject( PdfObject* pObject );

    /** Get access to the contents object of this page.
     *  If you want to draw onto the page, you have to add 
     *  drawing commands to the stream of the Contents object.
     * 
     *  The contents object is a this pointer in this case.
     *
     *  \returns a contents object
     */
    inline virtual PdfObject* GetContents() const;

   /** Get access to the contents object of this page.
     *  If you want to draw onto the page, you have to add 
     *  drawing commands to the stream of the Contents object.
     * 
     *  The contents object is a this pointer in this case.
     *
     *  \returns a contents object
     */
    inline virtual PdfObject* GetContentsForAppending() const { return GetContents(); }

    /** Get access to the resources object of this page.
     *  This is most likely an internal object.
     *  \returns a resources object
     */
    inline virtual PdfObject* GetResources() const;

    /** Get the current page size in PDF Units
     *  \returns a PdfRect containing the page size available for drawing
     */
    inline virtual const PdfRect GetPageSize() const;

    /** Get the identifier used for drawig this object
     *  \returns identifier
     */
    inline const PdfName & GetIdentifier() const;

 protected:
    PdfXObject( const char* pszSubType, PdfVecObjects* pParent );
    PdfXObject( const char* pszSubType, PdfObject* pObject );

 protected:
    PdfRect          m_rRect;

 private:
    static PdfArray  s_matrix;

    PdfObject*       m_pResources;

    PdfName          m_Identifier;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline PdfObject* PdfXObject::GetContents() const
{
    return m_pObject;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline PdfObject* PdfXObject::GetResources() const
{
    return m_pResources;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline const PdfRect PdfXObject::GetPageSize() const
{
    return m_rRect;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline const PdfName & PdfXObject::GetIdentifier() const
{
    return m_Identifier;
}

};

#endif /* _PDF_XOBJECT_H_ */


