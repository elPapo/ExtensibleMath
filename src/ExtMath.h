#pragma once

#include <cmath>
#include <complex>

// It is UB to define anything in std, but this is
// for demonstration purposes only.
namespace std
{
namespace math
{

namespace __sqrt
{
    // Poison pill
    void sqrt(auto) = delete;

    template<class T>
    concept has_member_sqrt = requires(T&& x)
    {
        std::forward<T>(x).sqrt();
    };

    template<class T>
    concept has_adl_sqrt = !has_member_sqrt<T> && requires(T&& x)
    {
        sqrt(std::forward<T>(x));
    };

    template<class T>
    concept has_std_sqrt = !has_member_sqrt<T> && !has_adl_sqrt<T> && requires(T&& x)
    {
        std::sqrt(std::forward<T>(x));
    };

    struct __fn
    {
        template<class T>
        requires has_member_sqrt<T>
              || has_adl_sqrt<T>
              || has_std_sqrt<T>
        constexpr auto operator()(T&& x) const
        {
            if constexpr (has_member_sqrt<T>)
                return std::forward<T>(x).sqrt();
            else if constexpr (has_adl_sqrt<T>)
                return sqrt(std::forward<T>(x));
            else
                return std::sqrt(std::forward<T>(x));
        }
    };
}

inline namespace __cpo
{
    inline constexpr __sqrt::__fn sqrt{};
}
} // namespace math

// ------------------------------------------------
// Legacy domain rule
// ------------------------------------------------

// I believe I could only check for convertible to
// double and that would cover it all, but I want
// to be explicit for now.
// This is to preserve legacy behaviour.
template<class T>
concept legacy_sqrt_domain =
    std::convertible_to<T, float>
    || std::convertible_to<T, double>
    || std::convertible_to<T, long double>
    || std::convertible_to<T, std::complex<float>>
    || std::convertible_to<T, std::complex<double>>
    || std::convertible_to<T, std::complex<long double>>;
// if it can convert to an integral type, it can convert
// to a floating point type too

// Note that I don't test for std::simd for two
// reasons: I would have to include simd and that's
// a lot, but also there should not be any legacy
// code with std::simd since it's C++26.

template<class T>
inline constexpr bool is_math_extensible = false;

// ------------------------------------------------
// Compatibility forwarding layer
// ------------------------------------------------

// Only if the legacy can not handle it, we look
// for a potential user-defined solution
template<class T>
constexpr auto sqrt(T&& x) -> decltype(math::sqrt(std::forward<T>(x)))
    requires (!legacy_sqrt_domain<T>)
        && is_math_extensible<std::remove_cvref_t<T>>
{
    return math::sqrt(std::forward<T>(x));
}

} // namespace std