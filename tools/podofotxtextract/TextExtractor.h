/***************************************************************************
 *   Copyright (C) 2008 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef _TEXT_EXTRACTOR_H_
#define _TEXT_EXTRACTOR_H_

#include <podofo.h>
#include <cstdio>

using namespace PoDoFo;

#ifndef MAX_PATH
#define MAX_PATH 512
#endif // MAX_PATH

/** This class uses the PoDoFo lib to parse 
 *  a PDF file and to write all text it finds
 *  in this PDF document to stdout.
 */
class TextExtractor {
 public:
    TextExtractor();
    virtual ~TextExtractor();

    void Init( const char* pszInput );

 private:
    /** Extract all text from the given page
     *
     *  \param pDocument the owning document
     *  \param pPage extract the text of this page.
     */
    void ExtractText( PdfMemDocument* pDocument, PdfPage* pPage );

    /** Adds a text string to a list which can be sorted by 
     *  position on the page later, so that the whole structure 
     *  of the text including formatting can be reconstructed.
     *
     *  \param dCurPosX x position of the text
     *  \param dCurPosY y position of the text
     *  \param pCurFont font of the text
     *  \param rString the actual string
     */
    void AddTextElement( double dCurPosX, double dCurPosY, 
                         PdfFont* pCurFont, const PdfString & rString );
};

#endif // _TEXT_EXTRACTOR_H_
