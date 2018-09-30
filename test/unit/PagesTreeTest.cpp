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

#include "PagesTreeTest.h"

#include <podofo.h>

#include <sstream>

#define PODOFO_TEST_PAGE_KEY "PoDoFoTestPageNumber"
#define PODOFO_TEST_NUM_PAGES 100

using namespace PoDoFo;

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( PagesTreeTest );

void PagesTreeTest::setUp()
{
}

void PagesTreeTest::tearDown()
{
}

void PagesTreeTest::testEmptyTree()
{
    PdfMemDocument  writer;

    // Empty document must have page count == 0
    CPPUNIT_ASSERT_EQUAL( writer.GetPageCount(), 0 );

    // Retrieving any page from an empty document must be NULL
    PdfPage* pPage = writer.GetPagesTree()->GetPage( 0 );
    CPPUNIT_ASSERT_EQUAL( pPage, static_cast<PdfPage*>(NULL) );

    pPage = writer.GetPagesTree()->GetPage( -1 );
    CPPUNIT_ASSERT_EQUAL( pPage, static_cast<PdfPage*>(NULL) );

    pPage = writer.GetPagesTree()->GetPage( 1 );
    CPPUNIT_ASSERT_EQUAL( pPage, static_cast<PdfPage*>(NULL) );
}

void PagesTreeTest::testEmptyDoc()
{
    // PdfPagesTree does not throw exceptions but PdfDocument does
    PdfMemDocument  writer;

    // Empty document must have page count == 0
    CPPUNIT_ASSERT_EQUAL( writer.GetPageCount(), 0 );

    // Retrieving any page from an empty document must be NULL
    CPPUNIT_ASSERT_THROW( writer.GetPage( 0 ), PdfError );
    CPPUNIT_ASSERT_THROW( writer.GetPage( -1 ), PdfError );
    CPPUNIT_ASSERT_THROW( writer.GetPage( 1 ), PdfError );
}

void PagesTreeTest::testCyclicTree()
{
    for (int pass=0; pass < 2; pass++)
    {
        PdfMemDocument doc;
        CreateCyclicTree( doc, pass==1);
        //doc.Write(pass==0?"tree_valid.pdf":"tree_cyclic.pdf");
        for (int pagenum=0; pagenum < doc.GetPageCount(); pagenum++)
        {
            if (pass==0)
            {    
                // pass 0:
                // valid tree without cycles should yield all pages
                PdfPage* pPage = doc.GetPage( pagenum );
                CPPUNIT_ASSERT_EQUAL( pPage != NULL, true );
                CPPUNIT_ASSERT_EQUAL( IsPageNumber( pPage, pagenum ), true );
            }
            else
            {
                // pass 1:
                // cyclic tree must throw exception to prevent infinite recursion
                CPPUNIT_ASSERT_THROW( doc.GetPage( pagenum ), PdfError );
            }
        }
    }
}

void PagesTreeTest::testEmptyKidsTree()
{
    PdfMemDocument doc;
    CreateEmptyKidsTree(doc);
    //doc.Write("tree_zerokids.pdf");
    for (int pagenum=0; pagenum < doc.GetPageCount(); pagenum++)
    {
        PdfPage* pPage = doc.GetPage( pagenum );
        CPPUNIT_ASSERT_EQUAL( pPage != NULL, true );
        CPPUNIT_ASSERT_EQUAL( IsPageNumber( pPage, pagenum ), true );
    }
}

void PagesTreeTest::testNestedArrayTree()
{
    PdfMemDocument doc;
    CreateNestedArrayTree(doc);
    //doc.Write("tree_nested_array.pdf");
    for (int pagenum=0; pagenum < doc.GetPageCount(); pagenum++)
    {
        PdfPage* pPage = doc.GetPage( pagenum );
        CPPUNIT_ASSERT_EQUAL( pPage == NULL, true );
    }
}

