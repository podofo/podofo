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

#ifndef _PDF_XOBJECT_H_
#define _PDF_XOBJECT_H_

#include "podofo/base/PdfDefines.h"
#include "podofo/base/PdfArray.h"
#include "podofo/base/PdfCanvas.h"
#include "podofo/base/PdfRect.h"

#include "PdfElement.h"

namespace PoDoFo {

class PdfDictionary;
class PdfObject;
class PdfMemDocument;

/** A XObject is a content stream with several drawing commands and data
 *  which can be used throughout a PDF document.
 *
 *  You can draw on a XObject like you would draw onto a page and can draw
 *  this XObject later again using a PdfPainter.
 * 
 *  \see PdfPainter
 */
class PODOFO_DOC_API PdfXObject : public PdfElement, public PdfCanvas {
 public:
    /** Create a new XObject with a specified dimension
     *  in a given document
     * 
     *  \param rRect the size of the XObject
     *  \param pParent the parent document of the XObject
	 *  \param pszPrefix optional prefix for XObject-name
     *  \param bWithoutObjNum do not create an object identifier name
     */
    PdfXObject( const PdfRect & rRect, PdfDocument* pParent, const char* pszPrefix = NULL, bool bWithoutObjNum = false);

    /** Create a new XObject with a specified dimension
     *  in a given vector of PdfObjects
     * 
     *  \param rRect the size of the XObject
     *  \param pParent the parent vector of the XObject
	 *  \param pszPrefix optional prefix for XObject-name
     */
    PdfXObject( const PdfRect & rRect, PdfVecObjects* pParent, const char* pszPrefix = NULL );
    
    /** Create a new XObject from a page of another document
     *  in a given document
     * 
     *  \param rSourceDoc the document to create the XObject from
     *  \param nPage the page-number in rDoc to create the XObject from
     *  \param pParent the parent document of the XObject
	 *  \param pszPrefix optional prefix for XObject-name
 	 *	\param bUseTrimBox if true try to use trimbox for size of xobject
     */
    PdfXObject( const PdfMemDocument & rSourceDoc, int nPage, PdfDocument* pParent, const char* pszPrefix = NULL, bool bUseTrimBox = false );

    /** Create a new XObject from an existing page
     * 
     *  \param pDoc the document to create the XObject at
     *  \param nPage the page-number in pDoc to create the XObject from
     *  \param pszPrefix optional prefix for XObject-name
     *  \param bUseTrimBox if true try to use trimbox for size of xobject
     */
    PdfXObject( PdfDocument *pDoc, int nPage, const char* pszPrefix = NULL, bool bUseTrimBox = false );

    /** Create a XObject from an existing PdfObject
     *  
     *  \param pObject an existing object which has to be
     *                 a XObject
     */
    PdfXObject( PdfObject* pObject );

    virtual ~PdfXObject() { }

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

    /** Get the reference to the XObject in the PDF file
     *  without having to access the PdfObject.
     *
     *  This allows to work with XObjects which have been 
     *  written to disk already.
     *
     *  \returns the reference of the PdfObject for this XObject
     */
    inline const PdfReference & GetObjectReference() const;

 protected:
    void InitXObject( const PdfRect & rRect, const char* pszPrefix = NULL );

    PdfXObject( const char* pszSubType, PdfDocument* pParent, const char* pszPrefix = NULL );
    PdfXObject( const char* pszSubType, PdfVecObjects* pParent, const char* pszPrefix = NULL );
    PdfXObject( const char* pszSubType, PdfObject* pObject );

 protected:
    PdfRect          m_rRect;

 private:
    PdfArray         m_matrix;

    PdfObject*       m_pResources;

    PdfName          m_Identifier;
    PdfReference     m_Reference;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline PdfObject* PdfXObject::GetContents() const
{
    return this->GetNonConstObject();
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

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline const PdfReference & PdfXObject::GetObjectReference() const
{
    return m_Reference;
}

};

#endif /* _PDF_XOBJECT_H_ */


