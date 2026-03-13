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
range of types. This makes for poor separation of concern.

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

But the fundamental approach is identical in all three. This independent convergence on the same pattern
is strong evidence both that the need is real and that the solution is well-understood, making this a natural candidate for standardization.
 
- **nholthaus/units** takes a different approach: it provides `units::math::sqrt` which
calls std::sqrt directly on the underlying scalar value, which means it does not enable
ADL resolution and thus does not extend to custom underlying types. [[nholthaus/units `math::sqrt`]](https://github.com/nholthaus/units/blob/578ac4ff8b0e96af8d87dd6b20357522038ccbb3/include/units.h#L4645)
 

---
 
### Existing Code
 
A large body of existing generic C++ code calls `std::sqrt` directly, either out
of habit or because the author was unaware of the ADL idiom. This code silently
fails to work with user-defined types that provide their own `sqrt` and restricts
the composability of generic code. There is no practical way for users of such
libraries to fix this without modifying the library itself.


---
 
### Teachability
 
The `using std::sqrt; sqrt(x);` idiom is specialist knowledge. For instance, it
does not appear as an explicit recommendation in the C++ Core Guidelines.
It is not obvious to intermediate C++ programmers. It is easy to get silently wrong.
Calling `std::sqrt(x)` looks correct and compiles cleanly, but breaks extensibility for
custom types.
 
The following exchange from [nholthaus/units GitHub issue #39](https://github.com/nholthaus/units/issues/39#issuecomment-270918812)
is instructive. A user reports that generic algorithms using ADL-found math functions
do not work with unit types, and the library author responds:
 
> *"Honestly, I guess I just never use ADL because I pretty much exclusively use
> fully qualified namespaces in my code, and I didn't put thought into it."*

This should not be surprising. The responsibility of making custom types work
intuitively should belong to their authors, not to their users.
It takes conscious effort for a generic library author to anticipate the needs for wrappers
of hypothetical custom types and explicitly provide support for them.
rithmetic operators require no such effort: they are defined by the type author and
compose transparently in generic code.
Math functions should be no different.

In the current situation, it is easy to do the wrong thing, and difficult to do the right one.

---
 
### Legacy Types

While it is desirable to change the behaviour with custom types, it is very clear that
any change in the behaviour of existing code would be unacceptible.
This requires a clear definition of which types belong to the legacy domain.
Today that boundary is straightforward: primitive arithmetic types and
std::complex specializations. But C++26 introduces std::simd, and future
standards will likely introduce further numeric vocabulary types. Each addition
makes the legacy boundary harder to define retroactively. Capturing std::simd in the legacy domain
would also require including <simd>, a substantial header.
The legacy boundary is still lightweight today; that may not remain true.
Establishing the extensibility mechanism now is considerably
easier than doing so after the standard numeric type landscape has grown further.

 
---
 
