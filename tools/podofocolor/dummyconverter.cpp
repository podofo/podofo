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

#include "dummyconverter.h"

using namespace PoDoFo;

DummyConverter::DummyConverter()
    : IConverter()
{

}

DummyConverter::~DummyConverter()
{
}

void DummyConverter::StartPage( PdfPage*, int )
{
}

void DummyConverter::EndPage( PdfPage*, int )
{
}

void DummyConverter::StartXObject( PdfXObject* )
{
}

void DummyConverter::EndXObject( PdfXObject* )
{
}

PdfColor DummyConverter::SetStrokingColorGray( const PdfColor & )
{
    return PdfColor( 1.0, 0.0, 0.0 );
}

PdfColor DummyConverter::SetStrokingColorRGB( const PdfColor & )
{
    return PdfColor( 1.0, 0.0, 0.0 );
}

PdfColor DummyConverter::SetStrokingColorCMYK( const PdfColor & )
{
    return PdfColor( 1.0, 0.0, 0.0 );
}

PdfColor DummyConverter::SetNonStrokingColorGray( const PdfColor & )
{
    return PdfColor( 0.0, 1.0, 0.0 );
}

PdfColor DummyConverter::SetNonStrokingColorRGB( const PdfColor & )
{
    return PdfColor( 0.0, 1.0, 0.0 );
}

PdfColor DummyConverter::SetNonStrokingColorCMYK( const PdfColor & )
{
    return PdfColor( 0.0, 1.0, 0.0 );
}
