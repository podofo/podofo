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

#include "PdfDictionary.h"

namespace PoDoFo {

const long  PdfAction::s_lNumActions = 19;
const char* PdfAction::s_names[] = {
    "GoTo",
    "GoToR",
    "GoToE",
    "Launch",
    "Thread",
    "URI",
    "Sound",
    "Movie",
    "Hide",
    "Named",
    "SubmitForm",
    "ResetForm",
    "ImportData",
    "JavaScript",
    "SetOCGState",
    "Rendition",
    "Trans",
    "GoTo3DView",
    NULL
};

PdfAction::PdfAction( EPdfAction eAction, PdfVecObjects* pParent )
    : PdfElement( "Action", pParent ), m_eType( eAction )
{
    const PdfName type = PdfName( TypeNameForIndex( eAction, s_names, s_lNumActions ) );

    if( !type.GetLength() )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_pObject->GetDictionary().AddKey( "S", type );
}

PdfAction::PdfAction( PdfObject* pObject )
    : PdfElement( "Action", pObject )
{
    m_eType = static_cast<EPdfAction>(TypeNameToIndex( m_pObject->GetDictionary().GetKeyAsName( "S" ).GetName().c_str(), s_names, s_lNumActions ));
}

PdfAction::PdfAction( const PdfAction & rhs )
    : PdfElement( "Action", rhs.m_pObject )
{
    m_eType = static_cast<EPdfAction>(TypeNameToIndex( m_pObject->GetDictionary().GetKeyAsName( "S" ).GetName().c_str(), s_names, s_lNumActions ));
}

void PdfAction::SetURI( const PdfString & sUri )
{
    m_pObject->GetDictionary().AddKey( "URI", sUri );
}

PdfString PdfAction::GetURI() const
{
    return m_pObject->GetDictionary().GetKey( "URI" )->GetString();
}

bool PdfAction::HasURI() const
{
    return m_pObject->GetDictionary().HasKey( "URI" );
}



};

