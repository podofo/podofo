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

#if defined(_MSC_VER)  &&  _MSC_VER <= 1200
#pragma warning(disable: 4786)
#endif

#include <algorithm>
#include <deque>
#include <iostream>

#include "PdfMemDocument.h"
#include "PdfSigIncMemDocument.h"
#include "base/PdfSigIncWriter.h"
#include "base/PdfDictionary.h"
#include "base/PdfInputStream.h"
#include "PdfAnnotation.h"
#include "PdfSignatureField.h"
#include "PdfPagesTree.h"
#include "PdfImage.h"
#include "PdfPainter.h"
#include "PdfSigIncPainter.h"
#include "PdfIdentityEncoding.h"
#include "base/PdfInputDevice.h" 

using namespace std;

#define BUFFER_SIZE 4096

#ifdef PODOFO_HAVE_JPEG_LIB
extern "C" {
#  ifndef XMD_H
#    define XMD_H
#  endif
#  ifdef _WIN32		// Collision between win and jpeg-headers
#    undef FAR
#  endif
#  include "jpeglib.h"
}
#endif // PODOFO_HAVE_JPEG_LIB

namespace PoDoFo {

////////////////////////////PdfExMemDocument/////////////////////////////////

PdfExMemDocument::PdfExMemDocument()
   : PdfMemDocument()
{
   m_XRefOffset = 0;
   m_bXRefStream = false;
}

PdfExMemDocument::PdfExMemDocument(const char* pszInpFilename)
   : PdfMemDocument()
{
   m_XRefOffset = 0;
   m_bXRefStream = false;
   Load(pszInpFilename);
}

PdfExMemDocument::PdfExMemDocument(const PdfRefCountedInputDevice &rInputDevice)
{
   m_XRefOffset = 0;
   m_bXRefStream = false;
   Load(rInputDevice);
}

PdfExMemDocument::~PdfExMemDocument()
{
}

void PdfExMemDocument::InitFromParser( PdfParser* pParser )
{
   m_XRefOffset = pParser->GetXRefOffset();
   PdfMemDocument::InitFromParser(pParser);
   m_bXRefStream = pParser->HasXRefStream();
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////PdfSigIncMemDocument/////////////////////////////////

PdfSigIncMemDocument::PdfSigIncMemDocument()
   : PdfMemDocument(true)
{
   m_InpFilename = NULL;

   m_Document = new PdfExMemDocument();
   m_pSignField = new PdfSigIncSignatureField(m_Document);
   m_pImgXObj = NULL;
   m_n2XObj = NULL;
   m_pFont = NULL;
   m_bLinearized = false;
}

PdfSigIncMemDocument::PdfSigIncMemDocument(const char* pszInpFilename)
   : PdfMemDocument(true)
{
   m_InpFilename = strdup(pszInpFilename);

   m_Document = new PdfExMemDocument(pszInpFilename);
   m_pSignField = new PdfSigIncSignatureField(m_Document);
   m_pImgXObj = NULL;
   m_n2XObj = NULL;
   m_pFont = NULL;
   m_bLinearized = false;

   m_Document->Load(pszInpFilename);
}

PdfSigIncMemDocument::~PdfSigIncMemDocument()
{
   if (m_Document && m_Document->IsLoaded()) {
      PdfAcroForm *pOldAcroForm = m_Document->GetAcroForm(false, ePdfAcroFormDefaultAppearance_None);
      if(pOldAcroForm == m_pAcroForms)
         m_pAcroForms = NULL;
   }

   if(m_InpFilename)
      podofo_free( m_InpFilename );
   if(m_pSignField)
      delete m_pSignField;
   if(m_Document)
      delete m_Document;  
   if(m_pImgXObj)
      delete m_pImgXObj;
   if(m_n2XObj)
      delete m_n2XObj;
}

int PdfSigIncMemDocument::GetPageCount(void) 
{
   return m_Document->GetPageCount();
}

PdfPage *PdfSigIncMemDocument::GetPage(int page) 
{
   return m_Document->GetPage(page);
}

void PdfSigIncMemDocument::Initialize() 
{
   //TODO - linearizovany pdf a pdf s XRefStream se preulozi 
   // tj. byli-li podepsany (muzou byt linearizovany soubory a soubory s XRefStream podepsany?)
   //-> zrusi se platnost podpisu
   if(m_Document->IsLinearized() || m_Document->HasXRefStream()) {
      m_bLinearized =  true;
      PdfRefCountedBuffer outBuff;
      PdfOutputDevice outDvc(&outBuff);
      m_Document->Write(&outDvc);
      PdfRefCountedInputDevice inpDvc(outBuff.GetBuffer(), outBuff.GetSize());
      delete m_Document;
      m_Document = new PdfExMemDocument(inpDvc);
   }
   CreateVisualSignRect();
   m_LastXRefOffset = m_Document->GetXRefOffset();
   
   PdfAcroForm *pOldAcroForm = m_Document->GetAcroForm(false, ePdfAcroFormDefaultAppearance_None);
   if(pOldAcroForm) {
      PdfObject *pCatalog = m_Document->GetCatalog();
      if(pCatalog) {
         PdfReference pdfRef(pCatalog->Reference());
         pdf_objnum objnum = pdfRef.ObjectNumber();
         objnum = objnum - 1;
         pdfRef.SetObjectNumber(objnum);
         this->GetObjects().SetObjectCount(pdfRef);
         
         m_pCatalog = new PdfObject(*pCatalog);
         this->GetObjects().push_back(m_pCatalog);
      } else
         m_pCatalog = this->GetObjects().CreateObject( "Catalog" );
            
      PdfAcroForm *pAcroForm = this->GetExistedAcroForm(pOldAcroForm); 
            
      PdfObject *pNewFields = pAcroForm->GetObject()->GetDictionary().GetKey( PdfName("Fields") );
      if(!pNewFields) {
         pAcroForm->GetObject()->GetDictionary().AddKey("Fields", PdfArray());
      }
      
      PdfObject* pOldFields = m_Document->GetAcroForm()->GetObject()->GetDictionary().GetKey( PdfName("Fields") );
      if( pOldFields ) {
         PdfObject* pFields = pAcroForm->GetObject()->GetDictionary().GetKey( PdfName("Fields") );
         if(pFields) {
            if(pOldFields->GetDataType() == ePdfDataType_Array )
               pFields->GetArray().insert(pFields->GetArray().begin(), pOldFields->GetArray().begin(), pOldFields->GetArray().end());
            else if(pOldFields->GetDataType() == ePdfDataType_Reference ) {
               PdfObject *pExFldObj = m_Document->GetObjects().GetObject(pOldFields->GetReference());
               if(pExFldObj) {
                  if(pOldFields == pFields) {
                     pFields = new PdfObject(*pExFldObj);
                     PdfReference pdfRef(pExFldObj->Reference());
                     this->GetObjects().SetObjectCount(pdfRef);
                     this->GetObjects().push_back(pFields);
                  } else
                     pFields->GetArray().insert(pFields->GetArray().begin(), pExFldObj->GetArray().begin(), pExFldObj->GetArray().end());
               }
            } else
               PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
         }
      }

      PdfObject* pagesRootObj = m_Document->GetCatalog()->GetIndirectKey( PdfName( "Pages" ) );
      if ( pagesRootObj ) {
         if(!m_pCatalog->GetDictionary().HasKey("Pages"))
            m_pCatalog->GetDictionary().AddKey( "Pages", pagesRootObj->Reference() );
      }

      PdfReference pdfRef;
      pdfRef.SetObjectNumber(m_Document->GetObjects().GetObjectCount() - 1);
      this->GetObjects().SetObjectCount(pdfRef);
    } else {
       PdfObject *pCatalog = m_Document->GetCatalog();
       if(pCatalog) {
          PdfReference pdfRef(pCatalog->Reference());
          pdf_objnum objnum = pdfRef.ObjectNumber();
          objnum = objnum - 1;
          pdfRef.SetObjectNumber(objnum);
          this->GetObjects().SetObjectCount(pdfRef);
         
          m_pCatalog = new PdfObject(*pCatalog);
          this->GetObjects().push_back(m_pCatalog);
       } else
          m_pCatalog = this->GetObjects().CreateObject( "Catalog" );
        
       PdfReference pdfRef;
       pdfRef.SetObjectNumber(m_Document->GetObjects().GetObjectCount() - 1);
       this->GetObjects().SetObjectCount(pdfRef);
       this->GetAcroForm(true, ePdfAcroFormDefaultAppearance_None);
    
       PdfObject* pagesRootObj = m_Document->GetCatalog()->GetIndirectKey( PdfName( "Pages" ) );
       if ( pagesRootObj ) {
           m_pCatalog->GetDictionary().AddKey( "Pages", pagesRootObj->Reference() );
       }
    }
   
    m_pInfo = new PdfInfo( &this->GetObjects() );

    m_pTrailer = new PdfObject(); // The trailer is NO part of the vector of objects
    m_pTrailer->SetOwner( &this->GetObjects());

    m_pTrailer->GetDictionary().AddKey( "Root", m_pCatalog->Reference() );
    m_pTrailer->GetDictionary().AddKey( "Info", m_pInfo->GetObject()->Reference() );
}

void PdfSigIncMemDocument::Load(const PdfRefCountedInputDevice &rInputDevice)
{
   m_InpDeviceRef = rInputDevice;
   m_Document->Load(m_InpDeviceRef);
}

PdfAcroForm* PdfSigIncMemDocument::GetExistedAcroForm(PdfAcroForm *pOldAcroForm)
{
    if( !m_pAcroForms )  {
       PdfReference pdfRef = pOldAcroForm->GetObject()->Reference();
       if(pdfRef.IsIndirect()) {
            PdfObject* pOldFields = pOldAcroForm->GetObject()->GetDictionary().GetKey( PdfName("Fields") );
            if(pOldFields) {
               if(pOldFields->GetDataType() == ePdfDataType_Array ) {
                  pdfRef.SetObjectNumber(pOldAcroForm->GetObject()->Reference().ObjectNumber() - 1);
                  this->GetObjects().SetObjectCount(pdfRef);
                  PdfObject *pNewObj = this->GetObjects().CreateObject();
                  m_pAcroForms = new PdfAcroForm( this, pNewObj, ePdfAcroFormDefaultAppearance_None );
                  this->GetCatalog()->GetDictionary().AddKey( "AcroForm", m_pAcroForms->GetObject()->Reference() );   
               } else if(pOldFields->GetDataType() == ePdfDataType_Reference ) {
                  m_pAcroForms = pOldAcroForm;
               }
            } else {
               pdfRef.SetObjectNumber(pOldAcroForm->GetObject()->Reference().ObjectNumber() - 1);
               this->GetObjects().SetObjectCount(pdfRef);
               PdfObject *pNewObj = this->GetObjects().CreateObject();
               m_pAcroForms = new PdfAcroForm( this, pNewObj, ePdfAcroFormDefaultAppearance_None );
               this->GetCatalog()->GetDictionary().AddKey( "AcroForm", m_pAcroForms->GetObject()->Reference() );
            }
       } else {
          m_pAcroForms = pOldAcroForm;
       }
    }        
    
    return m_pAcroForms;
}


void PdfSigIncMemDocument::Write( PdfSignOutputDevice* pDevice ) 
{
    if(m_bLinearized) {
       m_Document->Write(pDevice);
    } else {
       char *pBuffer = new char[BUFFER_SIZE];
       pdf_long read = 0;
       pdf_long tmpRead = 0;
       if (m_InpFilename) {
          PdfFileInputStream inputStream(m_InpFilename);
          do {
             tmpRead = inputStream.Read(pBuffer, BUFFER_SIZE);
             if(tmpRead != -1) {
               pDevice->Write(pBuffer, tmpRead);
               read += tmpRead;
             }
          } while(read < inputStream.GetFileLength() && tmpRead != -1);
       } else {
          PdfInputDevice *inputDevice = m_InpDeviceRef.Device();
          PODOFO_RAISE_LOGIC_IF( !inputDevice, "No input device set." );

          inputDevice->Seek(0);
          do {
             tmpRead = inputDevice->Read(pBuffer, BUFFER_SIZE);
             if(tmpRead > 0) {
               pDevice->Write(pBuffer, tmpRead);
               read += tmpRead;
             }
          } while(!inputDevice->Eof() && tmpRead > 0);
       }
       delete [] pBuffer;
    }
     

    //TODO - co kdyz nema zadnou page?
    PdfPage *pPage = m_Document->GetPage(m_Document->GetPageCount() - 1);
    CreateAnnotation(pDevice, pPage);

    const PdfObject *pTrailer = m_Document->GetTrailer();

    if( pTrailer->GetDictionary().HasKey( "Root" ) ) {
       GetMainTrailer()->GetDictionary().AddKey( "Root", pTrailer->GetDictionary().GetKey( "Root" ) );
    }
     
    if( pTrailer->GetDictionary().HasKey( "Info" ) )
      GetMainTrailer()->GetDictionary().AddKey( "Info", pTrailer->GetDictionary().GetKey( "Info" ) );
     
    if( pTrailer->GetDictionary().HasKey( "ID" ) )
      GetMainTrailer()->GetDictionary().AddKey( "ID", pTrailer->GetDictionary().GetKey( "ID" ) );
       

    PdfSigIncWriter writer( &(this->GetObjects()), this->GetTrailer());
    writer.SetPdfVersion( this->GetPdfVersion() );
    writer.SetWriteMode( ePdfWriteMode_Compact);
                
    writer.Write( pDevice, m_LastXRefOffset);    

    vector<PdfPage*>::iterator it, end = m_PagesRef.end();
    for(it = m_PagesRef.begin(); it != end; it++) {
       m_Document->GetObjects().RemoveObject((*it)->GetObject()->Reference());
    }
}

void PdfSigIncMemDocument::CreateAnnotation(PdfSignOutputDevice* pDevice, PdfPage* pPage )
{
   PdfAnnotation* pAnnot = new PdfAnnotation(pPage, ePdfAnnotation_Widget, m_SignRect, &this->GetObjects());
   pdf_int64 flags = 132;
   pAnnot->GetObject()->GetDictionary().AddKey("F", PdfObject(flags));
   PdfAcroForm *pAcroForm = this->GetAcroForm(); 

   if(pAcroForm) {
      if(!pAcroForm->GetObject()->GetDictionary().HasKey(PdfName("SigFlags"))) {
         pdf_int64 val = 3;
         pAcroForm->GetObject()->GetDictionary().AddKey("SigFlags", PdfObject(val));
      }

      PdfSignatureField signField( pAnnot, pAcroForm, this);

      //TODO - musi byt unikatni
      char fldName[40]; // 31 bytes would be enough, use bigger buffer to make sure sprintf does never overflow
      sprintf(fldName, "SignatureFieldName %d", pAnnot->GetObject()->Reference().ObjectNumber());
      PdfString name(fldName);
      signField.SetFieldName(name);
      signField.SetSignatureReason(m_pSignField->GetSignatuReason());
      signField.SetSignatureDate( m_pSignField->GetSignatureDate() );
      signField.SetSignature(*pDevice->GetSignatureBeacon());

      if(m_pImgXObj != NULL) {
         signField.SetAppearanceStream(m_pImgXObj);
      }
      if(m_PagesRef.size() > 0) {
         PdfPage *pPage = m_PagesRef[0];

         PdfObject *pAnnot = pPage->GetOwnAnnotationsArray(false, m_Document);
         if(!pAnnot)
            pAnnot = pPage->GetOwnAnnotationsArray(true, m_Document); 
         else {
            if (pPage->GetObject()->GetDictionary().HasKey( "Annots" ) )  {
               PdfObject *pTmpObj = pPage->GetObject()->GetDictionary().GetKey( "Annots" );
               if(pTmpObj->IsReference()) {
                  PdfObject *newAnnot = new PdfObject(*pAnnot);
                  this->GetObjects().push_back(newAnnot);
                  pAnnot = newAnnot;
               }
            }
         }
         if(pAnnot) {
            pAnnot->GetArray().push_back(signField.GetFieldObject()->Reference());
         }
      }
   } else
      PODOFO_RAISE_ERROR(ePdfError_InternalLogic);
}

void PdfSigIncMemDocument::CreateVisualSignRect(void)  
{
   PdfRect rect(0, 0, 50, 50);
   double dTRight = 0;
   double dTTop = 0;
   if(m_pSignField->HasSignatureText()) {
      PdfRect textRect = m_pSignField->GetTextRect();
      rect.SetBottom(textRect.GetBottom());
      rect.SetLeft(textRect.GetLeft());
      rect.SetHeight(textRect.GetHeight());
      rect.SetWidth(textRect.GetWidth());
      dTTop = textRect.GetBottom() + textRect.GetHeight();
      dTRight = textRect.GetLeft() + textRect.GetWidth();
   }
   if(m_pSignField->HasSignatureImage()) {
      PdfRect imgRect = m_pSignField->GetImageRect();
      if(!m_pSignField->HasSignatureText()) {
         rect.SetBottom(imgRect.GetBottom());
         rect.SetLeft(imgRect.GetLeft());
      } else {
         if(imgRect.GetLeft() < rect.GetLeft())
            rect.SetLeft(imgRect.GetLeft());
         if(imgRect.GetBottom() < rect.GetBottom())
            rect.SetBottom(imgRect.GetBottom());
      }

      double dITop = imgRect.GetBottom() + imgRect.GetHeight();
      double dIRight = imgRect.GetLeft() + imgRect.GetWidth();

      double dTMax = dITop > dTTop ? dITop : dTTop;
      double dRMax = dIRight > dTRight ? dIRight : dTRight;
   
      rect.SetHeight(dTMax - rect.GetBottom());
      rect.SetWidth(dRMax - rect.GetLeft());
   }
   m_SignRect = rect;
}

bool PdfSigIncMemDocument::AddPageToIncDocument(PdfPage *pPage)
{
   bool bResult = true;
   
   if(m_Document->IsLinearized())
      return bResult;

   PdfObject *objPage = pPage->GetObject();
   if(objPage != NULL) {
      PdfObject *obj = this->GetObjects().GetObject(objPage->Reference());
      if(!obj) {
         this->GetObjects().push_back(objPage);
         this->GetObjects().Sort();
      }
   } else
      bResult = false;
   return bResult;
}

void PdfSigIncMemDocument::AddVisualSign(PdfPage *pPage)
{
   if(AddPageToIncDocument(pPage)) {
      PdfRect objRect(0,0, m_SignRect.GetWidth(), m_SignRect.GetHeight());

      bool bLinear =  m_Document->IsLinearized();

      PdfDocument *pDocument = bLinear ? (PdfDocument*) m_Document : this;
      m_pImgXObj = new PdfXObject(objRect, pDocument);

      PdfSigIncPainter pnt(pDocument, bLinear);

      try {
          pnt.SetPageCanvas(pPage, m_pImgXObj->GetContents());

          PdfXObject frmXObj(objRect, pDocument, "FRM", true);

          m_pImgXObj->AddResource(PdfName("FRM"), frmXObj.GetObjectReference(), PdfName("XObject"));
          pnt.DrawXObject(0,0, &frmXObj);
          pnt.EndCanvas();

          pnt.SetPageCanvas(pPage, frmXObj.GetContents());

          PdfXObject n0XObj(objRect, pDocument, "n0", true);
          PdfXObject n2XObj(objRect, pDocument, "n2", true);

          frmXObj.AddResource(PdfName("n0"), n0XObj.GetObjectReference(), PdfName("XObject"));
          frmXObj.AddResource(PdfName("n2"), n2XObj.GetObjectReference(), PdfName("XObject"));

          pnt.DrawXObject(0,0, &n0XObj);
          pnt.DrawXObject(0,0, &n2XObj);
          pnt.EndCanvas();

          PdfImage *pdfImage = NULL;
          if(m_pSignField->HasSignatureImage()) {
             pdfImage = m_pSignField->CreateSignatureImage(pDocument);
          }
          if(m_pSignField->HasSignatureText() || pdfImage != NULL) {
             pnt.SetPageCanvas(pPage, n2XObj.GetContents());
          }
          if(pdfImage) {
             PdfRect imgRect = m_pSignField->GetImageRect();
             n2XObj.AddResource(pdfImage->GetIdentifier(), pdfImage->GetObjectReference(), PdfName("XObject"));

             double scaleX = imgRect.GetWidth() / pdfImage->GetWidth();
             double scaleY = imgRect.GetHeight() / pdfImage->GetHeight();

             pnt.DrawImage(imgRect.GetLeft() - m_SignRect.GetLeft(), imgRect.GetBottom() - m_SignRect.GetBottom(),
                            pdfImage, scaleX, scaleY);
             m_pSignField->FreeSignatureImage(pdfImage);
          }

          if(m_pSignField->HasSignatureText()) {
             if(m_pFont == NULL) {
                if (m_pSignField->createFontFunc) {
                    m_pFont = m_pSignField->createFontFunc (pDocument, m_pSignField->createFontUserData);
                }
                if (!m_pFont) {
                    m_pFont = pDocument->CreateFont(m_pSignField->GetFontName(), m_pSignField->GetFontIsSymbolic(), m_pSignField->GetFontEncoding());
                }
                m_pFont->SetFontSize(m_pSignField->GetFontSize());
             }
             pnt.SetFont(m_pFont);

             n2XObj.AddResource(m_pFont->GetIdentifier(), m_pFont->GetObject()->Reference(), PdfName("Font"));

             PdfRect tRect = m_pSignField->GetTextRect();
             PdfString text = m_pSignField->GetSignatureText();
             PdfRect txtRect(tRect.GetLeft() - m_SignRect.GetLeft(), tRect.GetBottom() - m_SignRect.GetBottom(),
                             tRect.GetWidth(), tRect.GetHeight());
             pnt.DrawMultiLineText(txtRect, text);
             //pnt.DrawText(10,50, text);
          }
      } catch (const PdfError &e) {
          try {
              pnt.FinishPage();
          } catch(...) {
          }
          throw e;
      }

      pnt.FinishPage();
   } else
      PODOFO_RAISE_ERROR(ePdfError_InternalLogic);

}

void PdfSigIncMemDocument::CreateVisualSign(void)
{
   if(m_pSignField->GetPage() < 0)
      return;
   if(m_pSignField->GetPage() >= m_Document->GetPageCount())
      PODOFO_RAISE_ERROR(ePdfError_InternalLogic);

   if(!m_pSignField->HasSignatureImage() && !m_pSignField->HasSignatureText())
      return;

   PdfPage *pPage = m_Document->GetPage(m_pSignField->GetPage());
   if(!m_Document->IsLinearized())
      m_PagesRef.push_back(pPage);
   AddVisualSign(pPage);
}

};

