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

#include "PdfInfo.h"

#include "PdfDate.h"
#include "PdfDictionary.h"
#include "PdfString.h"

#define PRODUCER_STRING "PoDoFo - http://podofo.sf.net"

namespace PoDoFo {

PdfInfo::PdfInfo( PdfVecObjects* pParent )
    : PdfElement( NULL, pParent )
{
    Init( false );
}

PdfInfo::PdfInfo( PdfObject* pObject )
    : PdfElement( NULL, pObject )
{
    Init( true );
}

PdfInfo::~PdfInfo()
{
}

void PdfInfo::Init( bool bModify )
{
    PdfDate   date;
    PdfString str;

    date.ToString( str );
    
    m_pObject->GetDictionary().AddKey( bModify ? "ModDate" : "CreationDate", str );
    m_pObject->GetDictionary().AddKey( "Producer", PdfString( PRODUCER_STRING) );
}

const PdfString & PdfInfo::GetStringFromInfoDict( const PdfName & rName ) const
{
    PdfObject* pObj = m_pObject->GetDictionary().GetKey( rName );
    
    return pObj && pObj->IsString() ? pObj->GetString() : PdfString::StringNull;
}

const PdfName & PdfInfo::GetNameFromInfoDict(const PdfName & rName) const
{
	PdfObject* pObj = m_pObject->GetDictionary().GetKey( rName );
    
	return pObj && pObj->IsName() ? pObj->GetName() : PdfName::KeyNull;
}

void PdfInfo::SetAuthor( const PdfString & sAuthor )
{
    m_pObject->GetDictionary().AddKey( "Author", sAuthor );
}

void PdfInfo::SetCreator( const PdfString & sCreator )
{
    m_pObject->GetDictionary().AddKey( "Creator", sCreator );
}

void PdfInfo::SetKeywords( const PdfString & sKeywords )
{
    m_pObject->GetDictionary().AddKey( "Keywords", sKeywords );
}

void PdfInfo::SetSubject( const PdfString & sSubject )
{
    m_pObject->GetDictionary().AddKey( "Subject", sSubject );
}

void PdfInfo::SetTitle( const PdfString & sTitle )
{
    m_pObject->GetDictionary().AddKey( "Title", sTitle );
}

void PdfInfo::SetTrapped(const PdfName & sTrapped)
{
	if((sTrapped.GetEscapedName() == "True" ) || (sTrapped.GetEscapedName() == "False" ))
		m_pObject->GetDictionary().AddKey( "Trapped", sTrapped );
	else
		m_pObject->GetDictionary().AddKey( "Trapped", PdfName( "Unknown" ) );
}

};



