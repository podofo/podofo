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

#ifndef _TOKENIZER_TEST_H_
#define _TOKENIZER_TEST_H_

#include <cppunit/extensions/HelperMacros.h>

#include <podofo.h>

/** This test tests the class PdfTokenizer
 *
 *  Currently the following methods are tested
 *  - void PdfTokenizer::GetNextVariant( PdfVariant& rVariant, PdfEncrypt* pEncrypt );
 *  - bool PdfTokenizer::GetNextToken( const char *& pszToken, EPdfTokenType* peType = NULL);
 *  - void PdfTokenizer::IsNextToken( const char* pszToken );
 */
class TokenizerTest : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( TokenizerTest );
  CPPUNIT_TEST( testArrays );
  CPPUNIT_TEST( testBool );
  CPPUNIT_TEST( testHexString );
  CPPUNIT_TEST( testName );
  CPPUNIT_TEST( testNull );
  CPPUNIT_TEST( testNumbers );
  CPPUNIT_TEST( testReference );
  CPPUNIT_TEST( testString );
  CPPUNIT_TEST( testTokens );
  CPPUNIT_TEST( testComments );
  CPPUNIT_TEST( testDictionary );
  CPPUNIT_TEST( testLocale );
  CPPUNIT_TEST_SUITE_END();

 public:
  void setUp();
  void tearDown();

  void testArrays();
  void testBool();
  void testHexString();
  void testName();
  void testNull();
  void testNumbers();
  void testReference();
  void testString();
  void testDictionary();

  void testTokens();
  void testComments();

  void testLocale();

 private:
  void Test( const char* pszString, PoDoFo::EPdfDataType eDataType, const char* pszExpected = NULL );

  /** Test parsing a stream.
   *
   *  \param pszBuffer a string buffer that will be parsed
   *  \param pszTokens a NULL terminated list of all tokens in the  
   *                   order PdfTokenizer should read them from pszBuffer 
   */
  void TestStream( const char* pszBuffer, const char* pszTokens[] );

  /** Test parsing a stream. As above but this time using PdfTokenizer::IsNextToken()
   *
   *  \param pszBuffer a string buffer that will be parsed
   *  \param pszTokens a NULL terminated list of all tokens in the  
   *                   order PdfTokenizer should read them from pszBuffer 
   */
  void TestStreamIsNextToken( const char* pszBuffer, const char* pszTokens[] );
};

#endif // _TOKENIZER_TEST_H_


