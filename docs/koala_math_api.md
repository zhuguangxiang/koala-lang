# Koala `koala/math` API Reference

## Contents

**Functions**

| Name | Summary |
|------|---------|
| [`sqrt`](#sqrt) | Return the square root of `x`. |
| [`pow`](#pow) | Return `x` raised to the power `y` (floating-point version). |
| [`sin`](#sin) | Return the sine of `x` (measured in radians). |
| [`cos`](#cos) | Return the cosine of `x` (measured in radians). |
| [`tan`](#tan) | Return the tangent of `x` (measured in radians). |
| [`asin`](#asin) | Return the arc sine of `x`, in radians. |
| [`acos`](#acos) | Return the arc cosine of `x`, in radians. |
| [`atan`](#atan) | Return the arc tangent of `x`, in radians. |
| [`atan2`](#atan2) | Return the arc tangent of `y/x`, in radians, using the signs of both arguments to determine the quadrant of the return value. |
| [`exp`](#exp) | Return e raised to the power `x`. |
| [`log`](#log) | Return the natural logarithm of `x` (base e). |
| [`log10`](#log10) | Return the base-10 logarithm of `x`. |
| [`log2`](#log2) | Return the base-2 logarithm of `x`. |
| [`isnan`](#isnan) | Return `true` if `x` is NaN (not a number). |
| [`isinf`](#isinf) | Return `true` if `x` is positive or negative infinity. |

**Constants**

| Name | Summary |
|------|---------|
| [`PI`](#pi) | The mathematical constant π (pi) ≈ 3.14159265358979323846. |
| [`E`](#e) | The mathematical constant e (Euler's number) ≈ 2.71828182845904523536. |

## Functions

<a id="sqrt"></a>

### `sqrt`

```kl
pub func sqrt(x float) float
```

*`@native`*

Return the square root of `x`.

Panic if `x` is negative.

<a id="pow"></a>

### `pow`

```kl
pub func pow(x float, y float) float
```

*`@native`*

Return `x` raised to the power `y` (floating-point version).

Note: This is the floating-point power function. For integer exponentiation,
use the top-level `pow(base, exp)` function.

<a id="sin"></a>

### `sin`

```kl
pub func sin(x float) float
```

*`@native`*

Return the sine of `x` (measured in radians).

<a id="cos"></a>

### `cos`

```kl
pub func cos(x float) float
```

*`@native`*

Return the cosine of `x` (measured in radians).

<a id="tan"></a>

### `tan`

```kl
pub func tan(x float) float
```

*`@native`*

Return the tangent of `x` (measured in radians).

<a id="asin"></a>

### `asin`

```kl
pub func asin(x float) float
```

*`@native`*

Return the arc sine of `x`, in radians.

The result is in the range [-π/2, π/2].
Panic if `x` is not in the range [-1.0, 1.0].

<a id="acos"></a>

### `acos`

```kl
pub func acos(x float) float
```

*`@native`*

Return the arc cosine of `x`, in radians.

The result is in the range [0, π].
Panic if `x` is not in the range [-1.0, 1.0].

<a id="atan"></a>

### `atan`

```kl
pub func atan(x float) float
```

*`@native`*

Return the arc tangent of `x`, in radians.

The result is in the range [-π/2, π/2].

<a id="atan2"></a>

### `atan2`

```kl
pub func atan2(y float, x float) float
```

*`@native`*

Return the arc tangent of `y/x`, in radians, using the signs of both
arguments to determine the quadrant of the return value.

The result is in the range [-π, π].

<a id="exp"></a>

### `exp`

```kl
pub func exp(x float) float
```

*`@native`*

Return e raised to the power `x`.

<a id="log"></a>

### `log`

```kl
pub func log(x float) float
```

*`@native`*

Return the natural logarithm of `x` (base e).

Panic if `x` is negative or zero.

<a id="log10"></a>

### `log10`

```kl
pub func log10(x float) float
```

*`@native`*

Return the base-10 logarithm of `x`.

Panic if `x` is negative or zero.

<a id="log2"></a>

### `log2`

```kl
pub func log2(x float) float
```

*`@native`*

Return the base-2 logarithm of `x`.

Panic if `x` is negative or zero.

<a id="isnan"></a>

### `isnan`

```kl
pub func isnan(x float) bool
```

*`@native`*

Return `true` if `x` is NaN (not a number).

<a id="isinf"></a>

### `isinf`

```kl
pub func isinf(x float) bool
```

*`@native`*

Return `true` if `x` is positive or negative infinity.

## Constants

<a id="pi"></a>

### `PI`

```kl
pub const PI = 3.14159265358979323846
```

The mathematical constant π (pi) ≈ 3.14159265358979323846.

<a id="e"></a>

### `E`

```kl
pub const E = 2.71828182845904523536
```

The mathematical constant e (Euler's number) ≈ 2.71828182845904523536.
