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
