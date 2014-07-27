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

#include "PdfSigIncSignatureField.h"
#include <string.h>
#include "PdfPage.h"

namespace PoDoFo {

PdfSigIncSignatureField::PdfSigIncSignatureField(PdfDocument *pDocument)
{
   m_SignPage = -1;

   m_pImageData = NULL;
   m_ImageLen= 0;
   m_SignReason = PdfString("I agree");
   m_FontSize = 8;
   m_FontName = "Helvetica";
   m_FontIsSymbolic = false;
   m_FontEncoding = PdfEncodingFactory::GlobalIdentityEncodingInstance();
   m_pDocument = pDocument;

   m_Red = 0;
   m_Green = 0;
   m_Blue = 0;
   m_Threshold = -1;

   createFontFunc = NULL;
   createFontUserData = NULL;
}

PdfSigIncSignatureField::~PdfSigIncSignatureField()
{
}

void PdfSigIncSignatureField::SetSignatureReason(const PdfString &text)
{
   m_SignReason = text;
}

void PdfSigIncSignatureField::SetSignatureReason(const wchar_t *text)
{
    m_SignReason.setFromWchar_t(text);
}

void PdfSigIncSignatureField::SetSignatureDate(const PdfDate &sigDate)
{
   m_SignDate = sigDate;
}

bool PdfSigIncSignatureField::HasSignatureText(void)
{
   return m_SignText.GetLength() > 0;
}

bool PdfSigIncSignatureField::HasSignatureImage(void)
{
   return m_ImageFile.GetLength() > 0 || (m_ImageLen > 0 && m_pImageData != NULL);
}

void PdfSigIncSignatureField::SetSignatureText(const wchar_t *text, int page, int x, int y, int width, int height, float fontSize, const char *fontName, bool fontIsSymbolic, const PdfEncoding *fontEncoding)
{
   PdfRect pdfRect(x, y, width, height);
   PdfPage *pPage = m_pDocument->GetPage(page);
   if(pPage) 
   {
      PdfRect size = pPage->GetPageSize();
      double pHeight = size.GetHeight();
      double newY = pHeight - (y + height);
      pdfRect.SetBottom(newY);
   }
   
   m_SignTextRect = pdfRect;
   m_SignPage = page;
   m_SignText.setFromWchar_t(text);
   m_FontIsSymbolic = fontIsSymbolic;
   m_FontEncoding = fontEncoding;
   if(fontSize > 0) 
   {
      m_FontSize = fontSize;
   }
   if (fontName)
   {
      m_FontName = fontName;
   }
}


void PdfSigIncSignatureField::SetSignatureImage(const char *fileName, int page, int x, int y, int width, int height)
{
   PdfRect pdfRect(x, y, width, height);
   PdfPage *pPage = m_pDocument->GetPage(page);
   if(pPage)
   {
      PdfRect size = pPage->GetPageSize();
      double pHeight = size.GetHeight();
      double newY = pHeight - (y + height);
      pdfRect.SetBottom(newY);
   }
   m_SignImageRect = pdfRect;
   m_ImageFile = fileName;
   m_SignPage = page;
}

void PdfSigIncSignatureField::SetSignatureImage(const unsigned char *pData, pdf_long lLen, int page, int x, int y, int width, int height)
{
   PdfRect pdfRect(x, y, width, height);
   PdfPage *pPage = m_pDocument->GetPage(page);
   if(pPage)
   {
      PdfRect size = pPage->GetPageSize();
      double pHeight = size.GetHeight();
      double newY = pHeight - (y + height);
      pdfRect.SetBottom(newY);
   }
   m_SignImageRect = pdfRect;
   m_pImageData = pData;
   m_ImageLen = lLen;
   m_SignPage = page;
}

void PdfSigIncSignatureField::SetImageChromaKeyMask(pdf_int64 r, pdf_int64 g, pdf_int64 b, pdf_int64 threshold)
{
   m_Red = r; m_Green = g;
   m_Blue = b; m_Threshold = threshold;
}

PdfImage *PdfSigIncSignatureField::CreateSignatureImage(PdfDocument *pParent)
{
   PdfImage *pPdfImage = NULL;
#ifdef PODOFO_HAVE_JPEG_LIB
   if(m_ImageFile.GetLength() > 0)
   {
      pPdfImage = new PdfImage(pParent);
      pPdfImage->LoadFromJpeg(m_ImageFile.GetString());
   }
   else if(m_ImageLen > 0 && m_pImageData != NULL)
   {
      pPdfImage = new PdfImage(pParent);
      pPdfImage->LoadFromJpegData((const unsigned char *) m_pImageData, m_ImageLen); 
   }
#endif

   if(pPdfImage != NULL  &&  m_Threshold >= 0)
   {
      pPdfImage->SetImageChromaKeyMask(m_Red, m_Green, m_Blue, m_Threshold);
   }
 
  return pPdfImage;
}

void PdfSigIncSignatureField::FreeSignatureImage(PdfImage *img)
{
   if(img)
   {
      delete img;
   }
}

}
