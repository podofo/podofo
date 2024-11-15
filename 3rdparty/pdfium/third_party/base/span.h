#pragma once

#include <span.hpp>

namespace pdfium
{
    template <class T, size_t Extent = tcb::dynamic_extent>
    using span = tcb::span<const T, Extent>;

    template <typename T, size_t N>
    constexpr span<T> make_span(T(&array)[N]) noexcept
    {
        return span<T>(array, N);
    }
}