void PagesTreeTest::testCreateDelete()
{
    PdfMemDocument  writer;
    PdfPage*        pPage;
    PdfPainter      painter;
	PdfFont *		pFont;

    // create font
	pFont = writer.CreateFont( "Arial" );
    if( !pFont )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }
    pFont->SetFontSize( 16.0 );

	// write 1. page
    pPage = writer.CreatePage( PdfPage::CreateStandardPageSize( ePdfPageSize_A4 ) );
    painter.SetPage( pPage );
    painter.SetFont( pFont );
    painter.DrawText( 200, 200, "Page 1"  );
    painter.FinishPage();
    CPPUNIT_ASSERT_EQUAL( writer.GetPageCount(), 1 );

	// write 2. page
    pPage = writer.CreatePage( PdfPage::CreateStandardPageSize( ePdfPageSize_A4 ) );
    painter.SetPage( pPage );
    painter.DrawText( 200, 200, "Page 2"  );
    painter.FinishPage();
    CPPUNIT_ASSERT_EQUAL( writer.GetPageCount(), 2 );

	// try to delete second page, index is 0 based 
	writer.DeletePages( 1, 1 );
    CPPUNIT_ASSERT_EQUAL( writer.GetPageCount(), 1 );

	// write 3. page
    pPage = writer.CreatePage( PdfPage::CreateStandardPageSize( ePdfPageSize_A4 ) );
    painter.SetPage( pPage );
    painter.DrawText( 200, 200, "Page 3"  );
    painter.FinishPage();
    CPPUNIT_ASSERT_EQUAL( writer.GetPageCount(), 2 );
}

void PagesTreeTest::testGetPagesCustom() 
{
    PdfMemDocument doc;
    
    CreateTestTreeCustom( doc );

    testGetPages( doc );
}

void PagesTreeTest::testGetPagesPoDoFo() 
{
    PdfMemDocument doc;
    
    CreateTestTreePoDoFo( doc );

    testGetPages( doc );
}

void PagesTreeTest::testGetPages( PdfMemDocument & doc ) 
{
    for(int i=0; i<PODOFO_TEST_NUM_PAGES; i++) 
    {
        PdfPage* pPage = doc.GetPage( i );

        CPPUNIT_ASSERT_EQUAL( pPage != NULL, true );

        CPPUNIT_ASSERT_EQUAL( IsPageNumber( pPage, i ), true );
    }

    // Now delete first page 
    doc.GetPagesTree()->DeletePage( 0 );

    for(int i=0; i<PODOFO_TEST_NUM_PAGES - 1; i++) 
    {
        PdfPage* pPage = doc.GetPage( i );

        CPPUNIT_ASSERT_EQUAL( pPage != NULL, true );
        
        CPPUNIT_ASSERT_EQUAL( IsPageNumber( pPage, i + 1 ), true );
    }

    // Now delete any page
    const int DELETED_PAGE = 50;
    doc.GetPagesTree()->DeletePage( DELETED_PAGE );

    for(int i=0; i<PODOFO_TEST_NUM_PAGES - 2; i++) 
    {
        PdfPage* pPage = doc.GetPage( i );

        CPPUNIT_ASSERT_EQUAL( pPage != NULL, true );
        
        if( i < DELETED_PAGE )
        {
            CPPUNIT_ASSERT_EQUAL( IsPageNumber( pPage, i + 1 ), true );
        }
        else
        {
            CPPUNIT_ASSERT_EQUAL( IsPageNumber( pPage, i + 2 ), true );
        }
    }
}

void PagesTreeTest::testGetPagesReverseCustom() 
{
    PdfMemDocument doc;
    
    CreateTestTreeCustom( doc );

    testGetPagesReverse( doc );
}

void PagesTreeTest::testGetPagesReversePoDoFo() 
{
    PdfMemDocument doc;
    
    CreateTestTreePoDoFo( doc );

    testGetPagesReverse( doc );
}

void PagesTreeTest::testGetPagesReverse( PdfMemDocument & doc ) 
{
    for(int i=PODOFO_TEST_NUM_PAGES-1; i>=0; i--)
    {
        PdfPage* pPage = doc.GetPage( i );

        CPPUNIT_ASSERT_EQUAL( pPage != NULL, true );
        
        CPPUNIT_ASSERT_EQUAL( IsPageNumber( pPage, i ), true );
    }

    // Now delete first page 
    doc.GetPagesTree()->DeletePage( 0 );

    for(int i=PODOFO_TEST_NUM_PAGES-2; i>=0; i--)
    {
        PdfPage* pPage = doc.GetPage( i );

        CPPUNIT_ASSERT_EQUAL( pPage != NULL, true );
        
        CPPUNIT_ASSERT_EQUAL( IsPageNumber( pPage, i + 1 ), true );
    }
}

