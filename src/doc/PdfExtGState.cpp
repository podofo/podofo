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

#include "PdfExtGState.h"

#include "base/PdfDefinesPrivate.h"

#include "base/PdfDictionary.h"
#include "base/PdfWriter.h"
#include "base/PdfLocale.h"

#include "PdfPage.h"

#include <sstream>

namespace PoDoFo {

PdfExtGState::PdfExtGState( PdfVecObjects* pParent )
    : PdfElement( "ExtGState", pParent )
{
    std::ostringstream out;
    // We probably aren't doing anything locale sensitive here, but it's
    // best to be sure.
    PdfLocaleImbue(out);

    // Implementation note: the identifier is always
    // Prefix+ObjectNo. Prefix is /Ft for fonts.
    out << "ExtGS" << this->GetObject()->Reference().ObjectNumber();
    m_Identifier = PdfName( out.str().c_str() );

    this->Init();
}

PdfExtGState::PdfExtGState( PdfDocument* pParent )
    : PdfElement( "ExtGState", pParent )
{
    std::ostringstream out;
    // We probably aren't doing anything locale sensitive here, but it's
    // best to be sure.
    PdfLocaleImbue(out);

    // Implementation note: the identifier is always
    // Prefix+ObjectNo. Prefix is /Ft for fonts.
    out << "ExtGS" << this->GetObject()->Reference().ObjectNumber();
    m_Identifier = PdfName( out.str().c_str() );

    this->Init();
}

PdfExtGState::~PdfExtGState()
{
}

void PdfExtGState::Init( void )
{
}

void PdfExtGState::SetFillOpacity( float opac )
{
    this->GetObject()->GetDictionary().AddKey( "ca", PdfVariant( static_cast<double>(opac) ) );
}

void PdfExtGState::SetStrokeOpacity( float opac )
{
    this->GetObject()->GetDictionary().AddKey( "CA", PdfVariant( opac ) );
}

void PdfExtGState::SetBlendMode( char* blendMode )
{
    this->GetObject()->GetDictionary().AddKey( "BM", PdfVariant( PdfName( blendMode ) ) );
}

void PdfExtGState::SetOverprint( bool enable )
{
    this->GetObject()->GetDictionary().AddKey( "OP", PdfVariant( enable ) );
}

void PdfExtGState::SetFillOverprint( bool enable )
{
    this->GetObject()->GetDictionary().AddKey( "op", PdfVariant( enable ) );
}

void PdfExtGState::SetStrokeOverprint( bool enable )
{
    this->GetObject()->GetDictionary().AddKey( "OP", PdfVariant( enable ) );
}

void PdfExtGState::SetNonZeroOverprint( bool enable )
{
    this->GetObject()->GetDictionary().AddKey( "OPM", PdfVariant( static_cast<pdf_int64>(enable ? 1LL : 0LL) ) );
}

void PdfExtGState::SetRenderingIntent( char* intent )
{
    this->GetObject()->GetDictionary().AddKey( "RI", PdfVariant( PdfName( intent ) ) );
}

void PdfExtGState::SetFrequency( double frequency )
{
	PdfDictionary halftoneDict;
	halftoneDict.AddKey( "HalftoneType", PdfVariant( static_cast<pdf_int64>(1LL) ) );
	halftoneDict.AddKey( "Frequency", PdfVariant( frequency ) );
	halftoneDict.AddKey( "Angle", PdfVariant( 45.0 ) );
	halftoneDict.AddKey( "SpotFunction", PdfName( "SimpleDot" ) );

    this->GetObject()->GetDictionary().AddKey( "HT", halftoneDict);
}


}	// end namespace
