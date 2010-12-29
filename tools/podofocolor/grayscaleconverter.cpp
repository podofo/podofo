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

#include "grayscaleconverter.h"

GrayscaleConverter::GrayscaleConverter()
    : IConverter()
{
}

GrayscaleConverter::~GrayscaleConverter()
{
}

void GrayscaleConverter::StartPage( PoDoFo::PdfPage*, int )
{
}

void GrayscaleConverter::EndPage( PoDoFo::PdfPage*, int )
{
}

void GrayscaleConverter::StartXObject( PoDoFo::PdfXObject* )
{
}

void GrayscaleConverter::EndXObject( PoDoFo::PdfXObject* )
{
}

PoDoFo::PdfColor GrayscaleConverter::SetStrokingColorGray( const PoDoFo::PdfColor & rColor )
{
    return rColor;
}

PoDoFo::PdfColor GrayscaleConverter::SetStrokingColorRGB( const PoDoFo::PdfColor & rColor )
{
    return rColor.ConvertToGrayScale();
}

PoDoFo::PdfColor GrayscaleConverter::SetStrokingColorCMYK( const PoDoFo::PdfColor & rColor )
{
    return rColor.ConvertToGrayScale();
}
  
PoDoFo::PdfColor GrayscaleConverter::SetNonStrokingColorGray( const PoDoFo::PdfColor & rColor )
{
    return rColor.ConvertToGrayScale();
}

PoDoFo::PdfColor GrayscaleConverter::SetNonStrokingColorRGB( const PoDoFo::PdfColor & rColor )
{
    return rColor.ConvertToGrayScale();
}

PoDoFo::PdfColor GrayscaleConverter::SetNonStrokingColorCMYK( const PoDoFo::PdfColor & rColor )
{
    return rColor.ConvertToGrayScale();
}
