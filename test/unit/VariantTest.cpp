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

#include "VariantTest.h"

#include <podofo.h>

using namespace PoDoFo;

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( VariantTest );

static const char* s_pszObjectData = 
    "242 0 obj\n"
    "<<\n"
    "/Type /Metadata\n"
    "/Length 9393\n"
    "/Subtype /XML\n"
    ">>\n"
    "stream\n"
    "<x:xmpmeta xmlns:x=\"adobe:ns:meta/\" x:xmptk=\"3.1.1-111\">\n"
    " <rdf:RDF xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\">\n"
    "  <rdf:Description rdf:about=\"\"\n"
    "    xmlns:dc=\"http://purl.org/dc/elements/1.1/\"\n"
    "    xmlns:xap=\"http://ns.adobe.com/xap/1.0/\"\n"
    "    xmlns:xapGImg=\"http://ns.adobe.com/xap/1.0/g/img/\"\n"
    "    xmlns:xapMM=\"http://ns.adobe.com/xap/1.0/mm/\"\n"
    "    xmlns:stRef=\"http://ns.adobe.com/xap/1.0/sType/ResourceRef#\"\n"
    "   dc:format=\"application/pdf\"\n"
    "   xap:CreatorTool=\"Adobe Illustrator CS2\"\n"
    "   xap:CreateDate=\"2006-01-22T11:41:01-08:00\"\n"
    "   xap:ModifyDate=\"2006-01-22T16:11:11-08:00\"\n"
    "   xap:MetadataDate=\"2006-01-22T16:11:11-08:00\"\n"
    "   xapMM:DocumentID=\"uuid:9D3BA55D8CCC11DA9C1EF28F08BA9E2D\"\n"
    "   xapMM:InstanceID=\"uuid:c2536d1f-8ba4-11da-9a3c-000d937692d2\">\n"
    "   <xap:Thumbnails>\n"
    "    <rdf:Alt>\n"
    "     <rdf:li\n"
    "      xapGImg:width=\"256\"\n"
    "      xapGImg:height=\"92\"\n"
    "      xapGImg:format=\"JPEG\"\n"
    "      xapGImg:image=\"/9j/4AAQSkZJRgABAgEASABIAAD/7QAsUGhvdG9zaG9wIDMuMAA4QklNA+0AAAAAABAASAAAAAEA&#xA;AQBIAAAAAQAB/+4ADkFkb2JlAGTAAAAAAf/bAIQABgQEBAUEBgUFBgkGBQYJCwgGBggLDAoKCwoK&#xA;DBAMDAwMDAwQDA4PEA8ODBMTFBQTExwbGxscHx8fHx8fHx8fHwEHBwcNDA0YEBAYGhURFRofHx8f&#xA;Hx8fHx8fHx8fHx8fHx8fHx8fHx8fHx8fHx8fHx8fHx8fHx8fHx8fHx8fHx8f/8AAEQgAXAEAAwER&#xA;AAIRAQMRAf/EAaIAAAAHAQEBAQEAAAAAAAAAAAQFAwIGAQAHCAkKCwEAAgIDAQEBAQEAAAAAAAAA&#xA;AQACAwQFBgcICQoLEAACAQMDAgQCBgcDBAIGAnMBAgMRBAAFIRIxQVEGE2EicYEUMpGhBxWxQiPB&#xA;UtHhMxZi8CRygvElQzRTkqKyY3PCNUQnk6OzNhdUZHTD0uIIJoMJChgZhJRFRqS0VtNVKBry4/PE&#xA;1OT0ZXWFlaW1xdXl9WZ2hpamtsbW5vY3R1dnd4eXp7fH1+f3OEhYaHiImKi4yNjo+Ck5SVlpeYmZ&#xA;qbnJ2en5KjpKWmp6ipqqusra6voRAAICAQIDBQUEBQYECAMDbQEAAhEDBCESMUEFURNhIgZxgZEy&#xA;obHwFMHR4SNCFVJicvEzJDRDghaSUyWiY7LCB3PSNeJEgxdUkwgJChgZJjZFGidkdFU38qOzwygp&#xA;0+PzhJSktMTU5PRldYWVpbXF1eX1RlZmdoaWprbG1ub2R1dnd4eXp7fH1+f3OEhYaHiImKi4yNjo&#xA;+DlJWWl5iZmpucnZ6fkqOkpaanqKmqq6ytrq+v/aAAwDAQACEQMRAD8A9U4q7FXYq7FXYq7FXYq7&#xA;FXYq7FXYq7FXYq7FXYq7FXYq7FXYq7FXYq7FXYq7FXYq7FXYq7FXYq7FXYq7FXYq7FXYq7FWmZVU&#xA;sxCqNyTsAMEpACzyUBK5vNGhRNxN0GI/kVmH3gEZqMnb+jgaM79wJ+4U5MdJkPREWmt6VdkLBcoz&#xA;HohPFj8g1DmRp+1dNmNQmCfkfkaYTwTjzCNzYNLsVdirsVdirsVdirsVdirsVdirsVdirsVdirsV&#xA;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&#xA;2KuxV2KuxV2KuxV2KuxV2KuxV2KuxV2KuxV//9k=\"/>\n"
    "    </rdf:Alt>\n"
    "   </xap:Thumbnails>\n"
    "   <xapMM:DerivedFrom\n"
    "    stRef:instanceID=\"uuid:80a15048-8a07-11da-95c4-000d937692d2\"\n"
    "    stRef:documentID=\"uuid:912BD87F8B5211DA82D09FC838327668\"/>\n"
    "  </rdf:Description>\n"
    " </rdf:RDF>\n"
    "</x:xmpmeta>\n"
    "\n"
    "endstream\n"
    "endobj\n";

