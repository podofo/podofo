#ifndef FORMAT_COMPAT_H
#define FORMAT_COMPAT_H

#if __cplusplus >= 202002L

#include <format>

#else // __cplusplus < 202002L

#include "podofo_config_private.h"

#ifndef PODOFO_DEVENDOR_FMT
// Include the library header only and rename the
// namespace, so it won't clash with other fmt copies
#define FMT_HEADER_ONLY
#define FMT_BEGIN_NAMESPACE \
    namespace fmt {           \
    inline namespace podofo {
#define FMT_END_NAMESPACE \
    }                       \
    }
#endif // PODOFO_DEVENDOR_FMT

#include <fmt/format.h>

namespace std
{
	using fmt::format;
	using fmt::format_to;
	using fmt::format_to_n;
}

#endif // __cplusplus

#endif // FORMAT_COMPAT_H
