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

#include "PdfFontFactory.h"

#include "PdfFont.h"
#include "PdfFontMetrics.h"
#include "PdfFontType1.h"
#include "PdfFontTrueType.h"

namespace PoDoFo {

PdfFont* PdfFontFactory::CreateFont( PdfFontMetrics* pMetrics, bool bEmbedd, bool bBold, bool bItalic, PdfVecObjects* pParent )
{
    PdfFont*     pFont = NULL;
    EPdfFontType eType = pMetrics->GetFontType();

    switch( eType ) 
    {
        case ePdfFontType_TrueType:
            pFont = new PdfFontTrueType( pMetrics, pParent );
            break;

        case ePdfFontType_Type1Pfa:
        case ePdfFontType_Type1Pfb:
            pFont = new PdfFontType1( pMetrics, bEmbedd, pParent );
            break;

        case ePdfFontType_Unknown:
        default:
            PdfError::LogMessage( eLogSeverity_Error, "The font format is unknown. Fontname: %s Filename: %s\n", 
                                  (pMetrics->GetFontname() ? pMetrics->GetFontname() : "<unknown>"),
                                  (pMetrics->GetFilename() ? pMetrics->GetFilename() : "<unknown>") );
    }

    if( pFont ) 
    {
        pFont->SetBold( bBold );
        pFont->SetItalic( bItalic );
    }

    return pFont;
}

};
