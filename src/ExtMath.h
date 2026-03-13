#pragma once

#include <cmath>
#include <complex>

// It is UB to define anything in std, but this is
// for demonstration purposes only.
namespace std
{

namespace math
{
// ------------------------------------------------
// New customizable math layer
// ------------------------------------------------


template<class T>
constexpr auto sqrt(T&& x)
{
    if constexpr (requires { std::forward<T>(x).sqrt(); })
        return std::forward<T>(x).sqrt();       // member customization
    else if constexpr (requires { sqrt(std::forward<T>(x)); })
        return sqrt(std::forward<T>(x));        // ADL only, no std in scope
    else
        return std::sqrt(std::forward<T>(x));   // std fallback
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


// ------------------------------------------------
// Compatibility forwarding layer
// ------------------------------------------------

// Only if the legacy can not handle it, we look
// for a potential user-defined solution
template<class T>
constexpr auto sqrt(T&& x)
requires (!legacy_sqrt_domain<T>)
{
    return math::sqrt(std::forward<T>(x));
}

} // namespace std