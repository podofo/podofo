/**
 * Copyright (C) 20012 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <PdfTest.h>

#include <date/date.h>

using namespace std;
using namespace PoDoFo;

static void deconstruct(const PdfDate& date, short& y, unsigned char& m, unsigned char& d,
    unsigned char& h, unsigned char& M, unsigned char& s);
static void deconstruct(const PdfDate& date, date::year_month_day& ymd, date::hh_mm_ss<chrono::seconds>& time);

static void checkExpected(const string_view& datestr, bool expectedValid)
{
    auto now = PdfDate::LocalNow();
    bool valid = PdfDate::TryParse(datestr, now);

    if (datestr.empty())
    {
        INFO("NULL");
    }
    else
    {
        INFO(datestr);
    }

    REQUIRE(valid == expectedValid);
}

TEST_CASE("TestCreateDateFromString")
{
    checkExpected({ }, false);
    checkExpected("D:2012", true);
    checkExpected("D:20120", true);
    checkExpected("D:201201", true);
    checkExpected("D:201213", false);
    checkExpected("D:2012010", true);
    checkExpected("D:20120101", true);
    checkExpected("D:201201012", true);
    checkExpected("D:20120132", false);
    checkExpected("D:2012010123", true);
    checkExpected("D:2012010125", false);
    checkExpected("D:20120101235", true);
    checkExpected("D:201201012359", true);
    checkExpected("D:2012010123595", true);
    checkExpected("D:20120101235959", true);
    checkExpected("D:20120120135959Z", true);
    checkExpected("D:20120120135959Z00", true);
    checkExpected("D:20120120135959Z00'", true);
    checkExpected("D:20120120135959Z00'00", true);
    checkExpected("D:20120120135959Z00'00'", true);
    checkExpected("D:20120120135959+0", true);
    checkExpected("D:20120120135959+00", true);
    checkExpected("D:20120120135959+00'", true);
    checkExpected("D:20120120135959+00'0", true);
    checkExpected("D:20120120135959+00'00", true);
    checkExpected("D:20120120135959-00'00", true);

    checkExpected("INVALID", false);
}

TEST_CASE("TestRoundTrip")
{
    auto testRoundTrip = [](const string_view& dateStr1)
    {
        auto date1 = PdfDate::Parse(dateStr1);
        string dateStr2 = (string)date1.ToString().GetString();
        auto date2 = PdfDate::Parse(dateStr2);
        REQUIRE(dateStr1 == dateStr2);
        REQUIRE(date1 == date2);
    };

    testRoundTrip("D:20221217220858+01'00'");
    testRoundTrip("D:20221217220858");
}

TEST_CASE("TestNoZoneShift")
{
    auto date1 = PdfDate::Parse("D:20221217220858+00'00'");
    auto date2 = PdfDate::Parse("D:20221217220858");
    // No zone shift should be equivalent to UTC
    REQUIRE(date1.GetSecondsFromEpoch() == date2.GetSecondsFromEpoch());
}

TEST_CASE("TestAdditional")
{
    struct name_date
    {
        string name;
        string date;
    };

    name_date data[] = {
        {"sample from pdf_reference_1_7.pdf", "D:199812231952-08'00'"},
        // UTC 1998-12-24 03:52:00
        {"all fields set", "D:20201223195200-08'00'"},   // UTC 2020-12-03:52:00
        {"set year", "D:2020"},   // UTC 2020-01-01 00:00:00
        {"set year, month", "D:202001"},   // UTC 2020-01-01 00:00:00
        {"set year, month, day", "D:20200101"},   // UTC 202001-01 00:00:00
        {"only year and timezone set", "D:2020-08'00'"},   // UTC 2020-01-01 08:00:00
        {"berlin", "D:20200315120820+01'00'"},   // UTC 2020-03-15 11:08:20
    };

    for (auto& d : data)
    {
        INFO(utls::Format("Parse {}", d.name));
        checkExpected(d.date, true);
    }
}

TEST_CASE("TestParseDateValid")
{
    // (Sun Feb 05 2012 13:24:56 GMT+0000)
    auto date = PdfDate::Parse("D:20120205132456");

    short y; unsigned char m, d, h, M, s;
    deconstruct(date, y, m, d, h, M, s);

    INFO("Year"); REQUIRE(y == 2012);
    INFO("Month"); REQUIRE(m == 2);
    INFO("Day"); REQUIRE(d == 5);
    INFO("Hour"); REQUIRE(h == 13);
    INFO("Minute"); REQUIRE(M == 24);
    INFO("Second"); REQUIRE(s == 56);

    unsigned timeExpected = 1328448296;
    REQUIRE(date.GetSecondsFromEpoch().count() == timeExpected);

    PdfDate date2 = PdfDate::Parse("D:20120205192456+06'00'");
    REQUIRE(date2.GetSecondsFromEpoch().count() == timeExpected);

    PdfDate date3 = PdfDate::Parse("D:20120205072456-06'00'");
    REQUIRE(date3.GetSecondsFromEpoch().count() == timeExpected);

    PdfDate date4 = PdfDate::Parse("D:20120205175456+04'30'");
    REQUIRE(date4.GetSecondsFromEpoch().count() == timeExpected);
}


static void deconstruct(const PdfDate& date, short& y, unsigned char& m, unsigned char& d,
    unsigned char& h, unsigned char& M, unsigned char& s)
{

    date::year_month_day ymd;
    date::hh_mm_ss<chrono::seconds> time;
    deconstruct(date, ymd, time);

    y = (short)(int)ymd.year();
    m = (unsigned char)(unsigned)ymd.month();
    d = (unsigned char)(unsigned)ymd.day();
    h = (unsigned char)time.hours().count();
    M = (unsigned char)time.minutes().count();
    s = (unsigned char)time.seconds().count();
}

void deconstruct(const PdfDate& date, date::year_month_day& ymd, date::hh_mm_ss<chrono::seconds>& time)
{
    auto minutesFromUtc = date.GetMinutesFromUtc();
    if (minutesFromUtc.has_value())
    {
        // Assume sys time
        auto secondsFromEpoch = (date::sys_seconds)(date.GetSecondsFromEpoch() + *minutesFromUtc);
        auto dp = date::floor<date::days>(secondsFromEpoch);
        ymd = date::year_month_day(dp);
        time = date::hh_mm_ss<chrono::seconds>(chrono::floor<chrono::seconds>(secondsFromEpoch - dp));
    }
    else
    {
        // Assume local time
        auto secondsFromEpoch = (date::local_seconds)date.GetSecondsFromEpoch();
        auto dp = date::floor<date::days>(secondsFromEpoch);
        ymd = date::year_month_day(dp);
        time = date::hh_mm_ss<chrono::seconds>(chrono::floor<chrono::seconds>(secondsFromEpoch - dp));
    }
}
