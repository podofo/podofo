/***************************************************************************
 *   Copyright (C) 2010 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "colorspace.h"

using namespace PoDoFo;

ColorSpace::ColorSpace()
    : m_name("DeviceGray")
{
}
    
ColorSpace::ColorSpace(const PdfName & rName)
    : m_name(rName)
{
}
    
ColorSpace::ColorSpace(const ColorSpace & rhs)
{
    this->operator=(rhs);
}

ColorSpace::~ColorSpace()
{
}
    
const ColorSpace & ColorSpace::operator=(const ColorSpace & rhs)
{
    m_name = rhs.m_name;

    return *this;
}

bool ColorSpace::IsSimpleColorSpace() const
{
    EPdfColorSpace eColorSpace = this->ConvertToPdfColorSpace();

    if( eColorSpace == ePdfColorSpace_DeviceGray
        || eColorSpace == ePdfColorSpace_DeviceRGB
        || eColorSpace == ePdfColorSpace_DeviceCMYK )
    {
        return true;
    }
    else
    {
        return false;
    }
}

EPdfColorSpace ColorSpace::ConvertToPdfColorSpace() const
{
    return PdfColor::GetColorSpaceForName(m_name);
}
