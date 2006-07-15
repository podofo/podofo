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

#ifndef _PDF_PAGE_H_
#define _PDF_PAGE_H_

#include "PdfDefines.h"

#include "PdfArray.h"
#include "PdfCanvas.h"
#include "PdfObject.h"
#include "PdfRect.h"

namespace PoDoFo {

class PdfDocument;
class PdfDictionary;
class PdfVecObjects;

/** PdfPage is one page in the pdf document. 
 *  It is possible to draw on a page using a PdfPainter object.
 *  Every document needs at least one page.
 */
class PdfPage : public PdfCanvas {
 public:
    /** Create a new PdfPage object.
     *  \param inOwningDoc the document this page belongs to 
     *  \param nObjectNo object no
     *  \param nGenerationNo generation number of the object
     */
    PdfPage( PdfDocument* inOwningDoc, unsigned int nObjectNo, unsigned int nGenerationNo );
 
    /** Create a new PdfPage object.
	 *  \param inOwningDoc the document this page belongs to 
     *  \param inObject the object from an existing PDF
     */
    PdfPage( PdfDocument* inOwningDoc, PdfObject* inObject );

    virtual ~PdfPage();

    /** Initialize a newly generated PdfPage object. If you use the CreatePage method of PdfSimpleWriter
     *  you do not have to call this method.
     *  \param tSize a size structure specifying the size of the page (i.e the /MediaBox key) in PDF units
     *  \param pParent pointer to the parent PdfVecObjects object
     *  \returns ErrOk on success
     */
    void Init( const TSize & tSize, PdfVecObjects* pParent );

    /** Creates a TSize page size object which is needed to create a PdfPage object
     *  from an enum which are defined for a few standard page sizes.
     *  \param ePageSize the page size you want
     *  \returns TSize object which can be passed to the PdfPage constructor
     */
    static TSize CreateStandardPageSize( const EPdfPageSize ePageSize );

    /** Retrieve the actual object for a given page
     *   \returns the PdfObject for this page
     */
    PdfObject* GetObject() const { return m_pObject; }

    /** Get access to the contents object of this page.
     *  If you want to draw onto the page, you have to add 
     *  drawing commands to the stream of the Contents object.
     *  \returns a contents object
     */
    virtual PdfObject* Contents() const { return m_pContents; }

    /** Get access to the resources object of this page.
     *  This is most likely an internal object.
     *  \returns a resources object
     */
    inline virtual PdfObject* Resources() const;

    /** Get the current page size in PDF units.
     *  \returns TSize the page size
     */
    virtual const TSize & PageSize() const { return m_tPageSize; }

    /** Get the current MediaBox (physical page size) in PDF units.
     *  \returns PdfRect the page box
     */
    virtual const PdfRect GetMediaBox() const { return GetPageBox( "MediaBox" ); }

    /** Get the current CropBox (visible page size) in PDF units.
     *  \returns PdfRect the page box
     */
    virtual const PdfRect GetCropBox() const { return GetPageBox( "CropBox" ); }

    /** Get the current TrimBox (cut area) in PDF units.
     *  \returns PdfRect the page box
     */
    virtual const PdfRect GetTrimBox() const { return GetPageBox( "TrimBox" ); }

    /** Get the current BleedBox (extra area for printing purposes) in PDF units.
     *  \returns PdfRect the page box
     */
    virtual const PdfRect GetBleedBox() const { return GetPageBox( "BleedBox" ); }

    /** Get the current ArtBox in PDF units.
     *  \returns PdfRect the page box
     */
    virtual const PdfRect GetArtBox() const { return GetPageBox( "ArtBox" ); }

    /** Get the current page rotation (if any).
     *  \returns int 0, 90, 180 or 270
     */
    virtual const int GetRotation() const;
        
    /** Get the number of annotations associated with this page
     * \ returns int number of annotations
     */
    virtual const int GetNumAnnots() const;

 private:

   /** Get the bounds of a specified page box in PDF units.
     * This function is internal, since there are wrappers for all standard boxes
     *  \returns PdfRect the page box
     */
    const PdfRect GetPageBox( const char* inBox ) const;
    
    /** Private method for getting a key value that could be inherited (such as the boxes, resources, etc.)
     *  \returns PdfVariant - the result of the key fetching
     */
    PdfObject* GetInheritedKeyFromObject( const char* inKey, PdfObject* inObject ) const; 


 private:
    PdfDocument*   m_pDocument;
    PdfObject*     m_pObject;
    PdfObject*     m_pContents;
    PdfObject*     m_pResources;
    TSize          m_tPageSize;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline PdfObject* PdfPage::Resources() const
{
    return m_pResources;
}

};

#endif // _PDF_PAGE_H_
