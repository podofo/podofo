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

#include "PdfXObject.h" 

#include "base/PdfDefinesPrivate.h"

#include "base/PdfDictionary.h"
#include "base/PdfLocale.h"
#include "base/PdfRect.h"
#include "base/PdfVariant.h"

#include "PdfImage.h"
#include "PdfPage.h"
#include "PdfMemDocument.h"
#include "PdfDocument.h"

#include <sstream>

#define PI           3.141592654f

using namespace std;

namespace PoDoFo {

PdfXObject::PdfXObject( const PdfRect & rRect, PdfDocument* pParent, const char* pszPrefix, bool bWithoutIdentifier )
    : PdfElement( "XObject", pParent ), PdfCanvas(), m_rRect( rRect ), m_pResources( NULL )
{
    InitXObject( rRect, pszPrefix );
    if( bWithoutIdentifier )
    {
       m_Identifier = PdfName(pszPrefix);
    }
}

PdfXObject::PdfXObject( const PdfRect & rRect, PdfVecObjects* pParent, const char* pszPrefix )
    : PdfElement( "XObject", pParent ), PdfCanvas(), m_rRect( rRect ), m_pResources( NULL )
{
    InitXObject( rRect, pszPrefix );
}

PdfXObject::PdfXObject( const PdfMemDocument & rDoc, int nPage, PdfDocument* pParent, const char* pszPrefix, bool bUseTrimBox )
    : PdfElement( "XObject", pParent ), PdfCanvas(), m_pResources( NULL )
{
    m_rRect = PdfRect();

    InitXObject( m_rRect, pszPrefix );

    // Implementation note: source document must be different from distination
    if ( pParent == reinterpret_cast<const PdfDocument*>(&rDoc) )
    {
        PODOFO_RAISE_ERROR( ePdfError_InternalLogic );
    }

    // After filling set correct BBox, independent of rotation
    m_rRect = pParent->FillXObjectFromDocumentPage( this, rDoc, nPage, bUseTrimBox );

    PdfVariant    var;
    m_rRect.ToVariant( var );
    this->GetObject()->GetDictionary().AddKey( "BBox", var );

 	int rotation = rDoc.GetPage( nPage )->GetRotation();
	// correct negative rotation
	if ( rotation < 0 )
		rotation = 360 + rotation;

	// Swap offsets/width/height for vertical rotation
 	switch ( rotation )
 	{
 		case 90:
 		case 270:
 		{
 			double temp;
			
			temp = m_rRect.GetWidth();
 			m_rRect.SetWidth( m_rRect.GetHeight() );
 			m_rRect.SetHeight( temp );

			temp = m_rRect.GetLeft();
 			m_rRect.SetLeft( m_rRect.GetBottom() );
 			m_rRect.SetBottom( temp );
 		}
 		break;
        
 		default:
            break;
 	}
 
 	// Build matrix for rotation and cropping
 	double alpha = -rotation / 360.0 * 2.0 * PI;
    
 	double a, b, c, d, e, f;
    
 	a = cos( alpha );
 	b = sin( alpha );
 	c = -sin( alpha );
 	d = cos( alpha );
 
 	switch ( rotation )
 	{
 		case 90:
 			e = - m_rRect.GetLeft();
			f = m_rRect.GetBottom() + m_rRect.GetHeight();
            break;
  
  		case 180:
 			e = m_rRect.GetLeft() + m_rRect.GetWidth();
 			f = m_rRect.GetBottom() + m_rRect.GetHeight();
            break;
            
 		case 270:
 			e = m_rRect.GetLeft() + m_rRect.GetWidth();
 			f = - m_rRect.GetBottom();
            break;
 
 		case 0:
 		default:
 			e = - m_rRect.GetLeft();
 			f = - m_rRect.GetBottom();
            break;
 	}

    PdfArray      matrix;
    matrix.push_back( PdfVariant( a ) );
    matrix.push_back( PdfVariant( b ) );
    matrix.push_back( PdfVariant( c ) );
    matrix.push_back( PdfVariant( d ) );
    matrix.push_back( PdfVariant( e ) );
    matrix.push_back( PdfVariant( f ) );
    
    this->GetObject()->GetDictionary().AddKey( "Matrix", matrix );
}

PdfXObject::PdfXObject( PdfDocument *pDoc, int nPage, const char* pszPrefix, bool bUseTrimBox )
    : PdfElement( "XObject", pDoc ), PdfCanvas(), m_pResources( NULL )
{
    m_rRect = PdfRect();

    InitXObject( m_rRect, pszPrefix );

    // After filling set correct BBox, independent of rotation
    m_rRect = pDoc->FillXObjectFromExistingPage( this, nPage, bUseTrimBox );

    PdfVariant    var;
    m_rRect.ToVariant( var );
    this->GetObject()->GetDictionary().AddKey( "BBox", var );

 	int rotation = pDoc->GetPage( nPage )->GetRotation();
	// correct negative rotation
	if ( rotation < 0 )
		rotation = 360 + rotation;

	// Swap offsets/width/height for vertical rotation
 	switch ( rotation )
 	{
 		case 90:
 		case 270:
 		{
 			double temp;
			
			temp = m_rRect.GetWidth();
 			m_rRect.SetWidth( m_rRect.GetHeight() );
 			m_rRect.SetHeight( temp );

			temp = m_rRect.GetLeft();
 			m_rRect.SetLeft( m_rRect.GetBottom() );
 			m_rRect.SetBottom( temp );
 		}
 		break;
        
 		default:
            break;
 	}
 
 	// Build matrix for rotation and cropping
 	double alpha = -rotation / 360.0 * 2.0 * PI;
    
 	double a, b, c, d, e, f;
    
 	a = cos( alpha );
 	b = sin( alpha );
 	c = -sin( alpha );
 	d = cos( alpha );
 
 	switch ( rotation )
 	{
 		case 90:
 			e = - m_rRect.GetLeft();
			f = m_rRect.GetBottom() + m_rRect.GetHeight();
            break;
  
  		case 180:
 			e = m_rRect.GetLeft() + m_rRect.GetWidth();
 			f = m_rRect.GetBottom() + m_rRect.GetHeight();
            break;
            
 		case 270:
 			e = m_rRect.GetLeft() + m_rRect.GetWidth();
 			f = - m_rRect.GetBottom();
            break;
 
 		case 0:
 		default:
 			e = - m_rRect.GetLeft();
 			f = - m_rRect.GetBottom();
            break;
 	}

    PdfArray      matrix;
    matrix.push_back( PdfVariant( a ) );
    matrix.push_back( PdfVariant( b ) );
    matrix.push_back( PdfVariant( c ) );
    matrix.push_back( PdfVariant( d ) );
    matrix.push_back( PdfVariant( e ) );
    matrix.push_back( PdfVariant( f ) );
    
    this->GetObject()->GetDictionary().AddKey( "Matrix", matrix );
}

PdfXObject::PdfXObject( PdfObject* pObject )
    : PdfElement( "XObject", pObject ), PdfCanvas(), m_pResources( NULL )
{
    ostringstream out;
    PdfLocaleImbue(out);
    // Implementation note: the identifier is always
    // Prefix+ObjectNo. Prefix is /XOb for XObject.
    out << "XOb" << this->GetObject()->Reference().ObjectNumber();

    
    m_pResources = pObject->GetIndirectKey( "Resources" );
    m_Identifier = PdfName( out.str().c_str() );
    m_Reference  = this->GetObject()->Reference();

    if( this->GetObject()->GetIndirectKey( "BBox" ) )
        m_rRect = PdfRect( this->GetObject()->GetIndirectKey( "BBox" )->GetArray() );
}

void PdfXObject::InitXObject( const PdfRect & rRect, const char* pszPrefix )
{
    PdfVariant    var;
    ostringstream out;
    PdfLocaleImbue(out);

    // Initialize static data
    if( m_matrix.empty() )
    {
        // This matrix is the same for all PdfXObjects so cache it
        m_matrix.push_back( PdfVariant( static_cast<pdf_int64>(PODOFO_LL_LITERAL(1)) ) );
        m_matrix.push_back( PdfVariant( static_cast<pdf_int64>(PODOFO_LL_LITERAL(0)) ) );
        m_matrix.push_back( PdfVariant( static_cast<pdf_int64>(PODOFO_LL_LITERAL(0)) ) );
        m_matrix.push_back( PdfVariant( static_cast<pdf_int64>(PODOFO_LL_LITERAL(1)) ) );
        m_matrix.push_back( PdfVariant( static_cast<pdf_int64>(PODOFO_LL_LITERAL(0)) ) );
        m_matrix.push_back( PdfVariant( static_cast<pdf_int64>(PODOFO_LL_LITERAL(0)) ) );
    }

    rRect.ToVariant( var );
    this->GetObject()->GetDictionary().AddKey( "BBox", var );
    this->GetObject()->GetDictionary().AddKey( PdfName::KeySubtype, PdfName("Form") );
    this->GetObject()->GetDictionary().AddKey( "FormType", PdfVariant( static_cast<pdf_int64>(PODOFO_LL_LITERAL(1)) ) ); // only 1 is only defined in the specification.
    this->GetObject()->GetDictionary().AddKey( "Matrix", m_matrix );

    // The PDF specification suggests that we send all available PDF Procedure sets
    this->GetObject()->GetDictionary().AddKey( "Resources", PdfObject( PdfDictionary() ) );
    m_pResources = this->GetObject()->GetDictionary().GetKey( "Resources" );
    m_pResources->GetDictionary().AddKey( "ProcSet", PdfCanvas::GetProcSet() );

    // Implementation note: the identifier is always
    // Prefix+ObjectNo. Prefix is /XOb for XObject.
	if ( pszPrefix == NULL )
	    out << "XOb" << this->GetObject()->Reference().ObjectNumber();
	else
	    out << pszPrefix << this->GetObject()->Reference().ObjectNumber();

    m_Identifier = PdfName( out.str().c_str() );
    m_Reference  = this->GetObject()->Reference();
}

PdfXObject::PdfXObject( const char* pszSubType, PdfDocument* pParent, const char* pszPrefix )
    : PdfElement( "XObject", pParent ), m_pResources( NULL )
{
    ostringstream out;
    PdfLocaleImbue(out);
    // Implementation note: the identifier is always
    // Prefix+ObjectNo. Prefix is /XOb for XObject.
	if ( pszPrefix == NULL )
	    out << "XOb" << this->GetObject()->Reference().ObjectNumber();
	else
	    out << pszPrefix << this->GetObject()->Reference().ObjectNumber();

    m_Identifier = PdfName( out.str().c_str() );
    m_Reference  = this->GetObject()->Reference();

    this->GetObject()->GetDictionary().AddKey( PdfName::KeySubtype, PdfName( pszSubType ) );
}

PdfXObject::PdfXObject( const char* pszSubType, PdfVecObjects* pParent, const char* pszPrefix )
    : PdfElement( "XObject", pParent ), m_pResources( NULL )
{
    ostringstream out;
    PdfLocaleImbue(out);
    // Implementation note: the identifier is always
    // Prefix+ObjectNo. Prefix is /XOb for XObject.
	if ( pszPrefix == NULL )
	    out << "XOb" << this->GetObject()->Reference().ObjectNumber();
	else
	    out << pszPrefix << this->GetObject()->Reference().ObjectNumber();

    m_Identifier = PdfName( out.str().c_str() );
    m_Reference  = this->GetObject()->Reference();

    this->GetObject()->GetDictionary().AddKey( PdfName::KeySubtype, PdfName( pszSubType ) );
}

PdfXObject::PdfXObject( const char* pszSubType, PdfObject* pObject )
    : PdfElement( "XObject", pObject ), m_pResources( NULL ) 
{
    ostringstream out;
    PdfLocaleImbue(out);

    if( this->GetObject()->GetIndirectKeyAsName( PdfName::KeySubtype ) != pszSubType ) 
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
    }

    // Implementation note: the identifier is always
    // Prefix+ObjectNo. Prefix is /XOb for XObject.
    out << "XOb" << this->GetObject()->Reference().ObjectNumber();

    m_Identifier = PdfName( out.str().c_str() );
    m_Reference  = this->GetObject()->Reference();
}

};
