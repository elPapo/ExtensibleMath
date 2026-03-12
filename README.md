# Extensible Math Functions for C++: A Direction Paper

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

