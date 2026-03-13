#include <concepts>
#include <iostream>
#include <type_traits>
#include <cmath>

#include "ExtMath.h"

template<typename T>
T length(T x, T y)
{
    // should be using namesapce std;
    // return sqrt(x*x + y*y);
    // but easy mistake to make
    return std::sqrt(x*x + y*y);
}

//some bigmath lib (or newtypes, or units or portable float...)
namespace bigmath
{
    struct bigint
    {
        int v;
    };

    bigint operator+(bigint a, bigint b) { return {a.v + b.v}; }
    bigint operator*(bigint a, bigint b) { return {a.v * b.v}; }

    constexpr bigint sqrt(bigint x) { return {x.v / 2}; } // fake sqrt
}

struct AmbiguousType
{
    double v;

    operator double() const
    {
        std::cout << "conversion to double\n";
        return v;
    }
};

double sqrt(AmbiguousType x)
{
    std::cout << "ADL\n";
    return std::sqrt(x.v);
}

int main()
{
    bigmath::bigint a{3};
    bigmath::bigint b{4};

    length(a, b);   // this would fail without ExtMath.h

    // called via legacy std::sqrt prefers compat unchanged behaviour
    std::sqrt(AmbiguousType{2.0});
    // called via new std::math::sqrt, prefers user-defined behaviour
    std::math::sqrt(AmbiguousType{2.0});
}