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
    concept has_adl_sqrt = requires(T&& x)
    {
        sqrt(std::forward<T>(x));
    };

    template<class T>
    concept has_std_sqrt = requires(T&& x)
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

// Existing code sometimes calls std::sqrt directly with
//  generic types, which works for float/double/int but
// not user-defined types.
// Explicitly opting a type in forwards std::sqrt calls to
// std::math::sqrt, where user-defined overloads are found.

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