#ifndef AUX_ENUM_FLAGS_H
#define AUX_ENUM_FLAGS_H

#include <type_traits>

// http://blog.bitwigglers.org/using-enum-classes-as-type-safe-bitmasks/
// https://gist.github.com/derofim/0188769131c62c8aff5e1da5740b3574

template<typename Enum>
struct EnableBitMaskOperators
{
    static const bool enable = false;
};

#define ENABLE_BITMASK_OPERATORS(x)  \
template<>                           \
struct EnableBitMaskOperators<x>     \
{                                    \
    static const bool enable = true; \
};

template<typename Enum>
constexpr typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type
operator &(Enum lhs, Enum rhs) noexcept
{
    using underlying = typename std::underlying_type<Enum>::type;
    return static_cast<Enum> (
        static_cast<underlying>(lhs) &
        static_cast<underlying>(rhs)
    );
}

template<typename Enum>
constexpr typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type
operator ^(Enum lhs, Enum rhs) noexcept
{
    using underlying = typename std::underlying_type<Enum>::type;
    return static_cast<Enum> (
        static_cast<underlying>(lhs) ^
        static_cast<underlying>(rhs)
    );
}

template<typename Enum>
constexpr typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type
operator ~(Enum rhs) noexcept
{
    
    using underlying = typename std::underlying_type<Enum>::type;
    return static_cast<Enum> (
        ~static_cast<underlying>(rhs)
    );
}

template<typename Enum>
constexpr typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type
operator |(Enum lhs, Enum rhs) noexcept
{
    using underlying = typename std::underlying_type<Enum>::type;
    return static_cast<Enum> (
        static_cast<underlying>(lhs) |
        static_cast<underlying>(rhs)
    );
}

template<typename Enum>
constexpr typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type&
operator &=(Enum& lhs, Enum rhs) noexcept
{
    using underlying = typename std::underlying_type<Enum>::type;
    lhs = static_cast<Enum> (
        static_cast<underlying>(lhs) &
        static_cast<underlying>(rhs)           
    );
    return lhs;
}

template<typename Enum>
constexpr typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type&
operator ^=(Enum& lhs, Enum rhs) noexcept
{
    using underlying = typename std::underlying_type<Enum>::type;
    lhs = static_cast<Enum> (
        static_cast<underlying>(lhs) ^
        static_cast<underlying>(rhs)           
    );
    return lhs;
}

template<typename Enum>
constexpr typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum&>::type
operator |=(Enum& lhs, Enum rhs) noexcept
{
    using underlying = typename std::underlying_type<Enum>::type;
    lhs = static_cast<Enum> (
        static_cast<underlying>(lhs) |
        static_cast<underlying>(rhs)           
    );
    return lhs;
}

#endif // AUX_ENUM_FLAGS_H
