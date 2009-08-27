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

#ifndef _VARIANT_TEST_H_
#define _VARIANT_TEST_H_

#include <cppunit/extensions/HelperMacros.h>

/** This test tests the class PdfVariant
 */
class VariantTest : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( VariantTest );
  CPPUNIT_TEST( testEmptyObject );
  CPPUNIT_TEST( testEmptyStream );
  CPPUNIT_TEST( testNameObject );
  CPPUNIT_TEST( testIsDirtyTrue );
  CPPUNIT_TEST( testIsDirtyFalse );
  CPPUNIT_TEST_SUITE_END();

 public:
  void setUp();
  void tearDown();

  void testEmptyObject();
  void testEmptyStream();
  void testNameObject();

  void testIsDirtyTrue();
  void testIsDirtyFalse();

 private:
};

#endif // _VARIANT_TEST_H_


