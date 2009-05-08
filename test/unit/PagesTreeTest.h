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

#ifndef _PAGES_TREE_TEST_H_
#define _PAGES_TREE_TEST_H_

#include <cppunit/extensions/HelperMacros.h>

namespace PoDoFo {
class PdfMemDocument;
class PdfPage;
};

/** This test tests the class PdfPagesTree
 */
class PagesTreeTest : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( PagesTreeTest );
  CPPUNIT_TEST( testEmptyTree );
  CPPUNIT_TEST( testEmptyDoc );
  CPPUNIT_TEST( testCreateDelete );
  CPPUNIT_TEST( testGetPagesCustom );
  CPPUNIT_TEST( testGetPagesPoDoFo );
  CPPUNIT_TEST( testGetPagesReverseCustom );
  CPPUNIT_TEST( testGetPagesReversePoDoFo );
  CPPUNIT_TEST( testInsertCustom );
  CPPUNIT_TEST( testInsertPoDoFo );
  CPPUNIT_TEST( testDeleteAllCustom );
  CPPUNIT_TEST( testDeleteAllPoDoFo );
  CPPUNIT_TEST_SUITE_END();

 public:
  void setUp();
  void tearDown();

  void testEmptyTree();
  void testEmptyDoc();
  void testCreateDelete();
  void testGetPagesCustom();
  void testGetPagesPoDoFo();
  void testGetPagesReverseCustom();
  void testGetPagesReversePoDoFo();
  void testInsertCustom();
  void testInsertPoDoFo();
  void testDeleteAllCustom();
  void testDeleteAllPoDoFo();
    
 private:
  void testGetPages( PoDoFo::PdfMemDocument & doc );
  void testGetPagesReverse( PoDoFo::PdfMemDocument & doc );
  void testInsert( PoDoFo::PdfMemDocument & doc );
  void testDeleteAll( PoDoFo::PdfMemDocument & doc );

  /**
   * Create a pages tree with 100 pages,
   * where every page object has an additional
   * key PoDoFoTestPageNumber with the original 
   * page number of the page.
   *
   * This method uses PoDoFo's build in PdfPagesTree
   * which creates a flat tree.
   *
   * You can check the page number ussing IsPageNumber()
   *
   * @see IsPageNumber
   */
  void CreateTestTreePoDoFo( PoDoFo::PdfMemDocument & rDoc );

  /**
   * Create a pages tree with 100 pages,
   * where every page object has an additional
   * key PoDoFoTestPageNumber with the original 
   * page number of the page.
   *
   * This builds a pages tree manually an makes
   * sure a real tree structure is build.
   *
   * You can check the page number ussing IsPageNumber()
   *
   * @see IsPageNumber
   */
  void CreateTestTreeCustom( PoDoFo::PdfMemDocument & rDoc );

  bool IsPageNumber( PoDoFo::PdfPage* pPage, int nNumber );
};

#endif // _PAGES_TREE_TEST_H_


