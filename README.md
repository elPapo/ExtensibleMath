# Extensible Math Functions for C++

## Abstract
 
This paper proposes a direction for making standard library mathematical functions
extensible via a new `std::math` sub-namespace, using `std::math::sqrt` as a
representative example. No wording is proposed at this stage. The goal is to gauge
committee appetite for the direction before committing to a full proposal covering
the entire `<cmath>` surface.

---
 
## Motivation
 
### The Problem
 
The C++ standard library provides mathematical functions such as `sqrt` and `abs`
in the `std` namespace. These functions are not customization points: calling
`std::sqrt(x)` directly bypasses any user-defined overload, even if one exists for
the type of `x`.
 
The idiomatic workaround is to enable ADL via a `using` declaration:
 
```cpp
using std::sqrt;
return sqrt(x);
```
 
This allows a user-defined `sqrt` in the same namespace as `x` to be found via ADL,
while falling back to `std::sqrt` for built-in types. However, this idiom has a
critical limitation: it requires a statement, and is therefore unavailable in
contexts that only accept expressions.
 
---

**Constructor member initializer lists:**
 
```cpp
MyType(A aSq, B b, C c)
    : a(sqrt(aSq)),   // ill-formed: sqrt not found
      b(b),
      c(abs(c))     // ill-formed: abs not found
{}
```
 
The same applies to default member initializers:
 
```cpp
struct Foo {
    double x = sqrt(v);  // ill-formed: sqrt not found
};
```
 
**Requires expressions:**
 
```cpp
template<class T>
concept numeric = requires(T x) {
    { abs(x) };  // ill-formed for custom types
};
```
 
There are other, less obvious situations where the same limitation applies,
such as constant expressions (array size from a new-type) and template arguments.

---

In all of these cases, the programmer faces the same three unsatisfactory choices:
 
**Option 1: Call `std::` explicitly**
 
```cpp
MyType(A a, B b, C c)
    : a(std::sqrt(a)),  // silently breaks extensibility
      b(b),
      c(std::abs(c))
{}
```
 
This compiles and works for built-in types, but silently cuts off any user-defined
overload.
 
**Option 2: Restructure to allow a statement**
 
For member initializer lists, this means moving initialization to the constructor
body:
 
```cpp
MyType(A a, B b, C c)
    : b(b)
{
    using namespace std;
    this->a = sqrt(a);  // requires a to be default-constructible
    this->c = abs(c);   // loses const and reference member support
}
```
 
This restores ADL but members can no longer be `const` or
references, and all members must be default-constructible. Not all contexts can
be restructured this way at all.
 
**Option 3: Write boilerplate helper functions**
 
```cpp
template<class T> auto my_sqrt(T&& x) {
    using std::sqrt;
    return sqrt(std::forward<T>(x));
}
 
MyType(A a, B b, C c)
    : a(my_sqrt(a)),
      b(b),
      c(my_abs(c))
{}
```
 
This restores extensibility but requires every author of generic code to write and
maintain their own dispatch layer; one wrapper per math function, 45+ to be exhaustive.
The ownership of these wrappers is unclear. Custom numeric-type authors should probably
embed them in their math function implementations, but authors of generic code using such functions
cannot rely on this being provided and may need to implement them redundantly to support a wider 
range of types.
The existence of multiple widely-used libraries implementing exactly this machinery
(see Prior Art below) is evidence that there is real use-cases and that the status quo is 
encouraging unnecessary duplication.

---
 
### Existing Practice
 
Multiple independent, widely-used C++ libraries have been confronted with this
problem and have each arrived at their own workaround.
 
**mp-units**, **Eigen**, and **Boost.Units** have all independently converged on
the same core mechanism: bring `std::sqrt` into scope and let ADL do the work.
 
```cpp
using std::sqrt;
return sqrt(x);
```
 
The surrounding machinery differs:
- mp-units adds an explicit member function check. [[mp-units math implementation]](https://github.com/mpusz/mp-units/blob/b0e72810b983841b260d570b241c52586aa78999/src/core/include/mp-units/framework/representation_concepts.h#L227-L262)
- Eigen wraps the dispatch in a traits struct with SIMD specialisations and relies on
macros to minimize the duplication between different functions. [[Eigen `sqrt` implementation]](https://gitlab.com/libeigen/eigen/-/blob/master/Eigen/src/Core/MathFunctions.h)
```cpp
#define EIGEN_MATHFUNC_IMPL(func, scalar) \
    Eigen::internal::func##_impl<typename \
    Eigen::internal::global_math_functions_filtering_base<scalar>::type>
```
- Boost.Units applies the pattern to the inner value of a quantity type. [[Boost.Units `sqrt`]](https://github.com/boostorg/units/blob/develop/include/boost/units/cmath.hpp)

But the fundamental approach is identical in all three. This independent convergence is strong evidence
that it has become a common pattern within the current language and library model.
 
- **nholthaus/units** takes a different approach: it provides `units::math::sqrt` which
calls std::sqrt directly on the underlying scalar value, which means it does not enable
ADL resolution and thus does not extend to custom underlying types. [[nholthaus/units `math::sqrt`]](https://github.com/nholthaus/units/blob/578ac4ff8b0e96af8d87dd6b20357522038ccbb3/include/units.h#L4645)
 
---


 
The pattern is easy to get wrong and difficult to get right.
 
