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
#include "PdfContents.h"
#include "PdfElement.h"
#include "PdfRect.h"

namespace PoDoFo {

class PdfDocument;
class PdfDictionary;
class PdfVecObjects;

/** PdfPage is one page in the pdf document. 
 *  It is possible to draw on a page using a PdfPainter object.
 *  Every document needs at least one page.
 */
class PdfPage : public PdfElement, public PdfCanvas {
 public:
    /** Create a new PdfPage object.
     *  \param rSize a PdfRect specifying the size of the page (i.e the /MediaBox key) in PDF units
     *  \param pParent add the page to this parent
     */
    PdfPage( const PdfRect & rSize, PdfVecObjects* pParent );
 
    /** Create a PdfPage based on an existing PdfObject
     *  \param pObject an existing PdfObject
     */
    PdfPage( PdfObject* pObject );

    virtual ~PdfPage();

    /** Get the current page size in PDF Units
     *  \returns a PdfRect containing the page size available for drawing
     */
    inline virtual const PdfRect GetPageSize() const;

    /** Creates a PdfRect with the page size as values which is needed to create a PdfPage object
     *  from an enum which are defined for a few standard page sizes.
     *
     *  \param ePageSize the page size you want
     *  \returns a PdfRect object which can be passed to the PdfPage constructor
     */
    static PdfRect CreateStandardPageSize( const EPdfPageSize ePageSize );

    /** Get access to the contents object of this page.
     *  If you want to draw onto the page, you have to add 
     *  drawing commands to the stream of the Contents object.
     *  \returns a contents object
     */
    virtual PdfObject* GetContents() const { return m_pContents->GetContents(); }

    /** Get access an object that you can use to ADD drawing to.
     *  If you want to draw onto the page, you have to add 
     *  drawing commands to the stream of the Contents object.
     *  \returns a contents object
     */
    virtual PdfObject* GetContentsForAppending() const { return m_pContents->GetContentsForAppending(); }

    /** Get access to the resources object of this page.
     *  This is most likely an internal object.
     *  \returns a resources object
     */
    inline virtual PdfObject* GetResources() const;

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

    /** Get the annotation with index index of the current page.
     *  \param index the index of the annotation to retrieve
     *
     *  \returns a annotation object
     *
     *  \see GetNumAnnots
     */
    //virtual PdfAnnotation GetAnnotation( int index );

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
    PdfContents*   m_pContents;
    PdfObject*     m_pResources;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline PdfObject* PdfPage::GetResources() const
{
    return m_pResources;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline const PdfRect PdfPage::GetPageSize() const
{
    return this->GetMediaBox();
}

};

#endif // _PDF_PAGE_H_
