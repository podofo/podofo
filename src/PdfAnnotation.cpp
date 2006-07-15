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

#include "PdfAnnotation.h"
#include "PdfAction.h"
#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfDate.h"
#include "PdfPage.h"
#include "PdfRect.h"
#include "PdfVariant.h"
#include "PdfXObject.h"

namespace PoDoFo {

PdfAnnotation::PdfAnnotation( unsigned int nObjectNo, unsigned int nGenerationNo )
    : PdfObject( nObjectNo, nGenerationNo, "Annot" )
{
    m_eAnnotation = ePdfAnnotation_Unknown;
}

void PdfAnnotation::Init( PdfPage* pPage, EPdfAnnotation eAnnot, const PdfRect & rRect )
{
    PdfVariant  rect;

    this->Init( (PdfObject*)pPage, eAnnot, rRect );

    rRect.ToVariant( rect, pPage );
    this->GetDictionary().AddKey( PdfName::KeyRect, rect );
}

void PdfAnnotation::Init( PdfObject* pObject, EPdfAnnotation eAnnot, const PdfRect & rRect )
{
    PdfVariant  rect;
    PdfDate     date;
    PdfString   sDate;

    const PdfName name( AnnotationKey( eAnnot ) );

    m_eAnnotation = eAnnot;

    if( !name.Length() )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    this->AddReferenceToKey( pObject, "Annots", this->Reference() );
    
    rRect.ToVariant( rect );
    date.ToString( sDate );

    this->GetDictionary().AddKey( PdfName::KeySubtype, name );
    this->GetDictionary().AddKey( PdfName::KeyRect, rect );
    this->GetDictionary().AddKey( "P", pObject->Reference() );
    this->GetDictionary().AddKey( "M", sDate );
}

void PdfAnnotation::AddReferenceToKey( PdfObject* pObject, const PdfName & keyName, const PdfReference & rRef )
{
    PdfObject*    pObj;
    PdfArray      list;

    if( pObject->GetDictionary().HasKey( keyName ) )
    {
        pObj = pObject->GetIndirectKey( keyName );
        if( !(pObj && pObj->IsArray()) )
        {
            RAISE_ERROR( ePdfError_InvalidDataType );
        }

        list = pObj->GetArray();
    }

    list.push_back( PdfVariant( rRef ) );

    pObject->GetDictionary().AddKey( keyName, PdfVariant( list ) );
}

void PdfAnnotation::SetAppearanceStream( PdfXObject* pObject )
{
    PdfDictionary dict;

    if( !pObject )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    dict.AddKey( "N", pObject->Reference() );

    this->GetDictionary().AddKey( "AP", dict );
}

void PdfAnnotation::SetFlags( pdf_uint32 uiFlags )
{
    this->GetDictionary().AddKey( "F", PdfVariant( (long)uiFlags ) );
}

void PdfAnnotation::SetTitle( const PdfString & sTitle )
{
    this->GetDictionary().AddKey( "T", sTitle );
}

void PdfAnnotation::SetContents( const PdfString & sContents )
{
    this->GetDictionary().AddKey( "Contents", sContents );
}

void PdfAnnotation::SetDestination( const PdfPage* pPage )
{
    return this->SetDestination( pPage->GetObject()->Reference() );
}

void PdfAnnotation::SetDestination( const PdfReference & rReference )
{
    PdfArray list;

    list.push_back( PdfVariant( rReference ) );
    list.push_back( PdfVariant( PdfName( "Fit" ) ) );

    this->GetDictionary().AddKey( "Dest", PdfVariant( list ) );
 }

void PdfAnnotation::SetDestination( const PdfAction* pAction )
{
    if( !pAction )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    this->GetDictionary().AddKey( "A", pAction->Reference() );
}

const char* PdfAnnotation::AnnotationKey( EPdfAnnotation eAnnot )
{
    char* pszKey;

    switch( eAnnot ) 
    {
        case ePdfAnnotation_Text:
            pszKey = "Text"; break;
        case ePdfAnnotation_Link:
            pszKey = "Link"; break;
        case ePdfAnnotation_FreeText:
            pszKey = "FreeText"; break;
        case ePdfAnnotation_Widget:
            pszKey = "Widget"; break;
        case ePdfAnnotation_Unknown:
        default:
            pszKey = NULL; break;
    }

    return pszKey;
}

};
