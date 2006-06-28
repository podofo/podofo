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

#include "PdfRect.h"

#include "PdfArray.h"
#include "PdfPage.h"
#include "PdfVariant.h"

#include <iostream>
#include <sstream>
#include <iomanip>

namespace PoDoFo {

PdfRect::PdfRect()
{
    m_lBottom = m_lLeft = m_lWidth = m_lHeight = 0;
}

PdfRect::PdfRect( double lLeft, double lBottom, double lWidth, double lHeight )
{
    m_lBottom = lBottom;
    m_lLeft   = lLeft;
    m_lWidth  = lWidth;
    m_lHeight = lHeight;
}

PdfRect::PdfRect( PdfArray& inArray )
{
	m_lBottom = m_lLeft = m_lWidth = m_lHeight = 0;
	FromArray( inArray );
}

PdfRect::PdfRect( const PdfRect & rhs )
{
    this->operator=( rhs );
}

void PdfRect::ToVariant( PdfVariant & var, PdfPage* pPage ) const
{
    PdfArray array;
    
    array.push_back( PdfVariant( m_lLeft ) );
    array.push_back( PdfVariant( m_lBottom ) );
    array.push_back( PdfVariant( (m_lWidth-m_lLeft) ) );
    array.push_back( PdfVariant( (m_lBottom+m_lHeight) ) );

    var = array;
}

std::string PdfRect::ToString() const
{
	std::ostringstream	oStr;
	oStr << "[ ";
	oStr << std::setprecision( 3 ) << m_lLeft << " ";
	oStr << std::setprecision( 3 ) << m_lBottom << " ";
	oStr << std::setprecision( 3 ) << m_lWidth + m_lLeft << " ";
	oStr << std::setprecision( 3 ) << m_lHeight - m_lBottom << " ]";

	return oStr.str();
}

void PdfRect::FromArray( const PdfArray& inArray )
{
	if ( inArray.size() == 4 ) 
	{
		m_lLeft = inArray[0].GetReal();
		m_lBottom = inArray[1].GetReal();
		m_lWidth = inArray[2].GetReal() - m_lLeft;
		m_lHeight = inArray[3].GetReal() + m_lBottom;
	} else 
	{
		// TODO: throw an error
	}
}

PdfRect & PdfRect::operator=( const PdfRect & rhs )
{
    this->m_lBottom = rhs.m_lBottom;
    this->m_lLeft   = rhs.m_lLeft;
    this->m_lWidth  = rhs.m_lWidth;
    this->m_lHeight = rhs.m_lHeight;

    return *this;
}

};
