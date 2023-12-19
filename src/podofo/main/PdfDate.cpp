/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfDate.h"

#include <algorithm>
#include <podofo/private/chrono_compat.h>

using namespace std;
using namespace PoDoFo;

#if _WIN32
#define timegm _mkgmtime
#endif

// a PDF date has a maximum of 24 bytes including the terminating \0
#define PDF_DATE_BUFFER_SIZE 24

// a W3C date has a maximum of 26 bytes including the terminating \0
#define W3C_DATE_BUFFER_SIZE 26

#define PEEK_DATE_CHAR(str, zoneShift)\
if (*str == '\0')\
{\
    goto End;\
}\
else if (tryReadShiftChar(str, zoneShift))\
{\
    goto ParseShift;\
}

#define PEEK_DATE_CHAR_W3C(str, separator)\
if (*str == '\0')\
{\
    goto End;\
}\
else if (*str != separator)\
{\
    return false;\
}\
else\
{\
    str++;\
}

static bool parseFixLenNumber(const char*& in, unsigned maxLength, int min, int max, int& ret);
static int getLocalOffesetFromUTCMinutes();
static bool tryReadShiftChar(const char*& in, int& zoneShift);
static bool tryParse(const string_view & dateStr, chrono::seconds & secondsFromEpoch, nullable<chrono::minutes>&minutesFromUtc);
static bool tryParseW3C(const string_view & dateStr, chrono::seconds & secondsFromEpoch, nullable<chrono::minutes>&minutesFromUtc);
static void inferTimeComponents(int y, int m, int d, int h, int M, int s,
    bool hasZoneShift, int zoneShift, int zoneHour, int zoneMin,
    chrono::seconds & secondsFromEpoch, nullable<chrono::minutes>&minutesFromUtc);
static chrono::seconds getSecondsFromEpoch();

PdfDate::PdfDate()
    : m_SecondsFromEpoch()
{ }

PdfDate::PdfDate(const chrono::seconds& secondsFromEpoch, const nullable<chrono::minutes>& offsetFromUTC)
    : m_SecondsFromEpoch(secondsFromEpoch), m_MinutesFromUtc(offsetFromUTC)
{
}

PdfDate PdfDate::LocalNow()
{
    auto minutesFromUtc = chrono::minutes(getLocalOffesetFromUTCMinutes());
    auto secondsFromEpoch = getSecondsFromEpoch();
    return PdfDate(secondsFromEpoch, minutesFromUtc);
}

PdfDate PdfDate::UtcNow()
{
    auto secondsFromEpoch = getSecondsFromEpoch();
    return PdfDate(secondsFromEpoch, { });
}

PdfDate PdfDate::Parse(const string_view& dateStr)
{
    PdfDate date;
    if (!TryParse(dateStr, date))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDataType, "Date is invalid");

    return date;
}

bool PdfDate::TryParse(const string_view& dateStr, PdfDate& date)
{
    chrono::seconds secondsFromEpoch;
    nullable<chrono::minutes> minutesFromUtc;
    if (!tryParse(dateStr, secondsFromEpoch, minutesFromUtc))
    {
        date = { };
        return false;
    }

    date = PdfDate(secondsFromEpoch, minutesFromUtc);
    return true;
}

PdfDate PdfDate::ParseW3C(const string_view& dateStr)
{
    PdfDate date;
    if (!TryParseW3C(dateStr, date))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDataType, "Date is invalid");

    return date;
}

bool PdfDate::TryParseW3C(const string_view& dateStr, PdfDate& date)
{
    chrono::seconds secondsFromEpoch;
    nullable<chrono::minutes> minutesFromUtc;
    if (!tryParseW3C(dateStr, secondsFromEpoch, minutesFromUtc))
    {
        date = { };
        return false;
    }

    date = PdfDate(secondsFromEpoch, minutesFromUtc);
    return true;
}

