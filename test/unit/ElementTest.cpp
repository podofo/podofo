/***************************************************************************
 *   Copyright (C) 2010 by Dominik Seichter                                *
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

#include "ElementTest.h"

#include <podofo.h>

using namespace PoDoFo;

CPPUNIT_TEST_SUITE_REGISTRATION( ElementTest );

void ElementTest::setUp()
{
}

void ElementTest::tearDown()
{
}

void ElementTest::testTypeToIndexAnnotation()
{
    // Check last entry in the type names array of PdfAnnotation
    PdfObject object;
    object.GetDictionary().AddKey( PdfName("Type"), PdfName("Annot") );
    object.GetDictionary().AddKey( PdfName("Subtype"), PdfName("RichMedia") );
    
    PdfAnnotation annot(&object, NULL);
    CPPUNIT_ASSERT_EQUAL( ePdfAnnotation_RichMedia, annot.GetType() );
}

void ElementTest::testTypeToIndexAction()
{
    // Check last entry in the type names array of PdfAction
    PdfObject object;
    object.GetDictionary().AddKey( PdfName("Type"), PdfName("Action") );
    object.GetDictionary().AddKey( PdfName("S"), PdfName("GoTo3DView") );
    
    PdfAction action(&object);
    CPPUNIT_ASSERT_EQUAL( ePdfAction_GoTo3DView, action.GetType() );
}

void ElementTest::testTypeToIndexAnnotationUnknown()
{
    // Check last entry in the type names array of PdfAnnotation
    PdfObject object;
    object.GetDictionary().AddKey( PdfName("Type"), PdfName("Annot") );
    object.GetDictionary().AddKey( PdfName("Subtype"), PdfName("PoDoFoRocksUnknownType") );
    
    PdfAnnotation annot(&object, NULL);
    CPPUNIT_ASSERT_EQUAL( ePdfAnnotation_Unknown, annot.GetType() );
}

void ElementTest::testTypeToIndexActionUnknown()
{
    // Check last entry in the type names array of PdfAction
    PdfObject object;
    object.GetDictionary().AddKey( PdfName("Type"), PdfName("Action") );
    object.GetDictionary().AddKey( PdfName("S"), PdfName("PoDoFoRocksUnknownType") );
    
    PdfAction action(&object);
    CPPUNIT_ASSERT_EQUAL( ePdfAction_Unknown, action.GetType() );
}
