/***************************************************************************
 *   Copyright (C) 2011 by Dominik Seichter                                *
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

#ifndef _PAINTER_TEST_H_
#define _PAINTER_TEST_H_

#include <cppunit/extensions/HelperMacros.h>

namespace PoDoFo {
class PdfPage;
class PdfStream;
};

/** This test tests the class PdfPainter
 */
class PainterTest : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( PainterTest );
  CPPUNIT_TEST( testAppend );
  CPPUNIT_TEST_SUITE_END();

 public:
  void setUp();
  void tearDown();

  /** 
   * Test if contents are appended correctly
   * to pages with existing contents.
   */
  void testAppend();

 private:
  /**
   * Compare the filtered contents of a PdfStream object
   * with a string and assert if the contents do not match!
   *
   * @param pStream PdfStream object
   * @param pszContent expected contents
   */
  void  CompareStreamContent(PoDoFo::PdfStream* pStream, const char* pszExpected);
};

#endif // _PAINTER_TEST_H_
