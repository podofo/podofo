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

using namespace PoDoFo;

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( PagesTreeTest );

void PagesTreeTest::setUp()
{
}

void PagesTreeTest::tearDown()
{
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
