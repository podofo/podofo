/***************************************************************************
 *   Copyright (C) 2005 by Dominik Seichter                                *
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

#include "PdfPage.h" 
#include "PdfDictionary.h"
#include "PdfRect.h"
#include "PdfVariant.h"
#include "PdfWriter.h"


namespace PoDoFo {

PdfPage::PdfPage( unsigned int nObjectNo, unsigned int nGenerationNo )
    : PdfObject( nObjectNo, nGenerationNo, "Page" ), PdfCanvas()
{
    PdfDictionary resources;

    // The PDF specification suggests that we send all available PDF Procedure sets
    this->AddKey( "Resources", PdfVariant( resources ) );
    m_pResources = &(GetVariant().GetDictionary().GetKey( "Resources" ).GetDictionary());
    Resources()->AddKey( "ProcSet", PdfCanvas::ProcSet() );
}

PdfPage::~PdfPage()
{

}

PdfError PdfPage::Init( const TSize & tSize, PdfVecObjects* pParent )
{
    PdfError   eCode;
    PdfVariant rect;

    m_pContents = pParent->CreateObject();

    m_tPageSize = tSize;

    PdfRect( 0, 0, tSize.lWidth, tSize.lHeight ).ToVariant( rect );

    this->AddKey( "MediaBox", rect );
    this->AddKey( PdfName::KeyContents, m_pContents->Reference() );

    return eCode;
}

PdfDictionary* PdfPage::Resources() const
{
    return m_pResources;
}

TSize PdfPage::CreateStadardPageSize( const EPdfPageSize ePageSize )
{
    TSize tSize;

    switch( ePageSize ) 
    {
        case ePdfPageSize_A4:
            tSize.lWidth  = 210000;
            tSize.lHeight = 297000;
            break;

        case ePdfPageSize_Letter:
            tSize.lWidth  = 216000;
            tSize.lHeight = 279000;
            break;
            
        default:
            tSize.lWidth = 
                tSize.lHeight = 0;
            break;
    }

    return tSize;
}

};
