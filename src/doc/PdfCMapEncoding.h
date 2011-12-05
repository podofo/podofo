/***************************************************************************
 *   Copyright (C) 2011 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                                                                         *
 *   Pdf CMAP encoding by kalyan                                           *
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

#ifndef _PDF_CMAP_ENCODING_H
#define _PDF_CMAP_ENCODING_H

#include "podofo/base/PdfDefines.h"
#include "podofo/base/PdfEncoding.h"
#include "PdfElement.h"


namespace PoDoFo {

class PODOFO_DOC_API PdfCMapEncoding: public PdfEncoding, private PdfElement{
public:
    enum EBaseEncoding {
        eBaseEncoding_Font,      ///< Use The fonts encoding as base
        eBaseEncoding_WinAnsi,   ///< Use WinAnsiEncoding as base encoding
        eBaseEncoding_MacRoman,  ///< Use MacRomanEncoding as base encoding
        eBaseEncoding_MacExpert  ///< Use MacExpertEncoding as base encoding
    };


    PdfCMapEncoding(PdfObject* pObject);
    virtual PdfString ConvertToUnicode(const PdfString& rEncodedString, const PdfFont* pFont) const;
    virtual void AddToDictionary(PdfDictionary & rDictionary ) const;
    virtual PdfRefCountedBuffer ConvertToEncoding(const PdfString& rString, const PdfFont* pFont) const; 
    virtual bool IsAutoDelete() const;
    virtual bool IsSingleByteEncoding() const;
    virtual pdf_utf16be GetCharCode(int nIndex) const;
    virtual const PdfName & GetID() const;
    const PdfEncoding* GetBaseEncoding() const;
private:

    pdf_utf16be *cMapEncoding;
    EBaseEncoding m_baseEncoding;

};

}; /*PoDoFo namespace end*/

#endif // _PDF_CMAP_ENCODING_H 




