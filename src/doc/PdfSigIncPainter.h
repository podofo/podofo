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

#ifndef _PDF_SIGINCPAINTER_H_
#define _PDF_SIGINCPAINTER_H_

#include "podofo/base/PdfDefines.h"

#include "PdfPainter.h"

namespace PoDoFo {
   
template <class T> struct TExLineElement 
{
	TExLineElement()
		: pszStart( NULL ), lLen( 0L )
	{
	}

	const T* pszStart;
	pdf_long        lLen;
};


class PdfPainter;

class PODOFO_DOC_API PdfSigIncPainter : public PdfPainter {

public:
    PdfSigIncPainter(PdfDocument *pDocument, bool bLinear);

    virtual ~PdfSigIncPainter();

    void SetPageCanvas(PdfCanvas *pPage, PdfObject* pContents );
    void EndCanvas();

    void DrawMultiLineText( const PdfRect & rRect, const PdfString & rsText, 
                            EPdfAlignment eAlignment = ePdfAlignment_Left, EPdfVerticalAlignment eVertical=ePdfVerticalAlignment_Top);

    
protected:
    int IsSpace(const char *pszChar);
    int IsLf(const char *pszChar);
    int IsSpace(const pdf_utf16be *pszChar);
    int IsLf(const pdf_utf16be *pszChar);
    double GetFontCharWidth(const char *pszChar);
    double GetFontCharWidth(const pdf_utf16be *pszChar);

    virtual void AddToPageResources( const PdfName & rIdentifier, const PdfReference & rRef, const PdfName & rName );
    

   template <class T> void DrawMultiLineText(double dX, double dY, double dWidth, double dHeight, 
                                                               EPdfAlignment eAlignment, EPdfVerticalAlignment eVertical,
                                                               const std::vector< TExLineElement<T> > &vecLines)
   {
      // Do vertical alignment
      switch( eVertical ) 
      {
         default:
         case ePdfVerticalAlignment_Top:
            dY += dHeight; break;
         case ePdfVerticalAlignment_Bottom:
            dY += m_pFont->GetFontMetrics()->GetLineSpacing() * vecLines.size(); break;
         case ePdfVerticalAlignment_Center:
            dY += (dHeight - 
                     ((dHeight - (m_pFont->GetFontMetrics()->GetLineSpacing() * vecLines.size()))/2.0)); 
            break;
      }

      typedef typename std::vector< TExLineElement<T> >::const_iterator TExLineElementVectorIterator;
      TExLineElementVectorIterator it = vecLines.begin();
      while( it != vecLines.end() ) {
         dY -= m_pFont->GetFontMetrics()->GetLineSpacing();
         if( (*it).pszStart )
            this->DrawTextAligned( dX, dY, dWidth, PdfString( (*it).pszStart, (*it).lLen ), eAlignment );

         ++it;
      }
   }

   template <class T> std::vector<TExLineElement<T> > GetMultiLineTextAsLines(const T *mainText, double dWidth)
   {
       PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );
   
       if( !m_pFont || !m_pPage || mainText == NULL ) {
           PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
       }

       std::vector<TExLineElement<T> > vecLines;

       if( dWidth <= 0.0 ) // nonsense arguments
           return vecLines;

       TExLineElement<T> tLine;
    
       tLine.pszStart = mainText;
    
       const T* pszCurrentCharacter   = tLine.pszStart;
       const T* pszStartOfCurrentWord  = tLine.pszStart;

       bool startOfWord = true;

       double dCurWidthOfLine = 0.0;

       // do simple word wrapping
       while( *pszCurrentCharacter ) 
       {
           if( IsLf(pszCurrentCharacter) ) // hard-break!
           {
               tLine.lLen = pszCurrentCharacter - tLine.pszStart;
               vecLines.push_back( tLine );

               ++pszCurrentCharacter; // skip the line feed

               tLine.pszStart = pszCurrentCharacter;
               startOfWord = true;
               dCurWidthOfLine = 0.0;
           }
           else if(IsSpace(pszCurrentCharacter))
           {
               if( dCurWidthOfLine > dWidth )
               {
                   // The previous word does not fit in the current line.
                   // -> Move it to the next one.
                   tLine.lLen = pszStartOfCurrentWord - tLine.pszStart;
                   vecLines.push_back( tLine );

                   tLine.pszStart = pszStartOfCurrentWord;

                   if (!startOfWord)
                   {
                      dCurWidthOfLine = m_pFont->GetFontMetrics()->StringWidth( pszStartOfCurrentWord, pszCurrentCharacter-pszStartOfCurrentWord );
                   }
                   else
                   {
                       dCurWidthOfLine = 0.0;
                   }
               }
               else 
               {
           
                   dCurWidthOfLine += GetFontCharWidth(pszCurrentCharacter );
               }

               startOfWord = true;
           }
           else
           {
               if (startOfWord)
               {
                   pszStartOfCurrentWord = pszCurrentCharacter;
                   startOfWord = false;
               }
               //else do nothing

               if ((dCurWidthOfLine + GetFontCharWidth(pszCurrentCharacter )) > dWidth)
               {
                   if ( tLine.pszStart == pszStartOfCurrentWord )
                   {
                       // This word takes up the whole line.
                       // Put as much as possible on this line.
                       tLine.lLen = pszCurrentCharacter - tLine.pszStart;
                       vecLines.push_back( tLine );

                       tLine.pszStart = pszCurrentCharacter;
                       pszStartOfCurrentWord = pszCurrentCharacter;

                       dCurWidthOfLine = GetFontCharWidth(pszCurrentCharacter );
                   }
                   else
                   {
                       // The current word does not fit in the current line.
                       // -> Move it to the next one.
                       tLine.lLen = pszStartOfCurrentWord - tLine.pszStart;
                       vecLines.push_back( tLine );

                       tLine.pszStart = pszStartOfCurrentWord;

                       dCurWidthOfLine = m_pFont->GetFontMetrics()->StringWidth( pszStartOfCurrentWord, (pszCurrentCharacter-pszStartOfCurrentWord) + 1 );
                   }
               }
               else 
               {
                  dCurWidthOfLine +=GetFontCharWidth(pszCurrentCharacter );
               }
           }
           ++pszCurrentCharacter;
       }

       if( pszCurrentCharacter-tLine.pszStart > 0 ) 
       {
           if( dCurWidthOfLine > dWidth )
           {
               // The previous word does not fit in the current line.
               // -> Move it to the next one.
               tLine.lLen = pszStartOfCurrentWord - tLine.pszStart;
               vecLines.push_back( tLine );

               tLine.pszStart = pszStartOfCurrentWord;

           }
           //else do nothing

           if( pszCurrentCharacter-tLine.pszStart > 0 ) 
           {
               tLine.lLen = pszCurrentCharacter - tLine.pszStart;
               vecLines.push_back( tLine );
           }
           //else do nothing
       }

       return vecLines;
   }

private:
    PdfDocument *m_pDocument;
    bool m_bLinearized;

 
};


};
#endif
