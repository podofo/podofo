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
    const PdfName type = PdfName( PdfAction::ActionName( eAction ) );

    if( !type.Length() )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_pObject->GetDictionary().AddKey( "S", type );
}

PdfAction::PdfAction( PdfObject* pObject )
    : PdfElement( "Action", pObject )
{
    m_eType = PdfAction::ActionType( m_pObject->GetDictionary().GetKeyAsName( "S" ).Name().c_str() );
}

const char* PdfAction::ActionName( EPdfAction eAction )
{
    const char* pszKey = NULL;

    if( (long)eAction < PdfAction::s_lNumActions )
    {
        pszKey = PdfAction::s_names[(int)eAction];
    }

    return pszKey;
}

EPdfAction PdfAction::ActionType( const char* pszType )
{
    EPdfAction eAction = ePdfAction_Unknown;
    int        i;

    if( !pszType )
        return eAction;

    for( i=0; i<PdfAction::s_lNumActions; i++ )
        if( strcmp( pszType, PdfAction::s_names[i] ) == 0 )
        {
            eAction = (EPdfAction)i;
            break;
        }

    return eAction;
}

void PdfAction::SetURI( const PdfString & sUri )
{
    m_pObject->GetDictionary().AddKey( "URI", sUri );
}

};

