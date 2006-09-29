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

const long  PdfAnnotation::s_lNumActions = 26;
const char* PdfAnnotation::s_names[] = {
    "Text",                       // - supported
    "Link",
    "FreeText",       // PDF 1.3  // - supported
    "Line",           // PDF 1.3  // - supported
    "Square",         // PDF 1.3
    "Circle",         // PDF 1.3
    "Polygon",        // PDF 1.5
    "PolyLine",       // PDF 1.5
    "Highlight",      // PDF 1.3
    "Underline",      // PDF 1.3
    "Squiggly",       // PDF 1.4
    "StrikeOut",      // PDF 1.3
    "Stamp",          // PDF 1.3
    "Caret",          // PDF 1.5
    "Ink",            // PDF 1.3
    "Popup",          // PDF 1.3
    "FileAttachement",// PDF 1.3
    "Sound",          // PDF 1.2
    "Movie",          // PDF 1.2
    "Widget",         // PDF 1.2  // - supported
    "Screen",         // PDF 1.5
    "PrinterMark",    // PDF 1.4
    "TrapNet",        // PDF 1.3
    "Watermark",      // PDF 1.6
    "3D",             // PDF 1.6
    NULL
};

PdfAnnotation::PdfAnnotation( PdfPage* pPage, EPdfAnnotation eAnnot, const PdfRect & rRect, PdfVecObjects* pParent )
    : PdfElement( "Annot", pParent ), m_eAnnotation( eAnnot )
{
    PdfVariant    rect;
    PdfDate       date;
    PdfString     sDate;
    const PdfName name( TypeNameForIndex( eAnnot, s_names, s_lNumActions ) );

    if( !name.GetLength() )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    rRect.ToVariant( rect );

    m_pObject->GetDictionary().AddKey( PdfName::KeyRect, rect );
    this->AddReferenceToKey( pPage->GetObject(), "Annots", m_pObject->Reference() );

    rRect.ToVariant( rect );
    date.ToString( sDate );
    
    m_pObject->GetDictionary().AddKey( PdfName::KeySubtype, name );
    m_pObject->GetDictionary().AddKey( PdfName::KeyRect, rect );
    m_pObject->GetDictionary().AddKey( "P", pPage->GetObject()->Reference() );
    m_pObject->GetDictionary().AddKey( "M", sDate );

}

PdfAnnotation::PdfAnnotation( PdfPage* pPage, EPdfAnnotation eAnnot, const PdfRect & rRect, PdfObject* pObject )
    : PdfElement( "Annot", pObject ), m_eAnnotation( ePdfAnnotation_Unknown )
{
    m_eAnnotation = (EPdfAnnotation)TypeNameToIndex( m_pObject->GetDictionary().GetKeyAsName( "S" ).GetName().c_str(), s_names, s_lNumActions );
}

PdfAnnotation::PdfAnnotation( PdfPage* pPage, PdfObject* pObject )
    : PdfElement( "Annot", pObject ), m_eAnnotation( ePdfAnnotation_Unknown )
{
    m_eAnnotation = (EPdfAnnotation)TypeNameToIndex( m_pObject->GetDictionary().GetKeyAsName( "S" ).GetName().c_str(), s_names, s_lNumActions );

    
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

    dict.AddKey( "N", pObject->GetObject()->Reference() );

    m_pObject->GetDictionary().AddKey( "AP", dict );
}

void PdfAnnotation::SetFlags( pdf_uint32 uiFlags )
{
    m_pObject->GetDictionary().AddKey( "F", PdfVariant( (long)uiFlags ) );
}

void PdfAnnotation::SetTitle( const PdfString & sTitle )
{
    m_pObject->GetDictionary().AddKey( "T", sTitle );
}

void PdfAnnotation::SetContents( const PdfString & sContents )
{
    m_pObject->GetDictionary().AddKey( "Contents", sContents );
}

void PdfAnnotation::SetDestination( const PdfPage* pPage )
{
    this->SetDestination( pPage->GetObject()->Reference() );
}

void PdfAnnotation::SetDestination( const PdfReference & rReference )
{
    PdfArray list;

    list.push_back( rReference );
    list.push_back( PdfName( "Fit" ) );

    m_pObject->GetDictionary().AddKey( "Dest", list );
 }

void PdfAnnotation::SetDestination( const PdfAction* pAction )
{
    if( !pAction )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_pObject->GetDictionary().AddKey( "A", pAction->GetObject()->Reference() );
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