void PagesTreeTest::testInsertCustom() 
{
    PdfMemDocument doc;

    CreateTestTreeCustom( doc );

    testInsert( doc );
}

void PagesTreeTest::testInsertPoDoFo() 
{
    PdfMemDocument doc;

    CreateTestTreePoDoFo( doc );

    testInsert( doc );
}

void PagesTreeTest::testInsert( PdfMemDocument & doc ) 
{
    const int INSERTED_PAGE_FLAG= 1234;
    const int INSERTED_PAGE_FLAG1= 1234 + 1;
    const int INSERTED_PAGE_FLAG2= 1234 + 2;

    PdfPage* pPage = new PdfPage( PdfPage::CreateStandardPageSize( ePdfPageSize_A4 ),
                                  &(doc.GetObjects()) );
    pPage->GetObject()->GetDictionary().AddKey( PODOFO_TEST_PAGE_KEY, 
                                                static_cast<pdf_int64>(INSERTED_PAGE_FLAG) );

    // Insert page at the beginning
    doc.GetPagesTree()->InsertPage(
        ePdfPageInsertionPoint_InsertBeforeFirstPage,
        pPage );
    delete pPage;

    // Find inserted page (beginning)
    pPage = doc.GetPage( 0 );
    CPPUNIT_ASSERT_EQUAL( IsPageNumber( pPage, INSERTED_PAGE_FLAG ), true );
    
    // Find old first page
    pPage = doc.GetPage( 1 );
    CPPUNIT_ASSERT_EQUAL( IsPageNumber( pPage, 0 ), true );

    // Insert at end 
    pPage = doc.CreatePage( PdfPage::CreateStandardPageSize( ePdfPageSize_A4 ) );
    pPage->GetObject()->GetDictionary().AddKey( PODOFO_TEST_PAGE_KEY, 
                                                static_cast<pdf_int64>(INSERTED_PAGE_FLAG1) );

    pPage = doc.GetPage( doc.GetPageCount() - 1 );
    CPPUNIT_ASSERT_EQUAL( IsPageNumber( pPage, INSERTED_PAGE_FLAG1 ), true );

    // Insert in middle
    pPage = new PdfPage( PdfPage::CreateStandardPageSize( ePdfPageSize_A4 ),
                                  &(doc.GetObjects()) );
    pPage->GetObject()->GetDictionary().AddKey( PODOFO_TEST_PAGE_KEY, 
                                                static_cast<pdf_int64>(INSERTED_PAGE_FLAG2) );

    const int INSERT_POINT = 50;
    doc.GetPagesTree()->InsertPage( INSERT_POINT, pPage );
    delete pPage;

    pPage = doc.GetPage( INSERT_POINT + 1 );
    CPPUNIT_ASSERT_EQUAL( IsPageNumber( pPage, INSERTED_PAGE_FLAG2 ), true );
}

void PagesTreeTest::testDeleteAllCustom() 
{
    PdfMemDocument doc;

    CreateTestTreeCustom( doc );

    testDeleteAll( doc );
}

void PagesTreeTest::testDeleteAllPoDoFo() 
{
    PdfMemDocument doc;

    CreateTestTreePoDoFo( doc );

    testDeleteAll( doc );
}

void PagesTreeTest::testDeleteAll( PdfMemDocument & doc ) 
{
    for(int i=0; i<PODOFO_TEST_NUM_PAGES; i++) 
    {
        doc.GetPagesTree()->DeletePage(0);

        CPPUNIT_ASSERT_EQUAL( doc.GetPageCount(), PODOFO_TEST_NUM_PAGES - (i + 1) );
    }

    CPPUNIT_ASSERT_EQUAL( doc.GetPageCount(), 0 );
}

void PagesTreeTest::CreateTestTreePoDoFo( PoDoFo::PdfMemDocument & rDoc )
{
    for(int i=0; i<PODOFO_TEST_NUM_PAGES; i++) 
    {
        PdfPage* pPage = rDoc.CreatePage( PdfPage::CreateStandardPageSize( ePdfPageSize_A4 ) );
        pPage->GetObject()->GetDictionary().AddKey( PODOFO_TEST_PAGE_KEY, static_cast<pdf_int64>(i) );

        CPPUNIT_ASSERT_EQUAL( rDoc.GetPageCount(), i + 1 );
    }
}

