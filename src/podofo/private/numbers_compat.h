#ifndef COMPAT_NUMBERS_H
#define COMPAT_NUMBERS_H

#if __cplusplus >= 202002L

#include <numbers>

#else // __cplusplus < 202002L

namespace std::numbers
{
	constexpr double pi = 3.14159265358979323846;
}

#endif // __cplusplus >= 202002L

#endif // COMPAT_NUMBERS_H
