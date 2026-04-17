// SPDX-FileCopyrightText: 2005 Dominik Seichter <domseichter@web.de>
// SPDX-FileCopyrightText: 2020 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfDate.h"

#include <algorithm>
#include <podofo/private/chrono_compat.h>

using namespace std;
using namespace PoDoFo;

#if _WIN32
#define timegm _mkgmtime
#endif

static bool parseFixLenNumber(const string_view& str, size_t& pos, unsigned maxLength, int min, int max, int& ret);
static int getLocalOffesetFromUTCMinutes();
static bool tryReadShiftChar(const string_view& str, size_t& pos, int& zoneShift);
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
    if (dateStr.empty())
        return false;

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

    size_t pos = 0;

    // Skip optional "D:" prefix
    if (dateStr[pos] == 'D')
    {
        pos++;
        if (pos >= dateStr.size() || dateStr[pos] != ':')
            return false;
        pos++;
    }

    // Parse each field; between fields check for end-of-input or zone shift
    struct { unsigned len; int min; int max; int* out; } fields[] = {
        { 4, 0, 9999, &y }, { 2, 1, 12, &m }, { 2, 1, 31, &d },
        { 2, 0, 23, &h },   { 2, 0, 59, &M }, { 2, 0, 59, &s },
    };

    for (auto& f : fields)
    {
        if (pos >= dateStr.size())
            goto End;
        if (tryReadShiftChar(dateStr, pos, zoneShift))
            goto ParseShift;
        if (!parseFixLenNumber(dateStr, pos, f.len, f.min, f.max, *f.out))
            return false;
    }

    // After all fields, check for trailing shift or end
    if (pos >= dateStr.size())
        goto End;
    if (!tryReadShiftChar(dateStr, pos, zoneShift))
        return false;

ParseShift:
    hasZoneShift = true;
    if (pos < dateStr.size())
    {
        if (!parseFixLenNumber(dateStr, pos, 2, 0, 59, zoneHour))
            goto End;

        if (pos < dateStr.size() && dateStr[pos] == '\'')
        {
            pos++;
            if (pos < dateStr.size())
            {
                if (!parseFixLenNumber(dateStr, pos, 2, 0, 59, zoneMin))
                    return false;

                if (pos < dateStr.size() && dateStr[pos] == '\'')
                    pos++;
            }
        }

        if (zoneShift == 0 && (zoneHour != 0 || zoneMin != 0))
            return false;

        if (pos < dateStr.size())
            return false;
    }

End:
    inferTimeComponents(y, m, d, h, M, s,
        hasZoneShift, zoneShift, zoneHour, zoneMin,
        secondsFromEpoch, minutesFromUtc);

    return true;
}

// Helper: consume a specific separator character at the current position.
enum class PeekResult { Continue, AtEnd, Mismatch };

static PeekResult tryReadSeparator(const string_view& str, size_t& pos, char separator)
{
    if (pos >= str.size())
        return PeekResult::AtEnd;
    if (str[pos] != separator)
        return PeekResult::Mismatch;
    pos++;
    return PeekResult::Continue;
}

bool tryParseW3C(const string_view& dateStr, chrono::seconds& secondsFromEpoch, nullable<chrono::minutes>& minutesFromUtc)
{
    if (dateStr.empty())
        return false;

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

    size_t pos = 0;

    if (!parseFixLenNumber(dateStr, pos, 4, 0, 9999, y))
        return false;

    // Parse: -MM-DDThh:mm
    struct { char sep; unsigned len; int min; int max; int* out; } fields[] = {
        { '-', 2, 1, 12, &m }, { '-', 2, 1, 31, &d },
        { 'T', 2, 0, 23, &h }, { ':', 2, 0, 59, &M },
    };

    for (auto& f : fields)
    {
        auto r = tryReadSeparator(dateStr, pos, f.sep);
        if (r == PeekResult::AtEnd)
            goto End;
        if (r == PeekResult::Mismatch)
            return false;
        if (!parseFixLenNumber(dateStr, pos, f.len, f.min, f.max, *f.out))
            return false;
    }

    // After minutes: optional zone shift or :ss
    if (tryReadShiftChar(dateStr, pos, zoneShift))
        goto ParseShift;

    {
        auto r = tryReadSeparator(dateStr, pos, ':');
        if (r == PeekResult::AtEnd)
            goto End;
        if (r == PeekResult::Mismatch)
            return false;
    }

    if (!parseFixLenNumber(dateStr, pos, 2, 0, 59, s))
        return false;

    if (!tryReadShiftChar(dateStr, pos, zoneShift))
        goto End;

ParseShift:
    hasZoneShift = true;
    if (pos < dateStr.size())
    {
        if (!parseFixLenNumber(dateStr, pos, 2, 0, 59, zoneHour))
            goto End;

        if (pos < dateStr.size() && dateStr[pos] == ':')
        {
            pos++;
            if (pos < dateStr.size())
            {
                if (!parseFixLenNumber(dateStr, pos, 2, 0, 59, zoneMin))
                    return false;
            }
        }

        if (zoneShift == 0 && (zoneHour != 0 || zoneMin != 0))
            return false;

        if (pos < dateStr.size())
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

bool tryReadShiftChar(const string_view& str, size_t& pos, int& zoneShift)
{
    if (pos >= str.size())
        return false;

    switch (str[pos])
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

    pos++;
    return true;
}

bool parseFixLenNumber(const string_view& str, size_t& pos, unsigned maxLength, int min, int max, int& ret)
{
    ret = 0;
    for (unsigned i = 0; i < maxLength; i++)
    {
        if (pos >= str.size() || !std::isdigit(static_cast<unsigned char>(str[pos])))
            return i != 0;

        ret = ret * 10 + (str[pos] - '0');
        pos++;
    }
    if (ret < min || ret > max)
        return false;

    return true;
}
