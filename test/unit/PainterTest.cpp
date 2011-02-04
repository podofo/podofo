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

#include "PainterTest.h"
#include "TestUtils.h"

#include <podofo.h>

using namespace PoDoFo;

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( PainterTest );

void PainterTest::setUp()
{

}

void PainterTest::tearDown()
{
}

void PainterTest::CompareStreamContent(PdfStream* pStream, const char* pszExpected)
{
    char* pBuffer;
    pdf_long lLen;
    pStream->GetFilteredCopy( &pBuffer, &lLen );

    std::string str(pBuffer, lLen);
    CPPUNIT_ASSERT_EQUAL( std::string(pszExpected), str );

    free( pBuffer );
}

void PainterTest::testAppend()
{
    const char* pszExample1 = "BT (Hallo) Tj ET";
    const char* pszColor = " 1.000 1.000 1.000 rg\n";

    PdfMemDocument doc;
    PdfPage* pPage = doc.CreatePage( PdfPage::CreateStandardPageSize( ePdfPageSize_A4 ) );
    pPage->GetContents()->GetStream()->Set(pszExample1) ;
    
    this->CompareStreamContent(pPage->GetContents()->GetStream(), pszExample1);

    PdfPainter painter;
    painter.SetPage( pPage );
    painter.SetColor( 1.0, 1.0, 1.0 );
    painter.FinishPage();

    std::string newContent = pszExample1;
    newContent += pszColor;

    this->CompareStreamContent(pPage->GetContents()->GetStream(), newContent.c_str());
}
