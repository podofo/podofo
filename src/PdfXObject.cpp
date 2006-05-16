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

#include "PdfImage.h"
#include "PdfRect.h"
#include "PdfVariant.h"

#include <sstream>

using namespace std;

namespace PoDoFo {

PdfXObject::PdfXObject( unsigned int nObjectNo, unsigned int nGenerationNo )
    : PdfObject( nObjectNo, nGenerationNo, "XObject" ), PdfCanvas()
{
    m_pResources = new PdfObject( 0, 0, NULL );
    m_pResources->SetDirect( true );

    this->AddKey( PdfName::KeySubtype, PdfName("Form") );
    this->AddKey( "FormType", (long)1 ); // only 1 is only defined in the specification.
                                      // it is required though.
    this->AddKey( "Resources", m_pResources );

    // The PDF specification suggests that we send all available PDF Procedure sets
    m_pResources->AddKey( "ProcSet", "[/PDF /Text /ImageB /ImageC /ImageI]" );

    m_size.lWidth  = 0;
    m_size.lHeight = 0;
}

PdfError PdfXObject::Init( const PdfRect & rRect )
{
    PdfError   eCode;

    PdfVariant var;
    rRect.ToVariant( var );

    this->AddKey( "BBox", var );
    this->AddKey( "Matrix", "[ 1 0 0 1 0 0 ]" );

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