PdfString PdfDate::createStringRepresentation(bool w3cstring) const
{
    string offset;
    std::chrono::minutes minutesFromUtc{};
    if (m_MinutesFromUtc.has_value())
    {
        minutesFromUtc = *m_MinutesFromUtc;
        int minutesFromUtci = (int)minutesFromUtc.count();
        unsigned offseth = std::abs(minutesFromUtci) / 60;
        unsigned offsetm = std::abs(minutesFromUtci) % 60;
        if (minutesFromUtci == 0)
        {
            offset = "Z";
        }
        else
        {
            bool plus = minutesFromUtci > 0 ? true : false;
            if (w3cstring)
                utls::FormatTo(offset, "{}{:02}:{:02}", plus ? '+' : '-', offseth, offsetm);
            else
                utls::FormatTo(offset, "{}{:02}'{:02}'", plus ? '+' : '-', offseth, offsetm);
        }
    }

    // Decompose the date
    auto secondsFromEpoch = (chrono::sys_seconds)(m_SecondsFromEpoch + minutesFromUtc);
    auto dp = chrono::floor<chrono::days>(secondsFromEpoch);
    auto ymd = chrono::year_month_day(dp);
    auto time = chrono::hh_mm_ss<chrono::seconds>(chrono::floor<chrono::seconds>(secondsFromEpoch - dp));

    short y = (short)(int)ymd.year();
    unsigned char m = (unsigned char)(unsigned)ymd.month();
    unsigned char d = (unsigned char)(unsigned)ymd.day();
    unsigned char h = (unsigned char)time.hours().count();
    unsigned char M = (unsigned char)time.minutes().count();
    unsigned char s = (unsigned char)time.seconds().count();

    string date;
    if (w3cstring)
    {
        // e.g. "1998-12-23T19:52:07-08:00"
        utls::FormatTo(date, "{:04}-{:02}-{:02}T{:02}:{:02}:{:02}{}", y, m, d, h, M, s, offset);
    }
    else
    {
        // e.g. "D:19981223195207−08'00'"
        utls::FormatTo(date, "D:{:04}{:02}{:02}{:02}{:02}{:02}{}", y, m, d, h, M, s, offset);
    }

    return PdfString(date);
}

PdfString PdfDate::ToString() const
{
    return createStringRepresentation(false);
}

PdfString PdfDate::ToStringW3C() const
{
    return createStringRepresentation(true);
}

bool PdfDate::operator==(const PdfDate& rhs) const
{
    return m_MinutesFromUtc == rhs.m_MinutesFromUtc && m_SecondsFromEpoch == rhs.m_SecondsFromEpoch;
}

bool PdfDate::operator!=(const PdfDate& rhs) const
{
    return m_MinutesFromUtc != rhs.m_MinutesFromUtc || m_SecondsFromEpoch != rhs.m_SecondsFromEpoch;
}

bool tryParse(const string_view& dateStr, chrono::seconds& secondsFromEpoch, nullable<chrono::minutes>& minutesFromUtc)
{
    int y = 0;
    int m = 0;
    int d = 0;
    int h = 0;
    int M = 0;
    int s = 0;

    bool hasZoneShift = false;
    int zoneShift = 0;
    int zoneHour = 0;
    int zoneMin = 0;

    const char* cursor = dateStr.data();
    if (cursor == nullptr)
        return false;

    if (*cursor == 'D')
    {
        cursor++;
        if (*cursor++ != ':')
            return false;
    }

    PEEK_DATE_CHAR(cursor, zoneShift);

    if (!parseFixLenNumber(cursor, 4, 0, 9999, y))
        return false;

    PEEK_DATE_CHAR(cursor, zoneShift);

    if (!parseFixLenNumber(cursor, 2, 1, 12, m))
        return false;

    PEEK_DATE_CHAR(cursor, zoneShift);

    if (!parseFixLenNumber(cursor, 2, 1, 31, d))
        return false;

    PEEK_DATE_CHAR(cursor, zoneShift);

    if (!parseFixLenNumber(cursor, 2, 0, 23, h))
        return false;

    PEEK_DATE_CHAR(cursor, zoneShift);

    if (!parseFixLenNumber(cursor, 2, 0, 59, M))
        return false;

    PEEK_DATE_CHAR(cursor, zoneShift);

    if (!parseFixLenNumber(cursor, 2, 0, 59, s))
        return false;

    PEEK_DATE_CHAR(cursor, zoneShift);

ParseShift:
    hasZoneShift = true;
    if (*cursor != '\0')
    {
        if (!parseFixLenNumber(cursor, 2, 0, 59, zoneHour))
            goto End;

        if (*cursor == '\'')
        {
            cursor++;
            if (*cursor != '\0')
            {
                if (!parseFixLenNumber(cursor, 2, 0, 59, zoneMin))
                    return false;

                if (*cursor == '\'')
                    cursor++;
            }
        }

        if (zoneShift == 0 && (zoneHour != 0 || zoneMin != 0))
            return false;

        if (*cursor != '\0')
            return false;
    }

End:
    inferTimeComponents(y, m, d, h, M, s,
        hasZoneShift, zoneShift, zoneHour, zoneMin,
        secondsFromEpoch, minutesFromUtc);

    return true;
}

