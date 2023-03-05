#ifndef COMMON_SPAN_H
#define COMMON_SPAN_H
#pragma once

#include "span.hpp"

namespace COMMON_NAMESPACE
{
    // https://stackoverflow.com/questions/56845801/what-happened-to-stdcspan
    /** Constant span
     */
    template <class T, size_t Extent = tcb::dynamic_extent>
    using cspan = tcb::span<const T, Extent>;

    /** Mutable span
     */
    template <class T, size_t Extent = tcb::dynamic_extent, typename std::enable_if<!std::is_const<T>::value, int>::type = 0>
    using mspan = tcb::span<T, Extent>;
}

#endif // COMMON_SPAN_H
