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

#include "PdfXObject.h" 

#include "PdfDictionary.h"
#include "PdfImage.h"
#include "PdfRect.h"
#include "PdfPage.h"
#include "PdfVariant.h"
#include "PdfDocument.h"
#include "PdfLocale.h"

#include <sstream>

using namespace std;

namespace PoDoFo {

PdfArray PdfXObject::s_matrix;

PdfXObject::PdfXObject( const PdfRect & rRect, PdfDocument* pParent )
    : PdfElement( "XObject", pParent ), PdfCanvas(), m_rRect( rRect )
{
    InitXObject( rRect );
}

PdfXObject::PdfXObject( const PdfRect & rRect, PdfVecObjects* pParent )
    : PdfElement( "XObject", pParent ), PdfCanvas(), m_rRect( rRect )
{
    InitXObject( rRect );
}

PdfXObject::PdfXObject( const PdfMemDocument & rDoc, int nPage, PdfDocument* pParent )
    : PdfElement( "XObject", pParent ), PdfCanvas()
{
    m_rRect = PdfRect();

    InitXObject( m_rRect );

    // Implementation note: source document must be different from distination
    if ( pParent == reinterpret_cast<const PdfDocument*>(&rDoc) )
    {
        PODOFO_RAISE_ERROR( ePdfError_InternalLogic );
    }
    // After filling set correct BBox
    m_rRect = pParent->FillXObjectFromDocumentPage( this, rDoc, nPage );

    PdfVariant    var;
    m_rRect.ToVariant( var );
    m_pObject->GetDictionary().AddKey( "BBox", var );

    PdfArray      matrix;
    matrix.push_back( PdfVariant( 1L ) );
    matrix.push_back( PdfVariant( 0L ) );
    matrix.push_back( PdfVariant( 0L ) );
    matrix.push_back( PdfVariant( 1L ) );
	if( m_rRect.GetLeft() != 0 )
	    matrix.push_back( PdfVariant( m_rRect.GetLeft() * (-1.0) ) );
	else
	    matrix.push_back( PdfVariant( 0L ) );
	if( m_rRect.GetBottom() != 0 )
	    matrix.push_back( PdfVariant( m_rRect.GetBottom() * (-1.0) ) );
	else
	    matrix.push_back( PdfVariant( 0L ) );
    m_pObject->GetDictionary().AddKey( "Matrix", matrix );
}

PdfXObject::PdfXObject( PdfObject* pObject )
    : PdfElement( "XObject", pObject ), PdfCanvas()
{
    ostringstream out;
    PdfLocaleImbue(out);
    // Implementation note: the identifier is always
    // Prefix+ObjectNo. Prefix is /XOb for XObject.
    out << "XOb" << m_pObject->Reference().ObjectNumber();

    
    m_pResources = pObject->GetIndirectKey( "Resources" );
    m_Identifier = PdfName( out.str().c_str() );
    m_rRect      = PdfRect( m_pObject->GetIndirectKey( "BBox" )->GetArray() );
    m_Reference  = m_pObject->Reference();
}

void PdfXObject::InitXObject( const PdfRect & rRect )
{
    PdfVariant    var;
    ostringstream out;
    PdfLocaleImbue(out);

    // Initialize static data
    if( s_matrix.empty() )
    {
        // This matrix is the same for all PdfXObjects so cache it
        s_matrix.push_back( PdfVariant( 1L ) );
        s_matrix.push_back( PdfVariant( 0L ) );
        s_matrix.push_back( PdfVariant( 0L ) );
        s_matrix.push_back( PdfVariant( 1L ) );
        s_matrix.push_back( PdfVariant( 0L ) );
        s_matrix.push_back( PdfVariant( 0L ) );
    }

    rRect.ToVariant( var );
    m_pObject->GetDictionary().AddKey( "BBox", var );
    m_pObject->GetDictionary().AddKey( PdfName::KeySubtype, PdfName("Form") );
    m_pObject->GetDictionary().AddKey( "FormType", PdfVariant( 1L ) ); // only 1 is only defined in the specification.
    m_pObject->GetDictionary().AddKey( "Matrix", s_matrix );

    // The PDF specification suggests that we send all available PDF Procedure sets
    m_pObject->GetDictionary().AddKey( "Resources", PdfObject( PdfDictionary() ) );
    m_pResources = m_pObject->GetDictionary().GetKey( "Resources" );
    m_pResources->GetDictionary().AddKey( "ProcSet", PdfCanvas::GetProcSet() );

    // Implementation note: the identifier is always
    // Prefix+ObjectNo. Prefix is /XOb for XObject.
    out << "XOb" << m_pObject->Reference().ObjectNumber();
    m_Identifier = PdfName( out.str().c_str() );
    m_Reference  = m_pObject->Reference();
}

PdfXObject::PdfXObject( const char* pszSubType, PdfDocument* pParent )
    : PdfElement( "XObject", pParent ) 
{
    ostringstream out;
    PdfLocaleImbue(out);
    // Implementation note: the identifier is always
    // Prefix+ObjectNo. Prefix is /XOb for XObject.
    out << "XOb" << m_pObject->Reference().ObjectNumber();

    m_Identifier = PdfName( out.str().c_str() );
    m_Reference  = m_pObject->Reference();

    m_pObject->GetDictionary().AddKey( PdfName::KeySubtype, PdfName( pszSubType ) );
}

PdfXObject::PdfXObject( const char* pszSubType, PdfVecObjects* pParent )
    : PdfElement( "XObject", pParent ) 
{
    ostringstream out;
    PdfLocaleImbue(out);
    // Implementation note: the identifier is always
    // Prefix+ObjectNo. Prefix is /XOb for XObject.
    out << "XOb" << m_pObject->Reference().ObjectNumber();

    m_Identifier = PdfName( out.str().c_str() );
    m_Reference  = m_pObject->Reference();

    m_pObject->GetDictionary().AddKey( PdfName::KeySubtype, PdfName( pszSubType ) );
}

PdfXObject::PdfXObject( const char* pszSubType, PdfObject* pObject )
    : PdfElement( "XObject", pObject ) 
{
    ostringstream out;
    PdfLocaleImbue(out);

    if( m_pObject->GetDictionary().GetKeyAsName( PdfName::KeySubtype ) != pszSubType ) 
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
    }

    // Implementation note: the identifier is always
    // Prefix+ObjectNo. Prefix is /XOb for XObject.
    out << "XOb" << m_pObject->Reference().ObjectNumber();

    m_Identifier = PdfName( out.str().c_str() );
    m_Reference  = m_pObject->Reference();
}

};
