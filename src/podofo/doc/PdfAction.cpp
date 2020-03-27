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

#include "PdfAction.h"

#include "base/PdfDefinesPrivate.h"
#include "base/PdfDictionary.h"

namespace PoDoFo {

const long  PdfAction::s_lNumActions = 18;
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
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    this->GetObject()->GetDictionary().AddKey( "S", type );
}

PdfAction::PdfAction( EPdfAction eAction, PdfDocument* pParent )
    : PdfElement( "Action", pParent ), m_eType( eAction )
{
    const PdfName type = PdfName( TypeNameForIndex( eAction, s_names, s_lNumActions ) );

    if( !type.GetLength() )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    this->GetObject()->GetDictionary().AddKey( "S", type );
}

PdfAction::PdfAction( PdfObject* pObject )
    // The typename /Action is optional for PdfActions
    : PdfElement( NULL, pObject )
{
    m_eType = static_cast<EPdfAction>(TypeNameToIndex( this->GetObject()->GetIndirectKeyAsName( "S" ).GetName().c_str(), s_names, s_lNumActions, ePdfAction_Unknown ));
}

PdfAction::PdfAction( const PdfAction & rhs )
    : PdfElement( "Action", rhs.GetNonConstObject() )
{
    m_eType = static_cast<EPdfAction>(TypeNameToIndex( this->GetObject()->GetIndirectKeyAsName( "S" ).GetName().c_str(), s_names, s_lNumActions, ePdfAction_Unknown ));
}

void PdfAction::SetURI( const PdfString & sUri )
{
    this->GetObject()->GetDictionary().AddKey( "URI", sUri );
}

PdfString PdfAction::GetURI() const
{
    return this->GetObject()->MustGetIndirectKey( "URI" )->GetString();
}

bool PdfAction::HasURI() const
{
    return (this->GetObject()->GetIndirectKey( "URI" ) != NULL);
}

void PdfAction::SetScript( const PdfString & sScript )
{
    this->GetObject()->GetDictionary().AddKey( "JS", sScript );

}

PdfString PdfAction::GetScript() const
{
    return this->GetObject()->MustGetIndirectKey( "JS" )->GetString();

}

bool PdfAction::HasScript() const
{
    return this->GetObject()->GetDictionary().HasKey( "JS" );
}

void PdfAction::AddToDictionary( PdfDictionary & dictionary ) const
{
    // Do not add empty destinations
//    if( !m_array.size() )
//        return;

    // since we can only have EITHER a Dest OR an Action
    // we check for an Action, and if already present, we throw
    if ( dictionary.HasKey( PdfName( "Dest" ) ) )
        PODOFO_RAISE_ERROR( ePdfError_ActionAlreadyPresent );

    dictionary.RemoveKey( "A" );
    dictionary.AddKey( "A", this->GetObject() );
}



};