void PagesTreeTest::CreateTestTreeCustom( PoDoFo::PdfMemDocument & rDoc )
{
    const int COUNT = PODOFO_TEST_NUM_PAGES / 10;
    PdfObject* pRoot = rDoc.GetPagesTree()->GetObject();
    PdfArray rootKids;
    

    for(int z=0; z<COUNT; z++) 
    {
        PdfObject* pNode = rDoc.GetObjects().CreateObject("Pages");
        PdfArray nodeKids;

        for(int i=0; i<COUNT; i++) 
        {
            PdfPage* pPage = new PdfPage( PdfPage::CreateStandardPageSize( ePdfPageSize_A4 ),
                                          &(rDoc.GetObjects()) );
            pPage->GetObject()->GetDictionary().AddKey( PODOFO_TEST_PAGE_KEY, 
                                                        static_cast<pdf_int64>(z * COUNT + i) );

            //printf("Creating page %i z=%i i=%i\n", z * COUNT + i, z, i );
            nodeKids.push_back( pPage->GetObject()->Reference() );
        }

        pNode->GetDictionary().AddKey( PdfName("Kids"), nodeKids );
        pNode->GetDictionary().AddKey( PdfName("Count"), static_cast<pdf_int64>(COUNT) );
        rootKids.push_back( pNode->Reference() );
    }

    pRoot->GetDictionary().AddKey( PdfName("Kids"), rootKids );
    pRoot->GetDictionary().AddKey( PdfName("Count"), static_cast<pdf_int64>(PODOFO_TEST_NUM_PAGES) );
}

std::vector<PdfPage*> PagesTreeTest::CreateSamplePages( PdfMemDocument & rDoc,
                                                        int nPageCount)
{
    PdfFont* pFont;

    // create font
    pFont = rDoc.CreateFont( "Arial" );
    if( !pFont )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }
    pFont->SetFontSize( 16.0 );

    std::vector<PdfPage*> pPage(nPageCount);
    for (int i = 0; i < nPageCount; ++i)
    {
        pPage[i] = new PdfPage( PdfPage::CreateStandardPageSize( ePdfPageSize_A4 ),
                                &(rDoc.GetObjects()) );
        pPage[i]->GetObject()->GetDictionary().AddKey( PODOFO_TEST_PAGE_KEY, 
                                                       static_cast<pdf_int64>(i) );

        PdfPainter painter;
        painter.SetPage( pPage[i] );
        painter.SetFont( pFont );
        std::ostringstream os;
        os << "Page " << i+1;
        painter.DrawText( 200, 200, os.str()  );
        painter.FinishPage();
    }

    return pPage;
}

std::vector<PdfObject*> PagesTreeTest::CreateNodes( PdfMemDocument & rDoc,
                                                    int nNodeCount)
{
    std::vector<PdfObject*> pNode(nNodeCount);

    for (int i = 0; i < nNodeCount; ++i)
    {
        pNode[i]=rDoc.GetObjects().CreateObject("Pages");
        // init required keys
        pNode[i]->GetDictionary().AddKey( "Kids", PdfArray());
        pNode[i]->GetDictionary().AddKey( "Count", PdfVariant(static_cast<pdf_int64>(0L)));
    }

    return pNode;
}

void PagesTreeTest::CreateCyclicTree( PoDoFo::PdfMemDocument & rDoc,
                                      bool bCreateCycle )
{
    const int COUNT = 3;

    std::vector<PdfPage*> pPage=CreateSamplePages( rDoc, COUNT );
    std::vector<PdfObject*> pNode=CreateNodes( rDoc, 2 );
    
    // manually insert pages into pagetree
    PdfObject* pRoot = rDoc.GetPagesTree()->GetObject();

    // tree layout (for !bCreateCycle):
    //
    //    root
    //    +-- node0
    //        +-- node1
    //        |   +-- page0
    //        |   +-- page1
    //        \-- page2

    // root node
    AppendChildNode(pRoot, pNode[0]);

    // tree node 0
    AppendChildNode(pNode[0], pNode[1]);
    AppendChildNode(pNode[0], pPage[2]->GetObject());

    // tree node 1
    AppendChildNode(pNode[1], pPage[0]->GetObject());
    AppendChildNode(pNode[1], pPage[1]->GetObject());

    if (bCreateCycle)
    {
        // invalid tree: Cycle!!!
        // was not detected in PdfPagesTree::GetPageNode() rev. 1937
        pNode[0]->GetIndirectKey("Kids")->GetArray()[0]=pRoot->Reference();
    }
}