void VariantTest::setUp()
{
}

void VariantTest::tearDown()
{
}

void VariantTest::testEmptyObject() 
{
    const char* pszObject = 
        "10 0 obj\nendobj\n";

    
    PdfRefCountedInputDevice device( pszObject, strlen( pszObject ) );
    PdfRefCountedBuffer buffer( 1024 );
    PdfVecObjects vecObjects;

    PdfParserObject parser( &vecObjects, device, buffer, 0 );
    parser.SetLoadOnDemand( false );
    parser.ParseFile( NULL );

    CPPUNIT_ASSERT_EQUAL( parser.IsNull(), true );
}

void VariantTest::testEmptyStream() 
{
    const char* pszObject = 
        "10 0 obj<</Length 0>>stream\nendstream\nendobj\n";

    
    PdfRefCountedInputDevice device( pszObject, strlen( pszObject ) );
    PdfRefCountedBuffer buffer( 1024 );
    PdfVecObjects vecObjects;

    PdfParserObject parser( &vecObjects, device, buffer, 0 );
    parser.SetLoadOnDemand( false );
    parser.ParseFile( NULL );

    CPPUNIT_ASSERT_EQUAL( parser.IsDictionary(), true );
    CPPUNIT_ASSERT_EQUAL( parser.HasStream(), true );
    CPPUNIT_ASSERT_EQUAL( parser.GetStream()->GetLength(), static_cast<pdf_long>(0) );
}


void VariantTest::testNameObject()
{
    const char* pszObject = 
        "10 0 obj / endobj\n";

    
    PdfRefCountedInputDevice device( pszObject, strlen( pszObject ) );
    PdfRefCountedBuffer buffer( 1024 );
    PdfVecObjects vecObjects;

    PdfParserObject parser( &vecObjects, device, buffer, 0 );
    parser.SetLoadOnDemand( false );
    parser.ParseFile( NULL );

    CPPUNIT_ASSERT_EQUAL( parser.IsName(), true );
    CPPUNIT_ASSERT_EQUAL( parser.GetName().GetName(), std::string("") );
}

