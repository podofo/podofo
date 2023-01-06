#ifndef CHRONO_COMPAT_H
#define CHRONO_COMPAT_H

#include <chrono>

#if __cplusplus < 202002L

#include <date/date.h>

namespace std
{
    namespace chrono
    {
        using local_seconds = date::local_seconds;
        using sys_seconds = date::sys_seconds;
        using local_days = date::local_days;
        using sys_days = date::sys_days;
        using years = date::years;
        using months = date::months;
        using weeks = date::weeks;
        using days = date::days;
        using year = date::year;
        using month = date::month;
        using day = date::day;
        using year_month_day = date::year_month_day;
        template <typename DurationT>
        using hh_mm_ss = date::hh_mm_ss<DurationT>;
    }
}

#endif // __cplusplus

#endif // CHRONO_COMPAT_H