void PagesTreeTest::CreateEmptyKidsTree( PoDoFo::PdfMemDocument & rDoc )
{
    const int COUNT = 3;

    std::vector<PdfPage*> pPage=CreateSamplePages( rDoc, COUNT );
    std::vector<PdfObject*> pNode=CreateNodes( rDoc, 3 );
    
    // manually insert pages into pagetree
    PdfObject* pRoot = rDoc.GetPagesTree()->GetObject();
    
    // tree layout:
    //
    //    root
    //    +-- node0
    //    |   +-- page0
    //    |   +-- page1
    //    |   +-- page2
    //    +-- node1
    //    \-- node2

    // root node
    AppendChildNode(pRoot, pNode[0]);
    AppendChildNode(pRoot, pNode[1]);
    AppendChildNode(pRoot, pNode[2]);
    
    // tree node 0
    AppendChildNode(pNode[0], pPage[0]->GetObject());
    AppendChildNode(pNode[0], pPage[1]->GetObject());
    AppendChildNode(pNode[0], pPage[2]->GetObject());

    // tree node 1 and node 2 are left empty: this is completely valid
    // according to the PDF spec, i.e. the required keys may have the
    // values "/Kids [ ]" and "/Count 0"
}

void PagesTreeTest::CreateNestedArrayTree( PoDoFo::PdfMemDocument & rDoc )
{
    const int COUNT = 3;

    std::vector<PdfPage*> pPage=CreateSamplePages( rDoc, COUNT );
    PdfObject* pRoot = rDoc.GetPagesTree()->GetObject();

    // create kids array
    PdfArray kids;
    for (int i=0; i < COUNT; i++)
    {
        kids.push_back( pPage[i]->GetObject()->Reference() );
        pPage[i]->GetObject()->GetDictionary().AddKey( PdfName("Parent"), pRoot->Reference());
    }

    // create nested kids array
    PdfArray nested;
    nested.push_back(kids);

    // manually insert pages into pagetree
    pRoot->GetDictionary().AddKey( PdfName("Count"), static_cast<pdf_int64>(COUNT) );
    pRoot->GetDictionary().AddKey( PdfName("Kids"), nested);
}

bool PagesTreeTest::IsPageNumber( PoDoFo::PdfPage* pPage, int nNumber )
{
    pdf_int64 lPageNumber = pPage->GetObject()->GetDictionary().GetKeyAsLong( PODOFO_TEST_PAGE_KEY, -1 );

    if( lPageNumber != static_cast<pdf_int64>(nNumber) )
    {
        printf("PagesTreeTest: Expected page number %i but got %" PDF_FORMAT_INT64 ".\n", nNumber, lPageNumber);
        return false;
    }
    else
        return true;
}

void PagesTreeTest::AppendChildNode(PdfObject* pParent, PdfObject* pChild)
{
    // 1. Add the reference of the new child to the kids array of pParent
    PdfArray kids;
    PdfObject* oldKids=pParent->GetIndirectKey("Kids");
    if (oldKids && oldKids->IsArray()) kids=oldKids->GetArray();
    kids.push_back(pChild->Reference());
    pParent->GetDictionary().AddKey( PdfName("Kids"), kids);

    // 2. If the child is a page (leaf node), increase count of every parent
    //    (which also includes pParent)
    if( pChild->GetDictionary().GetKeyAsName( PdfName( "Type" ) )
        == PdfName( "Page" ) )
    {
        PdfObject* node=pParent;
        while (node)
        {
            pdf_int64 count=0;
            if (node->GetIndirectKey("Count")) count=node->GetIndirectKey("Count")->GetNumber();
            count++;
            node->GetDictionary().AddKey( PdfName("Count"), count);
            
            node=node->GetIndirectKey("Parent");
        }
    }
    
    // 3. Add Parent key to the child
    pChild->GetDictionary().AddKey( PdfName("Parent"), pParent->Reference());
}