bool tryParseW3C(const string_view& dateStr, chrono::seconds& secondsFromEpoch, nullable<chrono::minutes>& minutesFromUtc)
{
    int y = 0;
    int m = 0;
    int d = 0;
    int h = 0;
    int M = 0;
    int s = 0;

    bool hasZoneShift = false;
    int zoneShift = 0;
    int zoneHour = 0;
    int zoneMin = 0;

    const char* cursor = dateStr.data();
    if (cursor == nullptr || *cursor == '\0')
        return false;

    if (!parseFixLenNumber(cursor, 4, 0, 9999, y))
        return false;

    PEEK_DATE_CHAR_W3C(cursor, '-');

    if (!parseFixLenNumber(cursor, 2, 1, 12, m))
        return false;

    PEEK_DATE_CHAR_W3C(cursor, '-');

    if (!parseFixLenNumber(cursor, 2, 1, 31, d))
        return false;

    PEEK_DATE_CHAR_W3C(cursor, 'T');

    if (!parseFixLenNumber(cursor, 2, 0, 23, h))
        return false;

    PEEK_DATE_CHAR_W3C(cursor, ':');

    if (!parseFixLenNumber(cursor, 2, 0, 59, M))
        return false;

    if (tryReadShiftChar(cursor, zoneShift))
        goto ParseShift;

    PEEK_DATE_CHAR_W3C(cursor, ':');

    if (!parseFixLenNumber(cursor, 2, 0, 59, s))
        return false;

    if (!tryReadShiftChar(cursor, zoneShift))
        goto End;

ParseShift:
    hasZoneShift = true;
    if (*cursor != '\0')
    {
        if (!parseFixLenNumber(cursor, 2, 0, 59, zoneHour))
            goto End;

        if (*cursor == ':')
        {
            cursor++;
            if (*cursor != '\0')
            {
                if (!parseFixLenNumber(cursor, 2, 0, 59, zoneMin))
                    return false;
            }
        }

        if (zoneShift == 0 && (zoneHour != 0 || zoneMin != 0))
            return false;

        if (*cursor != '\0')
            return false;
    }

End:
    inferTimeComponents(y, m, d, h, M, s,
        hasZoneShift, zoneShift, zoneHour, zoneMin,
        secondsFromEpoch, minutesFromUtc);

    return true;
}

void inferTimeComponents(int y, int m, int d, int h, int M, int s,
    bool hasZoneShift, int zoneShift, int zoneHour, int zoneMin,
    chrono::seconds& secondsFromEpoch, nullable<chrono::minutes>& minutesFromUtc)
{
    secondsFromEpoch = (chrono::local_days(chrono::year(y) / m / d) + chrono::hours(h) + chrono::minutes(M) + chrono::seconds(s)).time_since_epoch();
    if (hasZoneShift)
    {
        minutesFromUtc = zoneShift * (chrono::hours(zoneHour) + chrono::minutes(zoneMin));
        secondsFromEpoch -= *minutesFromUtc;
    }
    else
    {
        // ISO 32000-1:2008, 7.9.4 Dates:
        // "If no UT information is specified, the relationship of
        // the specified time to UT shall be considered to be GMT"
        // NOTE: UTC is the successor of GMT and in PDF context
        // can be assumed to the same
        minutesFromUtc = { };
    }
}

chrono::seconds getSecondsFromEpoch()
{
    // Cast now() to seconds. We assume system_clock epoch is
    //  always 1970/1/1 UTC in all platforms, like in C++20
    auto now = chrono::time_point_cast<chrono::seconds>(chrono::system_clock::now());
    // We forget about relationship with UTC, convert to local seconds
    return chrono::seconds(now.time_since_epoch());
}

// degski, https://stackoverflow.com/a/57520245/213871
// We use this function because we don't have C++20
// date library runtime support
int getLocalOffesetFromUTCMinutes()
{
    time_t t = time(nullptr);
    struct tm* locg = localtime(&t);
    struct tm locl;
    memcpy(&locl, locg, sizeof(struct tm));
    return (int)(timegm(locg) - mktime(&locl)) / 60;
}

bool tryReadShiftChar(const char*& in, int& zoneShift)
{
    switch (*in)
    {
        case '+':
            zoneShift = 1;
            break;
        case '-':
            zoneShift = -1;
            break;
        case 'Z':
            zoneShift = 0;
            break;
        default:
            return false;
    }

    in++;
    return true;
}

bool parseFixLenNumber(const char*& in, unsigned maxLength, int min, int max, int& ret)
{
    ret = 0;
    for (unsigned i = 0; i < maxLength; i++)
    {
        if (in == nullptr || !isdigit(*in))
            return i != 0;

        ret = ret * 10 + (*in - '0');
        in++;
    }
    if (ret < min || ret > max)
        return false;

    return true;
}
