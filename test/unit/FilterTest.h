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

#ifndef _FILTER_TEST_H_
#define _FILTER_TEST_H_

#include <cppunit/extensions/HelperMacros.h>

#include <podofo.h>

/** This test tests the various PdfFilter classes
 */
class FilterTest : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( FilterTest );
  CPPUNIT_TEST( testFilters );
  CPPUNIT_TEST( testCCITT );
  CPPUNIT_TEST_SUITE_END();

 public:
  void setUp();
  void tearDown();

  void testFilters();

  void testCCITT();

 private:
  void TestFilter( PoDoFo::EPdfFilter eFilter, const char * pTestBuffer, const long lTestLength );
};

#endif // _FILTER_TEST_H_


