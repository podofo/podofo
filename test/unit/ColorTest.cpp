/***************************************************************************
 *   Copyright (C) 2010 by Dominik Seichter                                *
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

#include "ColorTest.h"
#include <podofo.h>

using namespace PoDoFo;

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( ColorTest );

void ColorTest::setUp()
{
}

void ColorTest::tearDown()
{
}

void ColorTest::testHexNames() 
{
    PdfColor rgb = PdfColor::FromString( "#FF0AEF" );
    CPPUNIT_ASSERT( rgb.IsRGB() );
    CPPUNIT_ASSERT( static_cast<int>(rgb.GetRed() * 255.0) == 0xFF );
    CPPUNIT_ASSERT( static_cast<int>(rgb.GetGreen() * 255.0) == 0x0A );
    CPPUNIT_ASSERT( static_cast<int>(rgb.GetBlue() * 255.0) == 0xEF );


    PdfColor cmyk = PdfColor::FromString( "#ABCDEF01" );
    CPPUNIT_ASSERT( cmyk.IsCMYK() );
    CPPUNIT_ASSERT( static_cast<int>(cmyk.GetCyan() * 255.0) == 0xAB );
    CPPUNIT_ASSERT( static_cast<int>(cmyk.GetMagenta() * 255.0) == 0xCD );
    CPPUNIT_ASSERT( static_cast<int>(cmyk.GetYellow() * 255.0) == 0xEF );
    CPPUNIT_ASSERT( static_cast<int>(cmyk.GetBlack() * 255.0) == 0x01 );
}

void ColorTest::testNames()
{
    //PdfNamedColor( "aliceblue", PdfColor(0.941, 0.973, 1.000, 1.000) ) ,
    PdfColor aliceBlue = PdfColor::FromString( "aliceblue" );
    CPPUNIT_ASSERT( aliceBlue == PdfColor(0.941, 0.973, 1.000, 1.000) );
    CPPUNIT_ASSERT( aliceBlue.GetCyan() == 0.941 );
    CPPUNIT_ASSERT( aliceBlue.GetMagenta() == 0.973 );
    CPPUNIT_ASSERT( aliceBlue.GetYellow() == 1.000 );
    CPPUNIT_ASSERT( aliceBlue.GetBlack() == 1.000 );

    //PdfNamedColor( "lime", PdfColor(0.000, 1.000, 0.000, 1.000) ) ,
    PdfColor lime = PdfColor::FromString( "lime" );
    CPPUNIT_ASSERT( lime == PdfColor(0.000, 1.000, 0.000, 1.000) );

    //PdfNamedColor( "yellowgreen", PdfColor(0.604, 0.804, 0.196, 1.000) ) 
    PdfColor yellowGreen = PdfColor::FromString( "yellowgreen" );
    CPPUNIT_ASSERT( yellowGreen == PdfColor(0.604, 0.804, 0.196, 1.000) );


    // Test a not existing color
    PdfColor notExist = PdfColor::FromString( "asfaf9q341" );
    CPPUNIT_ASSERT( notExist == PdfColor() );
}

