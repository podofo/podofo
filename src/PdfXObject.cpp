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
#include "PdfVariant.h"

#include <sstream>

using namespace std;

namespace PoDoFo {

PdfArray PdfXObject::s_matrix;

PdfXObject::PdfXObject( unsigned int nObjectNo, unsigned int nGenerationNo )
    : PdfObject( nObjectNo, nGenerationNo, "XObject" ), PdfCanvas()
{
    PdfDictionary resources;

    this->AddKey( PdfName::KeySubtype, PdfName("Form") );
    this->AddKey( "FormType", (long)1 ); // only 1 is only defined in the specification.


    // The PDF specification suggests that we send all available PDF Procedure sets
    this->AddKey( "Resources", PdfVariant( resources ) );
    m_pResources = &(GetVariant().GetDictionary().GetKey( "Resources" ).GetDictionary());
    Resources()->AddKey( "ProcSet", PdfCanvas::ProcSet() );

    m_size.lWidth  = 0;
    m_size.lHeight = 0;
}

PdfError PdfXObject::Init( const PdfRect & rRect )
{
    PdfError   eCode;

    PdfVariant var;
    rRect.ToVariant( var );
    this->AddKey( "BBox", var );

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

    this->AddKey( "Matrix", s_matrix );

    m_size.lWidth  = rRect.Width();
    m_size.lHeight = rRect.Height();

    return eCode;
}

void PdfXObject::GetImageReference( PdfImageRef & rRef )
{
    ostringstream out;

    // Implementation note: the identifier is always
    // Prefix+ObjectNo. Prefix is /XOb for XObject.
    out << "XOb" << this->ObjectNumber();
    
    // convert 1/1000th mm into pixels
    rRef.SetWidth      ( long(m_size.lWidth  * CONVERSION_CONSTANT) );
    rRef.SetHeight     ( long(m_size.lHeight * CONVERSION_CONSTANT) );

    rRef.SetIdentifier ( PdfName( out.str().c_str() ) );
    rRef.SetReference  ( this->Reference() );
}

};
