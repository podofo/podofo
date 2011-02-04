/***************************************************************************
 *   Copyright (C) 2000 by Dominik Seichter                                *
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

#include "PageTest.h"
#include "TestUtils.h"

#include <podofo.h>

using namespace PoDoFo;

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( PageTest );

void PageTest::setUp()
{
}

void PageTest::tearDown()
{
}

void PageTest::testEmptyContents()
{
    PdfVecObjects vecObjects;
    PdfObject object( PdfReference( 1, 0 ), "Page" );
    vecObjects.push_back( &object );

    const std::deque<PdfObject*> parents;
    PdfPage page( &object, parents );
    CPPUNIT_ASSERT( NULL != page.GetContents() );
    
}

void PageTest::testEmptyContentsStream()
{
    PdfMemDocument doc;
    PdfPage*       pPage = doc.CreatePage( PdfPage::CreateStandardPageSize( ePdfPageSize_A4 ) );
    PdfAnnotation* pAnnot = pPage->CreateAnnotation( ePdfAnnotation_Popup, PdfRect( 300.0, 20.0, 250.0, 50.0 ) );
    PdfString      sTitle("Author: Dominik Seichter");
    pAnnot->SetContents( sTitle );
    pAnnot->SetOpen( true );

    std::string sFilename = TestUtils::getTempFilename();
    doc.Write( sFilename.c_str() );

    // Read annotation again
    PdfMemDocument doc2( sFilename.c_str() );
    CPPUNIT_ASSERT_EQUAL( 1, doc2.GetPageCount() );
    PdfPage* pPage2 = doc2.GetPage( 0 );
    CPPUNIT_ASSERT( NULL != pPage2 );
    CPPUNIT_ASSERT_EQUAL( 1, pPage2->GetNumAnnots() );
    PdfAnnotation* pAnnot2 = pPage2->GetAnnotation( 0 );
    CPPUNIT_ASSERT( NULL != pAnnot2 );
    CPPUNIT_ASSERT( sTitle == pAnnot2->GetContents() );

    PdfObject* pPageObject = pPage2->GetObject();        
    CPPUNIT_ASSERT( !pPageObject->GetDictionary().HasKey("Contents") );

    TestUtils::deleteFile( sFilename.c_str() );
}

