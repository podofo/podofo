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
#include "PdfDictionary.h"
#include "PdfDocument.h"
#include "PdfRect.h"
#include "PdfVariant.h"
#include "PdfWriter.h"


namespace PoDoFo {

PdfPage::PdfPage( PdfDocument* inOwningDoc, unsigned int nObjectNo, unsigned int nGenerationNo )
    : m_pDocument( inOwningDoc ), m_pObject( new PdfObject(nObjectNo, nGenerationNo, "Page") ), PdfCanvas()
{
    PdfDictionary resources;

    // The PDF specification suggests that we send all available PDF Procedure sets
    m_pObject->AddKey( "Resources", PdfVariant( resources ) );
    m_pResources = &(m_pObject->GetVariant().GetDictionary().GetKey( "Resources" ).GetDictionary());
    Resources()->AddKey( "ProcSet", PdfCanvas::ProcSet() );
}

PdfPage::PdfPage( PdfDocument* inOwningDoc, PdfObject* inObject )
    : m_pDocument( inOwningDoc ), m_pObject( inObject ), PdfCanvas()
{
    m_pResources = &(m_pObject->GetVariant().GetDictionary().GetKey( "Resources" ).GetDictionary());
	PdfVariant cVar = m_pObject->GetDictionary().GetKey( "Contents" );
	if ( cVar.IsReference() )	// let's hope so!
	{
		m_pContents = m_pDocument->GetObject( cVar.GetReference() );
	}
}

PdfPage::~PdfPage()
{

}

PdfError PdfPage::Init( const TSize & tSize, PdfVecObjects* pParent )
{
    PdfError   eCode;
    PdfVariant rect;

    m_pContents = pParent->CreateObject();

    m_tPageSize = tSize;

    PdfRect( 0, 0, tSize.lWidth, tSize.lHeight ).ToVariant( rect );

    m_pObject->AddKey( "MediaBox", rect );
    m_pObject->AddKey( PdfName::KeyContents, m_pContents->Reference() );

    return eCode;
}

PdfDictionary* PdfPage::Resources() const
{
    return m_pResources;
}

TSize PdfPage::CreateStandardPageSize( const EPdfPageSize ePageSize )
{
    TSize tSize;

    switch( ePageSize ) 
    {
        case ePdfPageSize_A4:
            tSize.lWidth  = 595;
            tSize.lHeight = 842;
            break;

        case ePdfPageSize_Letter:
            tSize.lWidth  = 612;
            tSize.lHeight = 792;
            break;
            
		case ePdfPageSize_Legal:
			tSize.lWidth  = 612;
			tSize.lHeight = 1008;
			break;

		case ePdfPageSize_A3:
			tSize.lWidth  = 842;
			tSize.lHeight = 1190;
			break;

        default:
            tSize.lWidth = tSize.lHeight = 0;
            break;
    }

    return tSize;
}

const PdfRect PdfPage::GetPageBox( const char* inBox ) const
{
	PdfRect		pageBox;
	PdfVariant	pbVar;

	// first we check the actual /Page object for the box data - this is the normal case
	if ( m_pObject->HasKey( inBox ) ) 
	{
		pbVar = m_pObject->GetKey( inBox );
	}

	// however, it may be that it is taking advantage of inherited values, so we need to
	// walk up the pages tree to find it
	// TODO!

	// assign the value of the box from the array
	if ( pbVar.IsArray() )
		pageBox.FromArray( pbVar.GetArray() );

	return pageBox;
}

};
