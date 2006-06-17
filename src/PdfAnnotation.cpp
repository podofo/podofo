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

PdfError PdfAnnotation::AddReferenceToKey( PdfObject* pObject, const PdfName & keyName, const PdfReference & rRef )
{
    PdfError     eCode;
    PdfVariant   var;
    TVariantList list;

    if( pObject->HasKey( keyName ) )
    {
        SAFE_OP( pObject->GetKeyValueVariant( keyName, var ) );
        if( var.GetDataType() != ePdfDataType_Array )
        {
            RAISE_ERROR( ePdfError_InvalidDataType );
        }

        list = var.GetArray();
    }

    var.SetDataType( ePdfDataType_Reference );
    var.SetReference( rRef );
    list.push_back( var );

    var.SetDataType( ePdfDataType_Array );
    SAFE_OP( var.SetArray( list ) );

    pObject->AddKey( keyName, var );

    return eCode;
}

PdfError PdfAnnotation::Init( PdfPage* pPage, EPdfAnnotation eAnnot, PdfRect & rRect )
{
    PdfError    eCode;
    PdfVariant  rect;

    SAFE_OP( this->Init( (PdfObject*)pPage, eAnnot, rRect ) );

    rRect.ToVariant( rect, pPage );
    SAFE_OP( this->AddKey( PdfName::KeyRect, rect ) );
             
    return eCode;
}

PdfError PdfAnnotation::Init( PdfObject* pObject, EPdfAnnotation eAnnot, PdfRect & rRect )
{
    PdfError    eCode;
    PdfVariant  rect;
    PdfDate     date;
    PdfString   sDate;

    const PdfName name( AnnotationKey( eAnnot ) );

    m_eAnnotation = eAnnot;

    if( !name.Length() )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    SAFE_OP( this->AddReferenceToKey( pObject, "Annots", this->Reference() ) );
    
    rRect.ToVariant( rect );
    date.ToString( sDate );

    SAFE_OP( this->AddKey( PdfName::KeySubtype, name ) );
    SAFE_OP( this->AddKey( PdfName::KeyRect, rect ) );
    SAFE_OP( this->AddKey( "P", pObject->Reference() ) );
    SAFE_OP( this->AddKey( "M", sDate ) );
    return eCode;
}

PdfError PdfAnnotation::SetAppearanceStream( PdfXObject* pObject )
{
    PdfError eCode;

    if( !pObject )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    PdfObject* pAP = new PdfObject( 0, 0 );
    pAP->SetDirect( true );
    pAP->AddKey( "N", pObject->Reference() );

    this->AddKey( "AP", pAP );

    return eCode;
}

void PdfAnnotation::SetFlags( pdf_uint32 uiFlags )
{
    this->AddKey( "F", (long)uiFlags );
}

void PdfAnnotation::SetTitle( const PdfString & sTitle )
{
    this->AddKey( "T", sTitle );
}

void PdfAnnotation::SetContents( const PdfString & sContents )
{
    this->AddKey( "Contents", sContents );
}

PdfError PdfAnnotation::SetDestination( const PdfPage* pPage )
{
    return this->SetDestination( pPage->Reference() );
}

PdfError PdfAnnotation::SetDestination( const PdfReference & rReference )
{
    PdfError     eCode;
    PdfVariant   var;
    PdfVariant   tmp;
    TVariantList list;

    tmp.SetDataType( ePdfDataType_Reference );
    tmp.SetReference( rReference );
    list.push_back( tmp );

    tmp.SetDataType( ePdfDataType_Name );
    tmp.SetName( "Fit" );
    list.push_back( tmp );

    var.SetDataType( ePdfDataType_Array );
    var.SetArray( list );

    SAFE_OP( this->AddKey( "Dest", var ) );
 
    return eCode;
}

PdfError PdfAnnotation::SetDestination( const PdfAction* pAction )
{
    PdfError eCode;

    if( !pAction )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    this->AddKey( "A", pAction->Reference() );

    return eCode;
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
