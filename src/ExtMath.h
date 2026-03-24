#pragma once

#include <cmath>

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

// Because existing code calls std::sqrt, we offer
// an opt-in mechanism to allow per-type extensibility.
// This is typically useful in a case where a library
// calls std::sqrt with generic types, which works
// with float/double/int etc, but not with a
// user-defined type.
// Explicitly opting the user-defined type in for math
// extensibility forwards it to std::math::sqrt where
// user-defined overloads are allowed.

template<class T>
inline constexpr bool is_math_extensible = false;

// ------------------------------------------------
// Compatibility forwarding layer
// ------------------------------------------------

// Only if the type is explicitly enabled for it, we 
// look for a potential user-defined solution
template<class T>
constexpr auto sqrt(T&& x) -> decltype(math::sqrt(std::forward<T>(x)))
    requires is_math_extensible<std::remove_cvref_t<T>>
{
    return math::sqrt(std::forward<T>(x));
}

} // namespace std