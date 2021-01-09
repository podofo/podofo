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
    if( pszDate != NULL )
    {
      CPPUNIT_ASSERT_EQUAL_MESSAGE(pszDate,bExpected,date.IsValid());
    }
    else
    {
      CPPUNIT_ASSERT_EQUAL_MESSAGE("NULL",bExpected,date.IsValid());
    }
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

    checkExpected("INVALID", false);
}

void DateTest::testDateValue()
{
    const char* pszDate = "D:20120530235959Z00'00'";
    PdfString tmp(pszDate);
    PdfDate date(tmp);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(std::string(pszDate),true,date.IsValid());
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

void DateTest::testAdditional()
{
  struct name_date {
    std::string name;
    std::string date;
  };

  const name_date data[] = {
			    {"sample from pdf_reference_1_7.pdf", "D:199812231952-08'00'"},
			    // UTC 1998-12-24 03:52:00
			    {"all fields set", "D:20201223195200-08'00'"},   // UTC 2020-12-03:52:00
			    {"set year", "D:2020"},   // UTC 2020-01-01 00:00:00
			    {"set year, month", "D:202001"},   // UTC 2020-01-01 00:00:00
			    {"set year, month, day", "D:20200101"},   // UTC 202001-01 00:00:00
			    {"only year and timezone set", "D:2020-08'00'"},   // UTC 2020-01-01 08:00:00
			    {"berlin", "D:20200315120820+01'00'"},   // UTC 2020-03-15 11:08:20
  };

  for (const auto& d : data) {
    std::cout << "Parse " << d.name << "\n";
    assert(PoDoFo::PdfDate(d.date).IsValid());
  }
}


void DateTest::testParseDateInvalid()
{
    PdfString tmp("D:2012020");
    PdfDate date(tmp);

    struct tm  _tm;
    memset (&_tm, 0, sizeof(struct tm));

    const time_t t = date.GetTime();

    CPPUNIT_ASSERT_EQUAL(false, date.IsValid());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Invalid date should be equal to time_t(-1)", time_t(-1), t);
}

void DateTest::testParseDateValid()
{
    PdfString tmp("D:20120205132456");
    PdfDate date(tmp);

    struct tm  _tm;
    memset (&_tm, 0, sizeof(struct tm));

    const time_t t = date.GetTime();
    localtime_r(&t, &_tm);

    CPPUNIT_ASSERT_EQUAL(true, date.IsValid());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Year", 2012, _tm.tm_year + 1900);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Month", 2, _tm.tm_mon + 1);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Day", 5, _tm.tm_mday);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Hour", 13, _tm.tm_hour);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Minute", 24, _tm.tm_min);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Second", 56, _tm.tm_sec);

}



