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

#ifndef _STRING_TEST_H_
#define _STRING_TEST_H_

#include <cppunit/extensions/HelperMacros.h>

/** This test tests the class PdfString
 */
class StringTest : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( StringTest );
  CPPUNIT_TEST( testGetStringUtf8 );
  CPPUNIT_TEST( testUtf16beContructor );
  CPPUNIT_TEST_SUITE_END();

 public:
  void setUp();
  void tearDown();

  void testGetStringUtf8();
  void testUtf16beContructor();

 private:

};

#endif // _STRING_TEST_H_


