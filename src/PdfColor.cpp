/***************************************************************************
 *   Copyright (C) 2007 by Dominik Seichter                                *
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

#include "PdfColor.h"

namespace PoDoFo {

static inline void CheckDoubleRange( double val, double min, double max )
{
    if( val < min || val > max )
    {
        PODOFO_RAISE_ERROR( ePdfError_ValueOutOfRange );
    }
}

PdfColor::PdfColor( double dGray )
    : m_eColorSpace( ePdfColorSpace_DeviceGray )
{
    CheckDoubleRange( dGray, 0.0, 1.0 );

    m_uColor.gray = dGray;
}

PdfColor::PdfColor( double dRed, double dGreen, double dBlue )
    : m_eColorSpace( ePdfColorSpace_DeviceRGB )
{
    CheckDoubleRange( dRed,   0.0, 1.0 );
    CheckDoubleRange( dGreen, 0.0, 1.0 );
    CheckDoubleRange( dBlue,  0.0, 1.0 );

    m_uColor.rgb[0] = dRed;
    m_uColor.rgb[1] = dGreen;
    m_uColor.rgb[2] = dBlue;
}

PdfColor::PdfColor( double dCyan, double dMagenta, double dYellow, double dBlack )
    : m_eColorSpace( ePdfColorSpace_DeviceCMYK )
{
    CheckDoubleRange( dCyan,    0.0, 1.0 );
    CheckDoubleRange( dMagenta, 0.0, 1.0 );
    CheckDoubleRange( dYellow,  0.0, 1.0 );
    CheckDoubleRange( dBlack,   0.0, 1.0 );

    m_uColor.cmyk[0] = dCyan;
    m_uColor.cmyk[1] = dMagenta;
    m_uColor.cmyk[2] = dYellow;
    m_uColor.cmyk[3] = dBlack;
}

PdfColor::PdfColor( const PdfColor & rhs )
{
    this->operator=( rhs );
}

const PdfColor & PdfColor::operator=( const PdfColor & rhs )
{
    m_eColorSpace = rhs.m_eColorSpace;
    memcpy( &m_uColor, &rhs.m_uColor, sizeof(m_uColor) );

    return *this;
}

};

