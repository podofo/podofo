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
#include "PdfObject.h"

namespace PoDoFo {

class PdfDictionary;
class PdfRect;
class PdfImageRef;

/** A XObject is a content stream with several drawing commands and data
 *  which can be used throughout a PDF document.
 *
 *  You can draw on a XObject like you would draw onto a page and can draw
 *  this XObject later again using a PdfPainter.
 * 
 *  \see PdfPainter
 */
class PdfXObject : public PdfObject, public PdfCanvas {
 public:
    PdfXObject( unsigned int nObjectNo, unsigned int nGenerationNo );

    PdfError Init( const PdfRect & rRect );

    /** Get access to the contents object of this page.
     *  If you want to draw onto the page, you have to add 
     *  drawing commands to the stream of the Contents object.
     * 
     *  The contents object is a this pointer in this case.
     *
     *  \returns a contents object
     */
    inline PdfObject* Contents() const;

    /** Get access to the resources object of this page.
     *  This is most likely an internal object.
     *  \returns a resources object
     */
    inline PdfObject* Resources() const;

    /** Get the current page size in 1/1000th mm.
     *  \returns TSize the page size
     */
    virtual inline const TSize & PageSize() const;

    /** Get an image reference object to this XObject.
     *  You can pass this image reference object to the
     *  DrawXObject method of PdfPainter.
     * 
     *  The PdfImageRef is very small in memory and contains
     *  all the information that is needed for drawing. I.e. 
     *  the large PdfXObject object can be written to the file
     *  and delete from the memory and you can still draw the image.
     *  This saves memory and increases speed therefore.
     *
     *  \param rRef the PdfImageRef object is initialized with the 
     *         necessary values and can be used for drawing later.
     *
     *  Images and XObjects are handled the same way internally,
     *  therefore you need a PdfImageRef object to draw a PdfXObject.
     */
    void GetImageReference( PdfImageRef & rRef );

 private:
    static PdfArray  s_matrix;
    PdfObject*       m_pResources;

    TSize            m_size;
};

PdfObject* PdfXObject::Contents() const
{
    return (PdfObject*)this;
}


PdfObject* PdfXObject::Resources() const
{
    return m_pResources;
}

inline const TSize & PdfXObject::PageSize() const
{
    return m_size;
}

};

#endif /* _PDF_XOBJECT_H_ */


