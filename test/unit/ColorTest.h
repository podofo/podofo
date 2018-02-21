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

#ifndef _COLOR_TEST_H_
#define _COLOR_TEST_H_

#include <cppunit/extensions/HelperMacros.h>

class ColorTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE( ColorTest );
    CPPUNIT_TEST( testDefaultConstructor );
    CPPUNIT_TEST( testGreyConstructor );
    CPPUNIT_TEST( testGreyConstructorInvalid );
    CPPUNIT_TEST( testRGBConstructor );
    CPPUNIT_TEST( testRGBConstructorInvalid );
    CPPUNIT_TEST( testCMYKConstructor );
    CPPUNIT_TEST( testCMYKConstructorInvalid );
    CPPUNIT_TEST( testCopyConstructor );
    CPPUNIT_TEST( testAssignmentOperator );
    CPPUNIT_TEST( testEqualsOperator );
    CPPUNIT_TEST( testHexNames );
    CPPUNIT_TEST( testNamesGeneral );
    CPPUNIT_TEST( testNamesOneByOne );

    CPPUNIT_TEST( testColorGreyConstructor );
    CPPUNIT_TEST( testColorRGBConstructor );
    CPPUNIT_TEST( testColorCMYKConstructor );

    CPPUNIT_TEST( testColorSeparationAllConstructor );
    CPPUNIT_TEST( testColorSeparationNoneConstructor );
    CPPUNIT_TEST( testColorSeparationConstructor );
    CPPUNIT_TEST( testColorCieLabConstructor );

    CPPUNIT_TEST( testRGBtoCMYKConversions );
    
    CPPUNIT_TEST_SUITE_END();

public:
    virtual void setUp();
    virtual void tearDown();

protected:
    void testDefaultConstructor();
    void testGreyConstructor();
    void testGreyConstructorInvalid();
    void testRGBConstructor();
    void testRGBConstructorInvalid();
    void testCMYKConstructor();
    void testCMYKConstructorInvalid();
    void testCopyConstructor();
    void testAssignmentOperator();
    void testEqualsOperator();
    void testHexNames();
    void testNamesGeneral();
    void testNamesOneByOne();

    void testColorGreyConstructor();
    void testColorRGBConstructor();
    void testColorCMYKConstructor();

    void testColorSeparationAllConstructor();
    void testColorSeparationNoneConstructor();
    void testColorSeparationConstructor();
    void testColorCieLabConstructor();

    void testRGBtoCMYKConversions();

    void testAssignNull();
};

#endif


