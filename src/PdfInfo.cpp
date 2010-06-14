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

#include "PdfDefinesPrivate.h"
#define PRODUCER_STRING "PoDoFo - http://podofo.sf.net"

namespace PoDoFo {

PdfInfo::PdfInfo( PdfVecObjects* pParent, int eInitial )
    : PdfElement( NULL, pParent )
{
    Init( eInitial );
}

PdfInfo::PdfInfo( PdfObject* pObject, int eInitial )
    : PdfElement( NULL, pObject )
{
    Init( eInitial );
}

PdfInfo::~PdfInfo()
{
}

void PdfInfo::Init( int eInitial )
{
    PdfDate   date;
    PdfString str;

    date.ToString( str );
    
    if( (eInitial & ePdfInfoInitial_WriteCreationTime) == ePdfInfoInitial_WriteCreationTime ) 
    {
        this->GetObject()->GetDictionary().AddKey( "CreationDate", str );
    }

    if( (eInitial & ePdfInfoInitial_WriteModificationTime) == ePdfInfoInitial_WriteModificationTime ) 
    {
        this->GetObject()->GetDictionary().AddKey( "ModDate", str );
    }

    if( (eInitial & ePdfInfoInitial_WriteProducer) == ePdfInfoInitial_WriteProducer ) 
    {
        this->GetObject()->GetDictionary().AddKey( "Producer", PdfString(PRODUCER_STRING) );
    }
}

const PdfString & PdfInfo::GetStringFromInfoDict( const PdfName & rName ) const
{
    const PdfObject* pObj = this->GetObject()->GetDictionary().GetKey( rName );
    
    return pObj && (pObj->IsString() || pObj->IsHexString()) ? pObj->GetString() : PdfString::StringNull;
}

const PdfName & PdfInfo::GetNameFromInfoDict(const PdfName & rName) const
{
	const PdfObject* pObj = this->GetObject()->GetDictionary().GetKey( rName );
    
	return pObj && pObj->IsName() ? pObj->GetName() : PdfName::KeyNull;
}

void PdfInfo::SetAuthor( const PdfString & sAuthor )
{
    this->GetObject()->GetDictionary().AddKey( "Author", sAuthor );
}

void PdfInfo::SetCreator( const PdfString & sCreator )
{
    this->GetObject()->GetDictionary().AddKey( "Creator", sCreator );
}

void PdfInfo::SetKeywords( const PdfString & sKeywords )
{
    this->GetObject()->GetDictionary().AddKey( "Keywords", sKeywords );
}

void PdfInfo::SetSubject( const PdfString & sSubject )
{
    this->GetObject()->GetDictionary().AddKey( "Subject", sSubject );
}

void PdfInfo::SetTitle( const PdfString & sTitle )
{
    this->GetObject()->GetDictionary().AddKey( "Title", sTitle );
}

// Peter Petrov 27 April 2008
// We have added a SetProducer() method in PdfInfo
void PdfInfo::SetProducer( const PdfString & sProducer )
{
    this->GetObject()->GetDictionary().AddKey( "Producer", sProducer );
}

void PdfInfo::SetTrapped(const PdfName & sTrapped)
{
	if((sTrapped.GetEscapedName() == "True" ) || (sTrapped.GetEscapedName() == "False" ))
		this->GetObject()->GetDictionary().AddKey( "Trapped", sTrapped );
	else
		this->GetObject()->GetDictionary().AddKey( "Trapped", PdfName( "Unknown" ) );
}

};



