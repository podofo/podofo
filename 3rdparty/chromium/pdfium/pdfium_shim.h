#pragma once

// This is a shim of several macros/functions/types
// that is needed to compile stripped pdium code

#include <cstring>
#include <vector>
#include <algorithm>
#include <numerics/safe_math.h>

#define DCHECK(cond)
#define DCHECK_EQ(x, y)

namespace fxcrt
{
    template <typename T>
    using DataVector = std::vector<T>;
}

using fxcrt::DataVector;

// Round up to the power-of-two boundary N.
template <int N, typename T>
inline T FxAlignToBoundary(T size) {
    static_assert(N > 0 && (N & (N - 1)) == 0, "Not non-zero power of two");
    return (size + (N - 1)) & ~(N - 1);
}

namespace pdfium
{
    template <typename T>
    constexpr const T& clamp(const T& v, const T& lo, const T& hi)
    {
        return v < lo ? lo : hi < v ? hi : v;
    }

    namespace base = chromium::base;
}

using FX_SAFE_UINT32 = chromium::base::CheckedNumeric<uint32_t>;

namespace fxge
{
    inline FX_SAFE_UINT32 CalculatePitch32Safely(int bpp, int width)
    {
        FX_SAFE_UINT32 pitch = bpp;
        pitch *= width;
        pitch += 31;
        pitch /= 32;  // quantized to number of 32-bit words.
        pitch *= 4;   // and then back to bytes, (not just /8 in one step).
        return pitch;
    }

    inline uint32_t CalculatePitch32OrDie(int bpp, int width) {
        return CalculatePitch32Safely(bpp, width).ValueOrDie();
    }
}
