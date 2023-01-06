#ifndef FORMAT_COMPAT_H
#define FORMAT_COMPAT_H

#if __cplusplus >= 202002L

#include <format>

#else // __cplusplus < 202002L

#define FMT_HEADER_ONLY
#include <fmt/format.h>

namespace std
{
	using fmt::format;
	using fmt::format_to;
	using fmt::format_to_n;
}

#endif // __cplusplus

#endif // FORMAT_COMPAT_H
