#ifndef CHARCONV_COMPAT_H
#define CHARCONV_COMPAT_H

#include <charconv>

// Older gcc and clang may have no floating point from_chars
#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ < 11
#define WANT_FROM_CHARS
#define WANT_CHARS_FORMAT
#elif defined(__clang__)
#define WANT_FROM_CHARS
#endif

#if defined(WANT_CHARS_FORMAT) || defined(WANT_FROM_CHARS)
#include <fast_float.h>
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
        double& value, chars_format fmt)
    {
        auto ret = fast_float::from_chars(first, last, value, (fast_float::chars_format)fmt);
        return { ret.ptr, ret.ec };
    }
}

#endif // WANT_FROM_CHARS

#endif // CHARCONV_COMPAT_H
