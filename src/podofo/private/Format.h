#ifndef FORMAT_H
#define FORMAT_H

#include "format_compat.h"

/** \def COMMON_FORMAT(fmt, ...)
 *
  * Format the string, if needed
 */
#define COMMON_FORMAT(fmt, ...) ::utls::FormatHelper::TryFormat(fmt, ##__VA_ARGS__)

namespace utls
{
    template <typename... Args>
    inline static std::string Format(const std::string_view& fmt, const Args&... args)
    {
        return std::format(fmt, args...);
    }

    template <typename... Args>
    inline static void FormatTo(std::string& dst, const std::string_view& fmt, const Args&... args)
    {
        dst.clear();
        std::format_to(std::back_inserter(dst), fmt, args...);
    }

    template <typename... Args>
    inline static void FormatTo(char* dst, size_t n, const std::string_view& fmt, const Args&... args)
    {
        std::format_to_n(dst, n, fmt, args...);
    }

    /** Helper class. Use COMMON_FORMAT macro instead
     */
    class FormatHelper
    {
    public:
        inline static std::string TryFormat(const std::string_view& str)
        {
            return (std::string)str;
        }

        inline static std::string TryFormat(const char* str)
        {
            return str;
        }

        inline static std::string TryFormat(const std::string& str)
        {
            return str;
        }

        inline static std::string TryFormat(std::string&& str)
        {
            return std::move(str);
        }

        template <typename... Args>
        inline static std::string TryFormat(const std::string_view& fmt, const Args&... args)
        {
            return std::format(fmt, args...);
        }
    };
}

#endif // FORMAT_H
