/***************************************************************************
 *   Copyright (C) 2014 by Dominik Seichter                                *
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

#ifndef _PDF_SIGINCSIGNATURE_FIELD_H_
#define _PDF_SIGINCSIGNATURE_FIELD_H_

#include "PdfField.h"
#include "PdfSignatureField.h"
#include "podofo/base/PdfDate.h"
#include "PdfImage.h"
#include "PdfDocument.h"
#include <string>

namespace PoDoFo {

typedef PdfFont * (*PdfSigIncCreateFont)(PdfDocument *document, void *user_data);

class PODOFO_DOC_API PdfSigIncSignatureField 
{
private:
    int m_SignPage;
    float m_FontSize;
    std::string m_FontName;
    const PdfEncoding *m_FontEncoding;
    bool m_FontIsSymbolic;

    PdfString m_SignText;
    PdfRect m_SignTextRect;
    
    const unsigned char* m_pImageData;
    pdf_long m_ImageLen;
    PdfString m_ImageFile;
    PdfRect m_SignImageRect;

    PdfString m_SignReason;
    PdfDate m_SignDate;

    PdfDocument *m_pDocument;

    pdf_int64 m_Red;
    pdf_int64 m_Green;
    pdf_int64 m_Blue;
    pdf_int64 m_Threshold;

private:

public:
    PdfSigIncSignatureField(PdfDocument *pDocument);
    virtual ~PdfSigIncSignatureField();

    void SetSignatureReason(const PdfString &text);
    void SetSignatureReason(const wchar_t *text);
    PdfString &GetSignatuReason(void) {return m_SignReason;}
    void SetSignatureDate(const PdfDate &sigDate);
    PdfDate &GetSignatureDate(void) {return m_SignDate;}
    bool HasSignatureText(void);
    bool HasSignatureImage(void);
    PdfRect &GetTextRect(void) {return m_SignTextRect;}
    PdfRect &GetImageRect(void) {return m_SignImageRect;}
    PdfString &GetSignatureText(void) {return m_SignText;}
    const char *GetFontName(void) { return m_FontName.c_str(); }
    bool GetFontIsSymbolic(void) { return m_FontIsSymbolic; }
    const PdfEncoding *GetFontEncoding(void) { return m_FontEncoding; }
    int GetPage(void) {return m_SignPage;}
    float GetFontSize(void) {return m_FontSize;}

    PdfSigIncCreateFont createFontFunc;
    void *createFontUserData;

    PdfImage *CreateSignatureImage(PdfDocument *pParent);
    void FreeSignatureImage(PdfImage *img);

    void SetSignatureText(const wchar_t *text, int page, int x, int y, int width, int height, float fontSize, const char *fontName, bool fontIsSymbolic, const PdfEncoding *fontEncoding);
    void SetSignatureImage(const char *fileName, int page, int x, int y, int width, int height);
    void SetSignatureImage(const unsigned char *pData, pdf_long lLen, int page, int x, int y, int width, int height);
    void SetImageChromaKeyMask(pdf_int64 r, pdf_int64 g, pdf_int64 b, pdf_int64 threshold);
    
};

}

#endif
