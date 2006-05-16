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
#include "PdfCanvas.h"
#include "PdfObject.h"

namespace PoDoFo {

class PdfWriter;

/** PdfPage is one page in the pdf document. 
 *  It is possible to draw on a page using a PdfPainter object.
 *  Every document needs at least one page.
 */
class PdfPage : public PdfObject, public PdfCanvas {
 public:
    /** Create a new PdfPage object.
     *  \param tSize a size structure spezifying the size of the page (i.e the /MediaBox key) in 1/1000th mm
     *  \param pWriter pointer to the parent PdfWriter object
     *  \param nObjectNo object no
     *  \param nGenerationNo generation number of the object
     */
    PdfPage( const TSize & tSize, PdfWriter* pWriter, unsigned int nObjectNo, unsigned int nGenerationNo );
    ~PdfPage();

    /** Initialize a newly generated PdfPage object. If you use the CreatePage method of PdfSimpleWriter
     *  you do not have to call this method.
     *  \returns ErrOk on success
     */
    PdfError Init();

    /** Creates a TSize page size object which is needed to create a PdfPage object
     *  from an enum which are defined for a few standard page sizes.
     *  \param ePageSize the page size you want
     *  \returns TSize object which can be passed to the PdfPage constructor
     */
    static TSize CreateStadardPageSize( const EPdfPageSize ePageSize );

    /** Get access to the contents object of this page.
     *  If you want to draw onto the page, you have to add 
     *  drawing commands to the stream of the Contents object.
     *  \returns a contents object
     */
    virtual inline PdfObject* Contents() const;

    /** Get access to the resources object of this page.
     *  This is most likely an internal object.
     *  \returns a resources object
     */
    virtual inline PdfObject* Resources() const;

    /** Get the current page size in 1/1000th mm.
     *  \returns TSize the page size
     */
    virtual inline const TSize & PageSize() const;

 private:
    PdfWriter* m_pWriter;
    PdfObject* m_pResources;
    PdfObject* m_pContents;

    TSize      m_tPageSize;
    
};

PdfObject* PdfPage::Contents() const
{
    return m_pContents;
}

PdfObject* PdfPage::Resources() const
{
    return m_pResources;
}

const TSize & PdfPage::PageSize() const
{
    return m_tPageSize;
}

};

#endif // _PDF_PAGE_H_
