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

#ifndef _NAME_TEST_H_
#define _NAME_TEST_H_

#include <cppunit/extensions/HelperMacros.h>

/** This test tests the class PdfName
 */
class NameTest : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( NameTest );
  CPPUNIT_TEST( testParseAndWrite );
  CPPUNIT_TEST( testNameEncoding );
  CPPUNIT_TEST( testEncodedNames );
  CPPUNIT_TEST( testEquality );
  CPPUNIT_TEST( testWrite );
  CPPUNIT_TEST( testFromEscaped );
  CPPUNIT_TEST_SUITE_END();

 public:
  void setUp();
  void tearDown();

  void testParseAndWrite();
  void testNameEncoding();
  void testEncodedNames();
  void testEquality();
  void testWrite();
  void testFromEscaped();

 private:

  void TestName( const char* pszString, const char* pszExpectedEncoded );
  void TestEncodedName( const char* pszString, const char* pszExpected );

  /** Tests if both names are equal
   */
  void TestNameEquality( const char * pszName1, const char* pszName2 );

  /** Test if pszName interpreted as PdfName and written
   *  to a PdfOutputDevice equals pszResult
   */
  void TestWrite( const char * pszName, const char* pszResult );

  void TestFromEscape( const char* pszName1, const char* pszName2 );
};

#endif // _NAME_TEST_H_


