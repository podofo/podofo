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

#ifndef _PARSER_TEST_H_
#define _PARSER_TEST_H_

#include <cppunit/extensions/HelperMacros.h>

#include <podofo.h>

/** This test tests the class PdfParser
 *
 *  PdfParser was responsible for 14% of the PoDoFo CVEs reported up to April 2018
 *  so this class tests CVE fixes along with additional tests to test boundary conditions
 */
class ParserTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE( ParserTest );
    CPPUNIT_TEST( testMaxObjectCount );
    CPPUNIT_TEST( testReadDocumentStructure );
    CPPUNIT_TEST( testReadXRefContents );
    CPPUNIT_TEST( testReadXRefContents );
    CPPUNIT_TEST( testReadXRefSubsection );
    CPPUNIT_TEST( testReadXRefStreamContents );
    CPPUNIT_TEST( testReadObjects );
    CPPUNIT_TEST( testIsPdfFile );
    CPPUNIT_TEST( testRoundTripIndirectTrailerID );
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

    // commented out tests still need implemented

    void testMaxObjectCount();

    // CVE-2018-8002
    // testParseFile();

    void testReadDocumentStructure();
    //void testReadTrailer();
    //void testReadXRef();

    // CVE-2017-8053 tested
    void testReadXRefContents();

    // CVE-2015-8981, CVE-2017-5853, CVE-2018-5296 - tested
    // CVE-2017-6844, CVE-2017-5855 - symptoms are false postives due to ASAN allocator_may_return_null=1 not throwing bad_alloc
    void testReadXRefSubsection();

    // CVE-2017-8787, CVE-2018-5295 - tested
    void testReadXRefStreamContents();

    // CVE-2017-8378 - tested
    // CVE-2018-6352 - no fix yet, so no test yet
    void testReadObjects();

    //void testReadObjectFromStream();
    void testIsPdfFile();
    //void testReadNextTrailer();
    //void testCheckEOFMarker();

    void testRoundTripIndirectTrailerID();

private:
    std::string generateXRefEntries( size_t count );
    bool canOutOfMemoryKillUnitTests();
};

#endif // _PARSER_TEST_H_


