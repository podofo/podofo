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
#include "PdfRect.h"
#include "PdfVariant.h"
#include "PdfWriter.h"


namespace PoDoFo {

PdfPage::PdfPage( const TSize & tSize, PdfWriter* pWriter, unsigned int nObjectNo, unsigned int nGenerationNo )
    : PdfObject( nObjectNo, nGenerationNo, "Page" ), PdfCanvas(), m_pWriter( pWriter )
{
    PdfVariant rect;

    m_pResources = new PdfObject( 0, 0, NULL );
    m_pResources->SetDirect( true );

    m_tPageSize = tSize;

    PdfRect( 0, 0, tSize.lWidth, tSize.lHeight ).ToVariant( rect );
    this->AddKey( "Resources", m_pResources );
    this->AddKey( "MediaBox", rect );

    // The PDF specification suggests that we send all available PDF Procedure sets
    m_pResources->AddKey( "ProcSet", "[/PDF /Text /ImageB /ImageC /ImageI]" );
}

PdfPage::~PdfPage()
{

}

PdfError PdfPage::Init()
{
    PdfError eCode;

    m_pContents = m_pWriter->CreateObject();

    this->AddKey( PdfName::KeyContents, m_pContents->Reference() );

    return eCode;
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
