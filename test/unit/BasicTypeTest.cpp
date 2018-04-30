/***************************************************************************
 *   Copyright (C) 2008 by Dominik Seichter                                *
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

#include "BasicTypeTest.h"
#include <podofo.h>

#include <limits>

using namespace PoDoFo;

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( BasicTypeTest );

void BasicTypeTest::setUp()
{
}

void BasicTypeTest::tearDown()
{
}

void BasicTypeTest::testXrefOffsetTypeSize() 
{
    CPPUNIT_ASSERT_MESSAGE("pdf_uint64 is big enough to hold an xref entry", std::numeric_limits<pdf_uint64>::max() >= PODOFO_ULL_LITERAL(9999999999) );
}

void BasicTypeTest::testDefaultMaximumNumberOfObjects()
{
    CPPUNIT_ASSERT_EQUAL_MESSAGE("PdfReference allows 8,388,607 indirect objects.", 8388607L, PdfParser::GetMaxObjectCount() );
}
