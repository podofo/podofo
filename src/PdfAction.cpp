/***************************************************************************
 *   Copyright (C) 2006 by Dominik Seichter                                *
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

#include "PdfAction.h"

namespace PoDoFo {

PdfAction::PdfAction( unsigned int nObjectNo, unsigned int nGenerationNo )
    : PdfObject( nObjectNo, nGenerationNo, "Action" )
{

}

PdfError PdfAction::Init( EPdfAction eAction )
{
    PdfError    eCode;
    const PdfName type = PdfName( ActionKey( eAction ) );

    if( !type.Length() )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    this->AddKey( "S", type );

    return eCode;
}

const char* PdfAction::ActionKey( EPdfAction eAction )
{
    const char* pszKey;

    switch( eAction ) 
    {
        case ePdfAction_URI:
            pszKey = "URI"; break;
        default:
            pszKey = NULL;
    }

    return pszKey;
}

void PdfAction::SetURI( const PdfString & sUri )
{
    this->AddKey( "URI", sUri );
}

};

