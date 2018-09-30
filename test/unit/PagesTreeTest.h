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

#include <vector>

#include <cppunit/extensions/HelperMacros.h>

namespace PoDoFo {
class PdfMemDocument;
class PdfPage;
class PdfObject;
};

/** This test tests the class PdfPagesTree
 */
class PagesTreeTest : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( PagesTreeTest );
  CPPUNIT_TEST( testEmptyTree );
  CPPUNIT_TEST( testEmptyDoc );
  CPPUNIT_TEST( testCyclicTree );
  CPPUNIT_TEST( testEmptyKidsTree );
  CPPUNIT_TEST( testNestedArrayTree );
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
  void testCyclicTree();
  void testEmptyKidsTree();
  void testNestedArrayTree();
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

  /**
   * Create a pages tree with cycles to test prevention of endless
   * recursion as mentioned in different CVE reports.
   *
   * \param bCreateCycle if true a cyclic tree is created, otherwise a
   *                     valid tree without cycles
   */
  void CreateCyclicTree( PoDoFo::PdfMemDocument & rDoc,
                         bool bCreateCycle );

  /**
   * Create a pages tree with nodes containing empty kids.
   *
   * This is completely valid according to the PDF spec, i.e. the
   * required keys may have the values "/Kids [ ]" and "/Count 0"
   * Such a tree must still be parsable by a conforming reader:
   *
   * <BLOCKQUOTE>The tree contains nodes of two types—intermediate
   * nodes, called page tree nodes, and leaf nodes, called page
   * objects—whose form is described in the subsequent subclauses.
   * Conforming products shall be prepared to handle any form
   * of tree structure built of such nodes.</BLOCKQUOTE>
   */
  void CreateEmptyKidsTree( PoDoFo::PdfMemDocument & rDoc );
  
  /**
  * Ceate a pages tree with a nested kids array.
  *
  * Such a tree is not valid to the PDF spec, which requires they key
  * "Kids" to be an array of indirect references. And the children shall
  * only be page objects or other page tree nodes.
  */
  void CreateNestedArrayTree( PoDoFo::PdfMemDocument & rDoc );

 /**
  * Create page object nodes (leaf nodes),
  * where every page object has an additional
  * key PoDoFoTestPageNumber with the original 
  * page number of the page.
  */  
  std::vector<PoDoFo::PdfPage*> CreateSamplePages( PoDoFo::PdfMemDocument & rDoc,
                                                   int nPageCount);

  /**
  * Create page tree nodes (internal nodes)
  */
  std::vector<PoDoFo::PdfObject*> CreateNodes( PoDoFo::PdfMemDocument & rDoc,
                                               int nNodeCount);

  bool IsPageNumber( PoDoFo::PdfPage* pPage, int nNumber );

  void AppendChildNode(PoDoFo::PdfObject* pParent, PoDoFo::PdfObject* pChild);
};

#endif // _PAGES_TREE_TEST_H_


