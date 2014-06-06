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

#include "PdfRect.h"

#include "PdfArray.h"
#include "PdfVariant.h"
#include "PdfDefinesPrivate.h"

#include <iostream>
#include <sstream>
#include <iomanip>

namespace PoDoFo {

PdfRect::PdfRect()
{
    m_dBottom = m_dLeft = m_dWidth = m_dHeight = 0;
}

PdfRect::PdfRect( double dLeft, double dBottom, double dWidth, double dHeight )
{
    m_dBottom = dBottom;
    m_dLeft   = dLeft;
    m_dWidth  = dWidth;
    m_dHeight = dHeight;
}

PdfRect::PdfRect( const PdfArray& inArray )
{
    m_dBottom = m_dLeft = m_dWidth = m_dHeight = 0;
    FromArray( inArray );
}

PdfRect::PdfRect( const PdfRect & rhs )
{
    this->operator=( rhs );
}

void PdfRect::ToVariant( PdfVariant & var ) const
{
    PdfArray array;
    
    array.push_back( PdfVariant( m_dLeft ) );
    array.push_back( PdfVariant( m_dBottom ) );
    array.push_back( PdfVariant( (m_dWidth+m_dLeft) ) );
    array.push_back( PdfVariant( (m_dHeight+m_dBottom) ) );

    var = array;
}

std::string PdfRect::ToString() const
{
    PdfVariant  var;
    std::string str;
    this->ToVariant( var );
    var.ToString( str );

    return str;

    /*
    std::ostringstream	oStr;
    oStr << "[ ";
    oStr << std::setprecision( 3 ) << m_dLeft << " ";
    oStr << std::setprecision( 3 ) << m_dBottom << " ";
    oStr << std::setprecision( 3 ) << m_dWidth + m_dLeft << " ";
    oStr << std::setprecision( 3 ) << m_dHeight - m_dBottom << " ]";
    
    return oStr.str();
    */
}

void PdfRect::FromArray( const PdfArray& inArray )
{
    if ( inArray.size() == 4 ) 
    {
        m_dLeft   = inArray[0].GetReal();
        m_dBottom = inArray[1].GetReal();
        m_dWidth  = inArray[2].GetReal() - m_dLeft;
        m_dHeight = inArray[3].GetReal() - m_dBottom;
    }
    else 
    {
        PODOFO_RAISE_ERROR( ePdfError_ValueOutOfRange );
    }
}

void PdfRect::Intersect( const PdfRect & rRect )
{
	if( rRect.GetBottom() != 0 || rRect.GetHeight() != 0 || rRect.GetLeft() != 0 || rRect.GetWidth() != 0 )
	{
		double diff;
		
		diff = rRect.m_dLeft - m_dLeft;
		if ( diff > 0.0 )
		{
			m_dLeft += diff;
			m_dWidth -= diff;
		}

		diff = (m_dLeft + m_dWidth) - (rRect.m_dLeft + rRect.m_dWidth);
		if ( diff > 0.0 )
		{
			m_dWidth -= diff;
		}

		diff = rRect.m_dBottom - m_dBottom;
		if ( diff > 0.0 )
		{
			m_dBottom += diff;
			m_dHeight -= diff;
		}

		diff = (m_dBottom + m_dHeight) - (rRect.m_dBottom + rRect.m_dHeight);
		if ( diff > 0.0 )
		{
			m_dHeight -= diff;
		}
	}
}

PdfRect & PdfRect::operator=( const PdfRect & rhs )
{
    this->m_dBottom = rhs.m_dBottom;
    this->m_dLeft   = rhs.m_dLeft;
    this->m_dWidth  = rhs.m_dWidth;
    this->m_dHeight = rhs.m_dHeight;

    return *this;
}

};
