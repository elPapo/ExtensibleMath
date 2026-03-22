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
			// std::cout is not constexpr
            if consteval
            {
                return {v / 2.f}; // dummy constexpr sqrt implementation
            }
            std::cout << "ScalarWithMember::sqrt\n";
            return {std::sqrt(v)};
        }
    };
}

// ------------------------------------------------
// A custom type
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

// Legacy generic function: calls std::sqrt directly.
// Cannot be modified. Represents any library code
// that was written without extensibility in mind.
template<typename T>
T length(T x, T y)
{
    // should be using namesapce std;
    // return sqrt(x*x + y*y);
    // but easy mistake to make:
    return std::sqrt(x*x + y*y);
}

// calling length with bigmath::bigint would have failed
// but now we can choose to opt bigmath::bigint in for
// for custom math functions:
template<>
inline constexpr bool std::is_math_extensible<bigmath::bigint> = true;

// ------------------------------------------------
// A type that is ambiguous between ADL and conversion
// to double usable with std::sqrt to illustrate the
// explicit if constexpr dispatch
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
// Checking concept correctness:
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

    bigmath::bigint a{3};
    bigmath::bigint b{4};
	
    // bigmath::bigint opts in via is_math_extensible,
    // so std::sqrt forwards to bigmath::sqrt
	// Now this works:
    length(a, b);

    // ---- Subtle: ambiguous type ----

    // std::sqrt prefers legacy: conversion to double wins
	// ensuring no change in behaviour of existing code.
    std::sqrt(AmbiguousType{2.0});

    // std::math::sqrt prefers ADL: free function wins
    std::math::sqrt(AmbiguousType{2.0});

    // ---- constexpr ----
    constexpr bigmath::bigint d{16};
    constexpr auto result = std::math::sqrt(d);
    static_assert(result.v == 8);
}