void VariantTest::testIsDirtyTrue()
{
    PdfArray array;
    PdfDictionary dict;

    PdfVariant varBool( true );
    PdfVariant varLong( static_cast<pdf_int64>(1LL) );
    PdfVariant varDouble( 1.0 );
    PdfVariant varStr( PdfString("Any") );
    PdfVariant varName( PdfName("Name") );
    PdfVariant varRef( PdfReference( 0, 0 ) );
    PdfVariant varArray( array );
    PdfVariant varDict( dict );
    PdfVariant varVariant( varBool );

    varBool.SetBool( false );
    varLong.SetNumber( static_cast<pdf_int64>(2LL) );
    varDouble.SetReal( 2.0 );
    varStr = PdfString("Other");
    varName = PdfName("Name2");
    varRef = PdfReference( 2, 0 );
    varArray.GetArray().push_back( varBool );
    varDict.GetDictionary().AddKey( varName.GetName(), varStr );
    varVariant = varLong;

    CPPUNIT_ASSERT_EQUAL_MESSAGE( "BOOL      IsDirty() == true", true, varBool.IsDirty() );
    CPPUNIT_ASSERT_EQUAL_MESSAGE( "LONG      IsDirty() == true", true, varLong.IsDirty() );
    CPPUNIT_ASSERT_EQUAL_MESSAGE( "DOUBLE    IsDirty() == true", true, varDouble.IsDirty() );
    CPPUNIT_ASSERT_EQUAL_MESSAGE( "STRING    IsDirty() == true", true, varStr.IsDirty() );
    CPPUNIT_ASSERT_EQUAL_MESSAGE( "REFERENCE IsDirty() == true", true, varRef.IsDirty() );
    CPPUNIT_ASSERT_EQUAL_MESSAGE( "ARRAY     IsDirty() == true", true, varArray.IsDirty() );
    CPPUNIT_ASSERT_EQUAL_MESSAGE( "DICT      IsDirty() == true", true, varDict.IsDirty() );
    CPPUNIT_ASSERT_EQUAL_MESSAGE( "VARIANT   IsDirty() == true", true, varVariant.IsDirty() );

    PdfRefCountedInputDevice device( s_pszObjectData, strlen( s_pszObjectData ) );
    PdfRefCountedBuffer buffer( 1024 );
    PdfVecObjects vecObjects;
    PdfParserObject parser( &vecObjects, device, buffer, 0 );
    parser.SetLoadOnDemand( false );
    parser.ParseFile( NULL );

    // After reading const stream it has still to be clean
    PdfStream* pStream = parser.GetStream();
    CPPUNIT_ASSERT_EQUAL( static_cast<long>(pStream->GetLength()), 9381L );
    CPPUNIT_ASSERT_EQUAL_MESSAGE( "STREAM    IsDirty() == true", true, parser.IsDirty() );
}

