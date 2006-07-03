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

#ifndef _PDF_CANVAS_H_
#define _PDF_CANVAS_H_

#include "PdfDefines.h"

#include "PdfArray.h"

namespace PoDoFo {

class PdfDictionary;
class PdfObject;

/** A interface that provides the necessary features 
 *  for a painter to draw onto a PdfObject.
 */
class PdfCanvas {
 public:
    /** Get access to the contents object of this page.
     *  If you want to draw onto the page, you have to add 
     *  drawing commands to the stream of the Contents object.
     *  \returns a contents object
     */
    virtual PdfObject* Contents() const = 0;

    /** Get access to the resources object of this page.
     *  This is most likely an internal object.
     *  \returns a resources object
     */
    virtual PdfObject* Resources() const = 0;

    /** Get the current page size in 1/1000th mm.
     *  \returns TSize the page size
     */
    virtual const TSize & PageSize() const = 0;

    /** Get a reference to a static procset PdfArray.
     *  \returns a reference to a static procset PdfArray
     */
    static const PdfArray & ProcSet();

 private:
    /** The procset is the same for all 
     *  PdfCanvas objects
     */
    static PdfArray s_procset;
};

};

#endif /* _PDF_CANVAS_H_ */
