/***************************************************************************
 *   Copyright (C) 2010 by Dominik Seichter                                *
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

#ifndef _PDF_FONT_TYPE1_BASE14_H_
#define _PDF_FONT_TYPE1_BASE14_H_

#include "podofo/base/PdfDefines.h"
#include "PdfFontSimple.h"

namespace PoDoFo {

/** A PdfFont implementation that can be used
 *  draw with base14 type1 fonts into a PDF file. 
 */
class PdfFontType1Base14 : public PdfFontSimple {
 public:
    /** Create a new Type1 font object.
     *
     *  \param pMetrics pointer to a font metrics object. The font in the PDF
     *         file will match this fontmetrics object. The metrics object is 
     *         deleted along with the font.
     *  \param pEncoding the encoding of this font. The font will take ownership of this object
     *                   depending on pEncoding->IsAutoDelete()
     *  \param pParent parent of the font object
     *  
     */
    PdfFontType1Base14( PdfFontMetrics* pMetrics, const PdfEncoding* const pEncoding, 
                  PdfVecObjects* pParent );

    // OC 13.08.2010 New:
    /** Create a new Type1 font object based on an existing PdfObject
     *  \param pMetrics pointer to a font metrics object. The font in the PDF
     *         file will match this fontmetrics object. The metrics object is 
     *         deleted along with the font.
     *  \param pEncoding the encoding of this font. The font will take ownership of this object
     *                   depending on pEncoding->IsAutoDelete()
     *  \param pObject an existing PdfObject
     */
    PdfFontType1Base14( PdfFontMetrics* pMetrics, const PdfEncoding* const pEncoding, 
                  PdfObject* pObject );

    ~PdfFontType1Base14();

 protected:
    /** Embed the font file directly into the PDF file.
     *
     *  \param pDescriptor font descriptor object
     */
    virtual void EmbedFontFile( PdfObject* pDescriptor );

 private:
    void InitBase14Font( PdfFontMetrics* pMetrics );

};

};

#endif // _PDF_FONT_TYPE1_BASE14_H_