void VariantTest::testIsDirtyFalse()
{
    PdfArray array;
    PdfDictionary dict;
    PdfData data("/Name");

    PdfVariant varEmpty;
    PdfVariant varBool( true );
    PdfVariant varLong( static_cast<pdf_int64>(1LL) );
    PdfVariant varDouble( 1.0 );
    PdfVariant varStr( PdfString("Any") );
    PdfVariant varName( PdfName("Name") );
    PdfVariant varRef( PdfReference( 0, 0 ) );
    PdfVariant varArray( array );
    PdfVariant varDict( dict );
    PdfVariant varData( data );
    PdfVariant varVariant( varBool );

    // IsDirty() should be false after construction
    CPPUNIT_ASSERT_EQUAL_MESSAGE( "EMPTY     IsDirty() == false", false, varEmpty.IsDirty() );
    CPPUNIT_ASSERT_EQUAL_MESSAGE( "BOOL      IsDirty() == false", false, varBool.IsDirty() );
    CPPUNIT_ASSERT_EQUAL_MESSAGE( "LONG      IsDirty() == false", false, varLong.IsDirty() );
    CPPUNIT_ASSERT_EQUAL_MESSAGE( "DOUBLE    IsDirty() == false", false, varDouble.IsDirty() );
    CPPUNIT_ASSERT_EQUAL_MESSAGE( "STRING    IsDirty() == false", false, varStr.IsDirty() );
    CPPUNIT_ASSERT_EQUAL_MESSAGE( "REFERENCE IsDirty() == false", false, varRef.IsDirty() );
    CPPUNIT_ASSERT_EQUAL_MESSAGE( "ARRAY     IsDirty() == false", false, varArray.IsDirty() );
    CPPUNIT_ASSERT_EQUAL_MESSAGE( "DICT      IsDirty() == false", false, varDict.IsDirty() );
    CPPUNIT_ASSERT_EQUAL_MESSAGE( "DATA      IsDirty() == false", false, varData.IsDirty() );
    CPPUNIT_ASSERT_EQUAL_MESSAGE( "VARIANT   IsDirty() == false", false, varVariant.IsDirty() );

    // IsDirty() should be false after calling const getter
    (void)varBool.GetBool();
    (void)varLong.GetNumber();
    (void)varDouble.GetReal();
    (void)varStr.GetString();
    (void)varName.GetName();
    (void)varRef.GetReference();
    (void)static_cast<const PdfVariant>(varArray).GetArray();
    (void)static_cast<const PdfVariant>(varDict).GetDictionary();
    (void)varVariant.GetBool();

    CPPUNIT_ASSERT_EQUAL_MESSAGE( "BOOL      IsDirty() == false", false, varBool.IsDirty() );
    CPPUNIT_ASSERT_EQUAL_MESSAGE( "LONG      IsDirty() == false", false, varLong.IsDirty() );
    CPPUNIT_ASSERT_EQUAL_MESSAGE( "DOUBLE    IsDirty() == false", false, varDouble.IsDirty() );
    CPPUNIT_ASSERT_EQUAL_MESSAGE( "STRING    IsDirty() == false", false, varStr.IsDirty() );
    CPPUNIT_ASSERT_EQUAL_MESSAGE( "REFERENCE IsDirty() == false", false, varRef.IsDirty() );
    CPPUNIT_ASSERT_EQUAL_MESSAGE( "ARRAY     IsDirty() == false", false, varArray.IsDirty() );
    CPPUNIT_ASSERT_EQUAL_MESSAGE( "DICT      IsDirty() == false", false, varDict.IsDirty() );
    CPPUNIT_ASSERT_EQUAL_MESSAGE( "VARIANT   IsDirty() == false", false, varVariant.IsDirty() );

    // IsDirty() should be false after calling non const getter, but not modifying object
    (void) varArray.GetArray();
    (void) varDict.GetDictionary();

    CPPUNIT_ASSERT_EQUAL_MESSAGE( "ARRAY     IsDirty() == false", false, varArray.IsDirty() );
    CPPUNIT_ASSERT_EQUAL_MESSAGE( "DICT      IsDirty() == false", false, varDict.IsDirty() );


    // IsDirty() should be false after reading an object
    PdfRefCountedInputDevice device( s_pszObjectData, strlen( s_pszObjectData ) );
    PdfRefCountedBuffer buffer( 1024 );
    PdfVecObjects vecObjects;
    PdfParserObject parser( &vecObjects, device, buffer, 0 );
    parser.SetLoadOnDemand( false );
    parser.ParseFile( NULL );

    // Newly create Object has to be clean
    CPPUNIT_ASSERT_EQUAL_MESSAGE( "OBJECT    IsDirty() == false", false, parser.IsDirty() );

    // After reading const stream it has still to be clean
    const PdfStream* pStream = static_cast<const PdfParserObject*>(&parser)->GetStream();
    CPPUNIT_ASSERT_EQUAL( static_cast<long>(pStream->GetLength()), 9381L );
    CPPUNIT_ASSERT_EQUAL_MESSAGE( "STREAM    IsDirty() == false", false, parser.IsDirty() );
}
