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

#include "PdfPage.h"
#include "PdfVariant.h"

#include <sstream>

using namespace std;

namespace PoDoFo {

PdfRect::PdfRect()
{
    m_lTop = m_lLeft = m_lWidth = m_lHeight = 0;
}

PdfRect::PdfRect( long lLeft, long lTop, long lWidth, long lHeight )
{
    m_lTop    = lTop;
    m_lLeft   = lLeft;
    m_lWidth  = lWidth;
    m_lHeight = lHeight;
}

PdfRect::PdfRect( const PdfRect & rhs )
{
    this->operator=( rhs );
}

void PdfRect::ToVariant( PdfVariant & var, PdfPage* pPage ) const
{
    ostringstream oStream;
    long lTop = pPage ? (pPage->PageSize().lHeight - m_lTop) : m_lTop;

    oStream << "[ " << 
        CONVERSION_CONSTANT * m_lLeft << " " <<
        CONVERSION_CONSTANT * (lTop+m_lHeight) << " " <<
        CONVERSION_CONSTANT * (m_lLeft+m_lWidth) << " " <<
        CONVERSION_CONSTANT * lTop << " ]";

    var.Init( oStream.str().c_str(), ePdfDataType_Array );
}

PdfRect & PdfRect::operator=( const PdfRect & rhs )
{
    this->m_lTop    = rhs.m_lTop;
    this->m_lLeft   = rhs.m_lLeft;
    this->m_lWidth  = rhs.m_lWidth;
    this->m_lHeight = rhs.m_lHeight;

    return *this;
}

};
