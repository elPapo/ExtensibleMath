#include <concepts>
#include <iostream>
#include <type_traits>
#include <cmath>
#include "ExtMath.h"

// ------------------------------------------------
// A simple custom numeric type
// with a free function sqrt in its namespace
// ------------------------------------------------
namespace mylib
{
    struct Scalar
    {
        double v;
    };

    Scalar sqrt(Scalar x)
    {
        std::cout << "mylib::sqrt\n";
        return {std::sqrt(x.v)};
    }
}

// ------------------------------------------------
// A type with a member sqrt
// ------------------------------------------------
namespace mylib
{
    struct ScalarWithMember
    {
        double v;
        constexpr ScalarWithMember sqrt() const
        {
            if consteval
            {
                return {v / 2.f}; // dummy constexpr sqrt implementation
            }
            std::cout << "ScalarWithMember::sqrt\n";
            return {std::sqrt(v)}; // dummy sqrt
        }
    };
}

// ------------------------------------------------
// A legacy third-party type we cannot modify
// We opt it in to the compatibility layer
// ------------------------------------------------
namespace bigmath
{
    struct bigint
    {
        int v;
    };
    bigint operator+(bigint a, bigint b) { return {a.v + b.v}; }
    bigint operator*(bigint a, bigint b) { return {a.v * b.v}; }
    constexpr bigint sqrt(bigint x) { return {x.v / 2}; }
}

template<>
inline constexpr bool std::is_math_extensible<bigmath::bigint> = true;

// ------------------------------------------------
// A type that is ambiguous between ADL and std::
// to illustrate the explicit if constexpr dispatch
// ------------------------------------------------
struct AmbiguousType
{
    double v;
    operator double() const
    {
        std::cout << "AmbiguousType: conversion to double\n";
        return v;
    }
};

double sqrt(AmbiguousType x)
{
    std::cout << "AmbiguousType: ADL\n";
    return std::sqrt(x.v);
}

// ------------------------------------------------
// Concept correctness: std::math::sqrt is
// SFINAE-friendly thanks to the CPO design
// ------------------------------------------------
template<class T>
concept has_sqrt = requires(T x)
{
    std::math::sqrt(x);
};

static_assert(has_sqrt<double>);                      // std fallback
static_assert(has_sqrt<mylib::Scalar>);               // ADL
static_assert(has_sqrt<mylib::ScalarWithMember>);     // member
static_assert(has_sqrt<bigmath::bigint>);             // ADL
static_assert(!has_sqrt<std::string>);                // correctly rejected

int main()
{
    // ---- Basic cases ----

    // ADL dispatch
    mylib::Scalar s{4.0};
    std::math::sqrt(s);

    // Member dispatch
    mylib::ScalarWithMember sm{4.0};
    std::math::sqrt(sm);

    // std fallback
    std::math::sqrt(4.0);

    // ---- Legacy compatibility layer ----

    // bigmath::bigint opts in via is_math_extensible,
    // so std::sqrt forwards to bigmath::sqrt
    bigmath::bigint a{9};
    std::sqrt(a);

    // ---- Generic code illustration ----

    // Without ExtMath.h, this would call std::sqrt directly
    // and fail for bigmath::bigint
    auto length = [](auto x, auto y)
    {
        // should be: using std::sqrt; return sqrt(...)
        // but the easy mistake is:
        return std::sqrt(x*x + y*y); // fails for bigmath::bigint without ExtMath.h
    };

    bigmath::bigint b{3}, c{4};
    length(b, c);

    // ---- Subtle: ambiguous type ----

    // std::sqrt prefers legacy: conversion to double wins
    std::sqrt(AmbiguousType{2.0});

    // std::math::sqrt prefers ADL: free function wins
    std::math::sqrt(AmbiguousType{2.0});

    // ---- constexpr ----
    constexpr bigmath::bigint d{16};
    constexpr auto result = std::math::sqrt(d);
    static_assert(result.v == 8);
}