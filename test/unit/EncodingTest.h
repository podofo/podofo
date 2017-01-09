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

#ifndef _ENCODING_TEST_H_
#define _ENCODING_TEST_H_

#include <cppunit/extensions/HelperMacros.h>

namespace PoDoFo {
  class PdfEncoding;
};

/** This test tests the various class PdfEncoding classes
 */
class EncodingTest : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( EncodingTest );
  CPPUNIT_TEST( testDifferences );
  CPPUNIT_TEST( testDifferencesEncoding );
  CPPUNIT_TEST( testDifferencesObject );
  CPPUNIT_TEST( testUnicodeNames );
  CPPUNIT_TEST( testGetCharCode );
  CPPUNIT_TEST( testToUnicodeParse );
  CPPUNIT_TEST_SUITE_END();

 public:
  void setUp();
  void tearDown();

  void testDifferences();
  void testDifferencesObject();
  void testDifferencesEncoding();
  void testUnicodeNames();
  void testGetCharCode();
  void testToUnicodeParse();

 private:

  bool outofRangeHelper( PoDoFo::PdfEncoding* pEncoding, std::string & rMsg, const char* pszName );
};

#endif // _STRING_TEST_H_


