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

#include "base/PdfObject.h"
#include "base/PdfStream.h"
#include "base/PdfCanvas.h"
#include "PdfDocument.h"

#include "PdfSigIncPainter.h"

using namespace std;

namespace PoDoFo {

PdfSigIncPainter::PdfSigIncPainter(PdfDocument *pDocument, bool bLinear)
: PdfPainter()
{
   m_pDocument = pDocument;
   m_bLinearized = bLinear;
}

PdfSigIncPainter::~PdfSigIncPainter()
{
}

void PdfSigIncPainter::SetPageCanvas( PdfCanvas *pPage, PdfObject* pContents )
{
   if(m_bLinearized) {
      SetPage(pPage);
      return;
   }
   if( m_pPage != pPage ) {
      if( m_pCanvas )
         m_pCanvas->EndAppend();
      m_pPage   = pPage;
   }
 
   EndCanvas();
   m_pCanvas = pContents->GetStream();
   if ( m_pCanvas )  {
      m_pCanvas->BeginAppend( false );
      if ( m_pCanvas->GetLength() )  {    
         m_pCanvas->Append( " " );
      }
   } else {
      PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
   }   
}


void PdfSigIncPainter::EndCanvas()
{
   if(m_bLinearized)
      return;
	try { 
		if( m_pCanvas )
			m_pCanvas->EndAppend();
	} catch( const PdfError & e ) {
	    // clean up, even in case of error
		m_pCanvas = NULL;
		throw e;
	}

    m_pCanvas = NULL;
}

void PdfSigIncPainter::AddToPageResources( const PdfName & rIdentifier, const PdfReference & rRef, const PdfName & rName )
{
   if(m_bLinearized)
      PdfPainter::AddToPageResources(rIdentifier, rRef, rName);
   /*Pro podpis nedavat font do stranky, ale do XObject resources*/
   return;
  
}

void PdfSigIncPainter::DrawMultiLineText( const PdfRect & rRect, const PdfString & rsText, 
                                    EPdfAlignment eAlignment, EPdfVerticalAlignment eVertical)
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );
    double dX = rRect.GetLeft();
    double dY = rRect.GetBottom();
    double dWidth = rRect.GetWidth();
    double dHeight = rRect.GetHeight();

    if( !m_pFont || !m_pPage || !rsText.IsValid() )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }
    m_pFont->EmbedFont();
    
    if( dWidth <= 0.0 || dHeight <= 0.0 ) // nonsense arguments
        return;

    this->Save();
    this->SetClipRect( dX, dY, dWidth, dHeight );

    PdfString   sString  = this->ExpandTabs( rsText, rsText.GetCharacterLength() );

    if(sString.IsUnicode()) {
       vector<TExLineElement<pdf_utf16be> > vecLines = GetMultiLineTextAsLines(sString.GetUnicode(), dWidth);
       DrawMultiLineText(dX, dY, dWidth, dHeight, eAlignment, eVertical, vecLines);
    } else {
       vector<TExLineElement<char> > vecLines = GetMultiLineTextAsLines(sString.GetString(), dWidth);
       DrawMultiLineText(dX, dY, dWidth, dHeight, eAlignment, eVertical, vecLines);
    }
    this->Restore();

}

int PdfSigIncPainter::IsLf(const char *pszChar)
{
   return *pszChar == '\n';
}

int PdfSigIncPainter::IsSpace(const char *pszChar) 
{
   return isspace( static_cast<unsigned int>(static_cast<unsigned char>(*pszChar)));
}

int PdfSigIncPainter::IsLf(const pdf_utf16be *pszChar)
{
   const pdf_utf16be cLf = 0x0a00;

   return *pszChar == cLf;
}

int PdfSigIncPainter::IsSpace(const pdf_utf16be *pszChar) 
{
   const pdf_utf16be cTab    = 0x0900;
   const pdf_utf16be cSpace  = 0x2000;
   const pdf_utf16be cLf     = 0x0a00;
   const pdf_utf16be cVt     = 0x0b00;
   const pdf_utf16be cFeed   = 0x0c00;
   const pdf_utf16be cCr     = 0x0d00;

   if(*pszChar == cTab || *pszChar == cSpace || *pszChar == cLf || *pszChar == cVt || *pszChar == cFeed || *pszChar == cCr)
      return 1;
   return 0;
}

double PdfSigIncPainter::GetFontCharWidth(const pdf_utf16be *pszChar)
{
   if( !m_pFont)  {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
   }
   return m_pFont->GetFontMetrics()->UnicodeCharWidth( *pszChar);
}

double PdfSigIncPainter::GetFontCharWidth(const char *pszChar)
{
   if( !m_pFont)  {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
   }
   return m_pFont->GetFontMetrics()->UnicodeCharWidth(*pszChar);
}

};
