/***************************************************************************
 *   Copyright (C) 2012 by Dominik Seichter                                *
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

#include "DateTest.h"
#include <podofo.h>

using namespace PoDoFo;

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( DateTest );

void DateTest::setUp()
{
}

void DateTest::tearDown()
{
}

void checkExpected(const char *pszDate, bool bExpected)
{
    PdfString tmp(pszDate);
    PdfDate date(tmp);
    CPPUNIT_ASSERT_EQUAL(bExpected,date.IsValid());
}

void DateTest::testCreateDateFromString()
{
    checkExpected(NULL,false);
    checkExpected("D:2012",true);
    checkExpected("D:20120",false);
    checkExpected("D:201201",true);
    checkExpected("D:2012010",false);
    checkExpected("D:20120101",true);
    checkExpected("D:201201012",false);
    checkExpected("D:2012010123",true);
    checkExpected("D:20120101235",false);
    checkExpected("D:201201012359",true);
    checkExpected("D:2012010123595",false);
    checkExpected("D:20120101235959",true);
    checkExpected("D:20120120135959Z",false);
    checkExpected("D:20120120135959Z0",false);
    checkExpected("D:20120120135959Z00",true);
    checkExpected("D:20120120135959Z00'",false);
    checkExpected("D:20120120135959Z00'0",false);
    checkExpected("D:20120120135959Z00'00",false);
    checkExpected("D:20120120135959Z00'00'",true);
}

void DateTest::testDateValue()
{
    PdfDate date(PdfString("D:20120530235959Z00'00'"));
    CPPUNIT_ASSERT_EQUAL(true,date.IsValid());
    const time_t &time = date.GetTime();
    struct tm  _tm;
    memset (&_tm, 0, sizeof(struct tm));
    _tm.tm_year = 2012-1900;
    _tm.tm_mon = 4;
    _tm.tm_mday = 30;
    _tm.tm_hour = 23;
    _tm.tm_min = 59;
    _tm.tm_sec = 59;
    time_t time2 = mktime(&_tm);
    CPPUNIT_ASSERT_EQUAL(true,time==time2);
}


