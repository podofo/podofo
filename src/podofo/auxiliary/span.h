#ifndef AUX_SPAN_H
#define AUX_SPAN_H
#pragma once

#include <podofo/3rdparty/span.hpp>

namespace PoDoFo
{
    // https://stackoverflow.com/questions/56845801/what-happened-to-stdcspan
    /** Constant span
     */
    template <class T, size_t Extent = tcb::dynamic_extent>
    using cspan = tcb::span<const T, Extent>;

    /** Mutable span
     */
    template <class T, size_t Extent = tcb::dynamic_extent, typename std::enable_if<!std::is_const_v<T>, int>::type = 0>
    using mspan = tcb::span<T, Extent>;
}

#endif // AUX_SPAN_H
