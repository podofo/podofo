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

#ifndef _PDF_FONT_TRUE_TYPE_H_
#define _PDF_FONT_TRUE_TYPE_H_

#include "PdfDefines.h"
#include "PdfFontSimple.h"

namespace PoDoFo {

/** A PdfFont implementation that can be used
 *  to embedd truetype fonts into a PDF file
 *  or to draw with truetype fonts. 
 *
 *  TrueType fonts are always embedded as suggested in the PDF reference.
 */
class PdfFontTrueType : public PdfFontSimple {
 public:

    /** Create a new TrueType font. 
     *
     *  It will get embedded automatically.
     * 
     *  \param pMetrics pointer to a font metrics object. The font in the PDF
     *         file will match this fontmetrics object. The metrics object is 
     *         deleted along with the font.
     *  \param pEncoding the encoding of this font. The font will not take ownership of this object.
     *  \param pParent parent of the font object
     *  \param bEmbed if true the font will get embedded.
     *  
     */
    PdfFontTrueType( PdfFontMetrics* pMetrics, const PdfEncoding* const pEncoding, 
                     PdfVecObjects* pParent, bool bEmbed );

    /** Create a PdfFont based on an existing PdfObject
     *  \param pMetrics pointer to a font metrics object. The font in the PDF
     *         file will match this fontmetrics object. The metrics object is 
     *         deleted along with the font.
     *  \param pEncoding the encoding of this font. The font will not take ownership of this object.
     *  \param pObject an existing PdfObject
     */
    PdfFontTrueType( PdfFontMetrics* pMetrics, const PdfEncoding* const pEncoding, 
                     PdfObject* pObject );

 private:

    /** Embed the font file directly into the PDF file.
     *
     *  \param pDescriptor font descriptor object
     */
    void EmbedFontFile( PdfObject* pDescriptor );

};

};

#endif // _PDF_FONT_TRUE_TYPE_H_

