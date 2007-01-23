/***************************************************************************
 *   Copyright (C) 2007 by Dominik Seichter                                *
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

#ifndef _PDF_XREF_STREAM_H_
#define _PDF_XREF_STREAM_H_

#include "PdfDefines.h"

#include "PdfXRef.h"

namespace PoDoFo {

class PdfOutputDevice;
class PdfVecObjects;
class PdfWriter;


/**
 * Creates an XRef table that is a stream object.
 * Requires at least PDF 1.5. XRef streams are more
 * compact than normal XRef tables.
 *
 * This is an internal class of PoDoFo used by PdfWriter.
 */
class PdfXRefStream : public PdfXRef {
 public:
    /** Create a new XRef table
     *
     *  \param pParent a vector of PdfObject is required
     *                 to create a PdfObject for the XRef
     *  \param pWriter is needed to fill the trailer directory
     *                 correctly which is included into the XRef
     */
    PdfXRefStream( PdfVecObjects* pParent, PdfWriter* pWriter );

    /** Destruct the XRef table
     */
    virtual ~PdfXRefStream();

    /** Write the XRef table to an output device.
     * 
     *  \param pDevice an output device (usually a PDF file)
     *
     */
    virtual void Write( PdfOutputDevice* pDevice );

 private:
    PdfVecObjects* m_pParent;
    PdfWriter*     m_pWriter;
};

};

#endif /* _PDF_XREF_H_ */
