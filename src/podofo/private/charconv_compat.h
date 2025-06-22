#ifndef CHARCONV_COMPAT_H
#define CHARCONV_COMPAT_H

#include <charconv>

// Older gcc and clang may have not have floating point std::from_chars/std::to_chars
#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ < 10
#define WANT_CHARS_FORMAT
#endif
#if (defined(__GNUC__) && !defined(__clang__) && !defined(__MINGW32__) &&  __GNUC__ < 11) || (defined(__MINGW32__) &&  __GNUC__ < 12) || (defined(__clang__) && defined(_LIBCPP_VERSION) && (_LIBCPP_VERSION < 200000))
#define WANT_FROM_CHARS
#endif
#if (defined(__GNUC__) && !defined(__clang__) && !defined(__MINGW32__) &&  __GNUC__ < 11) || (defined(__MINGW32__) &&  __GNUC__ < 12) || (defined(__clang__) && ((defined(__apple_build_version__) && __apple_build_version__ < 15000000) || __clang_major__ < 14))
#define WANT_TO_CHARS
#endif

#if defined(WANT_CHARS_FORMAT) || defined(WANT_FROM_CHARS)
#include <fast_float.h>
#endif

#ifdef WANT_TO_CHARS
#include "format_compat.h"
#endif

#ifdef WANT_CHARS_FORMAT

namespace std
{
    using chars_format = fast_float::chars_format;
}

#endif // WANT_CHARS_FORMAT

#ifdef WANT_FROM_CHARS

namespace std
{
    // NOTE: Don't provide an alias using default
    // parameter since it may create mysterious
    // issues on clang
    inline from_chars_result from_chars(const char* first, const char* last,
        double& value, chars_format fmt) noexcept
    {
        auto ret = fast_float::from_chars(first, last, value, (fast_float::chars_format)fmt);
        return { ret.ptr, ret.ec };
    }
}

#endif // WANT_FROM_CHARS

#ifdef WANT_TO_CHARS

namespace std
{
    inline to_chars_result to_chars(char* first, char* last,
        double value, chars_format fmt, int precision) noexcept
    {
        // NOTE: Only chars_format::fixed is supported
        (void)fmt;
        size_t n = last - first;
        auto ret = std::format_to_n(first, n, "{:.{}f}", value, precision);
        if (ret.size > n)
            return to_chars_result{ first + n, errc::value_too_large };
        else
            return to_chars_result{ first + ret.size, errc{} };
    }

    inline to_chars_result to_chars(char* first, char* last,
        float value, chars_format fmt, int precision) noexcept
    {
        // NOTE: Only chars_format::fixed is supported
        (void)fmt;
        size_t n = last - first;
        auto ret = std::format_to_n(first, n, "{:.{}f}", value, precision);
        if (ret.size > n)
            return to_chars_result{ first + n, errc::value_too_large };
        else
            return to_chars_result{ first + ret.size, errc{} };
    }
}

#endif // WANT_TO_CHARS

#endif // CHARCONV_COMPAT_H
