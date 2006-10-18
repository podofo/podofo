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

#include <sstream>

namespace PoDoFo {

	PdfExtGState::PdfExtGState( PdfVecObjects* pParent )
		: PdfElement( "ExtGState", pParent )
	{
		std::ostringstream out;

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
		m_pObject->GetDictionary().AddKey( "ca", PdfVariant( opac ) );
	}

	void PdfExtGState::SetStrokeOpacity( float opac )
	{
		m_pObject->GetDictionary().AddKey( "CA", PdfVariant( opac ) );
	}


}	// end namespace
