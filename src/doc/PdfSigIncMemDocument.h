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

#ifndef _PDF_SIGMEM_DOCUMENT_H_
#define _PDF_SIGMEM_DOCUMENT_H_


#include "podofo/base/PdfDefines.h"
#include "podofo/base/PdfObject.h"

#include "PdfDocument.h"
#include "PdfFontCache.h"
#include "PdfSignOutputDevice.h"
#include "podofo/base/PdfRefCountedInputDevice.h"
#include "podofo/base/PdfRect.h"
#include "podofo/base/PdfCanvas.h"
#include "PdfPage.h"
#include "PdfImage.h"
#include "PdfSigIncSignatureField.h"

namespace PoDoFo {

class PdfMemDocument;

class PODOFO_DOC_API PdfExMemDocument : public PdfMemDocument {

private:
   pdf_long m_XRefOffset;
   bool m_bXRefStream;

protected:
   virtual void InitFromParser( PdfParser* pParser );

public:
    PdfExMemDocument();
    PdfExMemDocument(const char* pszInpFilename);
    PdfExMemDocument(const PdfRefCountedInputDevice &rInputDevice);
    virtual ~PdfExMemDocument();

    pdf_long GetXRefOffset(void) { return m_XRefOffset;}
    bool HasXRefStream(void) {return m_bXRefStream;}
};

class PODOFO_DOC_API PdfSigIncMemDocument : private PdfMemDocument {

 private:
    char* m_InpFilename;
    PdfRefCountedInputDevice m_InpDeviceRef;
    PdfRect m_SignRect;
    PdfExMemDocument *m_Document;
    pdf_int64 m_LastXRefOffset;

    std::vector<PdfPage*> m_PagesRef;

    PdfXObject *m_pImgXObj;
    PdfXObject *m_n2XObj;

    PdfFont *m_pFont;
    PdfSigIncSignatureField *m_pSignField;

    bool m_bLinearized;
       
protected:
   void CreateAnnotation(PdfSignOutputDevice* pDevice, PdfPage* pPage);
      
   PdfAcroForm* GetExistedAcroForm(PdfAcroForm *pOldAcroForm);
   void AddSignImage(PdfImage &pdfImage, const wchar_t *signatureText, PdfPage *pPage, PdfRect &pdfRect);
   bool AddPageToIncDocument(PdfPage *pPage);
   //void CreateSignObject(int page, PdfRect &pdfRect);

   void CreateVisualSignRect(void);
   void AddVisualSign(PdfPage *pPage);

 public:
    PdfSigIncMemDocument();
    PdfSigIncMemDocument(const char* pszInpFilename);
    virtual ~PdfSigIncMemDocument();

    void Initialize();
    void Load(const PdfRefCountedInputDevice &rInputDevice);
    int GetPageCount(void);
    PdfPage *GetPage(int page);
    PdfMemDocument *GetMainPdfDocument(void) {return m_Document;}
    PdfSigIncSignatureField *GetSignatureField(void) {return m_pSignField;}
           
    void Write( PdfSignOutputDevice* pDevice );
    void CreateVisualSign(void);
    //void CreateSignImage(const char* pszInpFilename, int page, PdfRect &pdfRect);
   // void CreateSignText(const char* signText, int page, PdfRect &pdfRect);
};


};
#endif	// _PDF_SIGMEM_DOCUMENT_H_
