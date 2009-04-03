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

#include "PdfDictionary.h"
#include "PdfPage.h"
#include "PdfWriter.h"
#include "PdfLocale.h"

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
    out << "ExtGS" << m_pObject->Reference().ObjectNumber();
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
    out << "ExtGS" << m_pObject->Reference().ObjectNumber();
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
    m_pObject->GetDictionary().AddKey( "ca", PdfVariant( static_cast<double>(opac) ) );
}

void PdfExtGState::SetStrokeOpacity( float opac )
{
    m_pObject->GetDictionary().AddKey( "CA", PdfVariant( opac ) );
}

void PdfExtGState::SetBlendMode( char* blendMode )
{
    m_pObject->GetDictionary().AddKey( "BM", PdfVariant( PdfName( blendMode ) ) );
}

void PdfExtGState::SetOverprint( bool enable )
{
    m_pObject->GetDictionary().AddKey( "OP", PdfVariant( enable ) );
}

void PdfExtGState::SetFillOverprint( bool enable )
{
    m_pObject->GetDictionary().AddKey( "op", PdfVariant( enable ) );
}

void PdfExtGState::SetStrokeOverprint( bool enable )
{
    m_pObject->GetDictionary().AddKey( "OP", PdfVariant( enable ) );
}

void PdfExtGState::SetNonZeroOverprint( bool enable )
{
    m_pObject->GetDictionary().AddKey( "OPM", PdfVariant( (enable ? 1LL : 0LL) ) );
}

void PdfExtGState::SetRenderingIntent( char* intent )
{
    m_pObject->GetDictionary().AddKey( "RI", PdfVariant( PdfName( intent ) ) );
}


}	// end namespace
