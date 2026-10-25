# Koala `std/ut` API Reference

## Contents

**Functions**

| Name | Summary |
|------|---------|
| [`assert`](#assert) | Assert that a boolean condition is true. |
| [`assert_true`](#assert_true) | Assert that a boolean value is true. |
| [`assert_false`](#assert_false) | Assert that a boolean value is false. |
| [`assert_eq`](#assert_eq) | Assert that two values `a` and `b` are equal (`a == b`). |
| [`assert_eq_int8`](#assert_eq_int8) | Specialized explicit implementation of `assert_eq` for `int8` values. |
| [`assert_eq_uint8`](#assert_eq_uint8) | Specialized explicit implementation of `assert_eq` for `uint8` values. |
| [`assert_eq_int64`](#assert_eq_int64) | Specialized explicit implementation of `assert_eq` for `int64` values. |
| [`assert_ne`](#assert_ne) | Assert that two values `a` and `b` are not equal (`a != b`). |
| [`assert_ne_int8`](#assert_ne_int8) | Specialized explicit implementation of `assert_ne` for `int8` values. |
| [`assert_ne_uint8`](#assert_ne_uint8) | Specialized explicit implementation of `assert_ne` for `uint8` values. |
| [`assert_ne_int64`](#assert_ne_int64) | Specialized explicit implementation of `assert_ne` for `int64` values. |
| [`assert_lt`](#assert_lt) | Assert that `a` is strictly less than `b` (`a < b`). |
| [`assert_le`](#assert_le) | Assert that `a` is less than or equal to `b` (`a <= b`). |
| [`assert_gt`](#assert_gt) | Assert that `a` is strictly greater than `b` (`a > b`). |
| [`assert_ge`](#assert_ge) | Assert that `a` is greater than or equal to `b` (`a >= b`). |
| [`assert_float_close`](#assert_float_close) | Assert that two floating-point numbers `a` and `b` are approximately equal. |
| [`assert_float_eq`](#assert_float_eq) | Assert that two `float` values `a` and `b` are exactly equal (`a == b`). |
| [`assert_float_ne`](#assert_float_ne) | Assert that two `float` values `a` and `b` are not equal (`a != b`). |
| [`assert_int_eq`](#assert_int_eq) | Assert that two `int` values `a` and `b` are equal (`a == b`). |
| [`assert_int_ne`](#assert_int_ne) | Assert that two signed integers `a` and `b` are NOT equal (`a != b`). |
| [`assert_int_lt`](#assert_int_lt) | Assert that `a` is less than `b` (`a < b`). |
| [`assert_int_le`](#assert_int_le) | Assert that `a` is less than or equal to `b` (`a <= b`). |
| [`assert_int_gt`](#assert_int_gt) | Assert that `a` is greater than `b` (`a > b`). |
| [`assert_int_ge`](#assert_int_ge) | Assert that `a` is greater than or equal to `b` (`a >= b`). |
| [`assert_uint_eq`](#assert_uint_eq) | Assert that two unsigned integers `a` and `b` are equal (`a == b`). |
| [`assert_uint_ne`](#assert_uint_ne) | Assert that two unsigned integers `a` and `b` are NOT equal (`a != b`). |
| [`assert_uint_lt`](#assert_uint_lt) | Assert that unsigned `a` is less than `b` (`a < b`). |
| [`assert_uint_le`](#assert_uint_le) | Assert that unsigned `a` is less than or equal to `b` (`a <= b`). |
| [`assert_uint_gt`](#assert_uint_gt) | Assert that unsigned `a` is greater than `b` (`a > b`). |
| [`assert_uint_ge`](#assert_uint_ge) | Assert that unsigned `a` is greater than or equal to `b` (`a >= b`). |

## Functions

<a id="assert"></a>

### `assert`

```kl
pub func assert(cond bool, msg = "")
```

Assert that a boolean condition is true.

If `cond` is false, raises a `panic("assertion failed")` or
`panic("assertion failed: " + msg)` if a non-empty `msg` is provided.

Example:
    assert(x > 0)
    assert(items.len() != 0, "items should not be empty")

<a id="assert_true"></a>

### `assert_true`

```kl
pub func assert_true(cond bool, msg = "")
```

Assert that a boolean value is true.

Raises a panic if `cond` evaluates to false, optionally appending `msg`.

Example:
    assert_true(flag)
    assert_true(user.is_active(), "user must be active")

<a id="assert_false"></a>

### `assert_false`

```kl
pub func assert_false(cond bool, msg = "")
```

Assert that a boolean value is false.

Raises a panic if `cond` evaluates to true, optionally appending `msg`.

Example:
    assert_false(list.is_empty())
    assert_false(connection.is_closed(), "connection should remain open")

<a id="assert_eq"></a>

### `assert_eq`

```kl
pub func assert_eq[T : Equatable](a T, b T)
```

Assert that two values `a` and `b` are equal (`a == b`).

Requires that type `T` implements the `Equatable[T]` interface.
On failure, raises a panic displaying both formatting values.

Example:
    assert_eq(add(1, 2), 3)
    assert_eq(name, "koala")

<a id="assert_eq_int8"></a>

### `assert_eq_int8`

```kl
pub func assert_eq_int8(a int8, b int8)
```

Specialized explicit implementation of `assert_eq` for `int8` values.

If user writes `assert_eq` with `int8` values, this specialized implementation will be used.

<a id="assert_eq_uint8"></a>

### `assert_eq_uint8`

```kl
pub func assert_eq_uint8(a uint8, b uint8)
```

Specialized explicit implementation of `assert_eq` for `uint8` values.

If user writes `assert_eq` with `uint8` values, this specialized implementation will be used.

<a id="assert_eq_int64"></a>

### `assert_eq_int64`

```kl
pub func assert_eq_int64(a int, b int)
```

Specialized explicit implementation of `assert_eq` for `int64` values.

If user writes `assert_eq` with `int64` values, this specialized implementation will be used.

<a id="assert_ne"></a>

### `assert_ne`

```kl
pub func assert_ne[T : Equatable[T]](a T, b T)
```

Assert that two values `a` and `b` are not equal (`a != b`).

Requires that type `T` implements the `Equatable[T]` interface.
On failure, raises a panic displaying both formatting values.

Example:
    assert_ne(status_code, 404)
    assert_ne(ptr, nil)

<a id="assert_ne_int8"></a>

### `assert_ne_int8`

```kl
pub func assert_ne_int8(a int8, b int8)
```

Specialized explicit implementation of `assert_ne` for `int8` values.

If user writes `assert_ne` with `int8` values, this specialized implementation will be used.

<a id="assert_ne_uint8"></a>

### `assert_ne_uint8`

```kl
pub func assert_ne_uint8(a uint8, b uint8)
```

Specialized explicit implementation of `assert_ne` for `uint8` values.

If user writes `assert_ne` with `uint8` values, this specialized implementation will be used.

<a id="assert_ne_int64"></a>

### `assert_ne_int64`

```kl
pub func assert_ne_int64(a int, b int)
```

Specialized explicit implementation of `assert_ne` for `int64` values.

If user writes `assert_ne` with `int64` values, this specialized implementation will be used.

<a id="assert_lt"></a>

### `assert_lt`

```kl
pub func assert_lt[T : Comparable](a T, b T)
```

Assert that `a` is strictly less than `b` (`a < b`).

Requires that type `T` implements the `Comparable` interface.

Example:
    assert_lt(index, array.len())

<a id="assert_le"></a>

### `assert_le`

```kl
pub func assert_le[T : Comparable](a T, b T)
```

Assert that `a` is less than or equal to `b` (`a <= b`).

Requires that type `T` implements the `Comparable` interface.

Example:
    assert_le(count, MAX_LIMIT)

<a id="assert_gt"></a>

### `assert_gt`

```kl
pub func assert_gt[T : Comparable](a T, b T)
```

Assert that `a` is strictly greater than `b` (`a > b`).

Requires that type `T` implements the `Comparable` interface.

Example:
    assert_gt(score, 0)

<a id="assert_ge"></a>

### `assert_ge`

```kl
pub func assert_ge[T : Comparable](a T, b T)
```

Assert that `a` is greater than or equal to `b` (`a >= b`).

Requires that type `T` implements the `Comparable` interface.

Example:
    assert_ge(balance, total_cost)

<a id="assert_float_close"></a>

### `assert_float_close`

```kl
pub func assert_float_close(a float, b float, eps float = 1e-9)
```

Assert that two floating-point numbers `a` and `b` are approximately equal.

Compares `abs(a - b) <= eps`. Default tolerance `eps` is `1e-9`.

Example:
    assert_float_close(0.1 + 0.2, 0.3)
    assert_float_close(calculated_pi, 3.1415926, 1e-5)

<a id="assert_float_eq"></a>

### `assert_float_eq`

```kl
pub func assert_float_eq(a float, b float)
```

Assert that two `float` values `a` and `b` are exactly equal (`a == b`).

For practical floating-point comparisons where exact equality may fail due to
rounding errors, prefer `assert_float_close` with an appropriate tolerance.

On failure, raises a panic displaying both values.

<a id="assert_float_ne"></a>

### `assert_float_ne`

```kl
pub func assert_float_ne(a float, b float)
```

Assert that two `float` values `a` and `b` are not equal (`a != b`).

On failure, raises a panic displaying the value.

<a id="assert_int_eq"></a>

### `assert_int_eq`

```kl
pub func assert_int_eq(a int, b int)
```

Assert that two `int` values `a` and `b` are equal (`a == b`).

Thanks to Koala's type promotion system, smaller concrete unsigned integer types
(such as `int8` or `uint8`) will be automatically and implicitly promoted to `int64`
when calling this concrete function.

NOTE: While Koala supports automatic type promotion for concrete parameters,
generic functions (like `assert_eq[T]`) cannot perform type promotion across
type parameters due to strict generic type inference.

On failure, raises a panic displaying both formatting values.

<a id="assert_int_ne"></a>

### `assert_int_ne`

```kl
pub func assert_int_ne(a int, b int)
```

Assert that two signed integers `a` and `b` are NOT equal (`a != b`).

<a id="assert_int_lt"></a>

### `assert_int_lt`

```kl
pub func assert_int_lt(a int, b int)
```

Assert that `a` is less than `b` (`a < b`).

<a id="assert_int_le"></a>

### `assert_int_le`

```kl
pub func assert_int_le(a int, b int)
```

Assert that `a` is less than or equal to `b` (`a <= b`).

<a id="assert_int_gt"></a>

### `assert_int_gt`

```kl
pub func assert_int_gt(a int, b int)
```

Assert that `a` is greater than `b` (`a > b`).

<a id="assert_int_ge"></a>

### `assert_int_ge`

```kl
pub func assert_int_ge(a int, b int)
```

Assert that `a` is greater than or equal to `b` (`a >= b`).

<a id="assert_uint_eq"></a>

### `assert_uint_eq`

```kl
pub func assert_uint_eq(a uint, b uint)
```

Assert that two unsigned integers `a` and `b` are equal (`a == b`).

Thanks to Koala's type promotion system, smaller concrete unsigned integer
types (such as `uint8` or `uint16`) will be automatically and implicitly
promoted to `uint64` when calling this concrete function.

On failure, raises a panic displaying both formatting values.

<a id="assert_uint_ne"></a>

### `assert_uint_ne`

```kl
pub func assert_uint_ne(a uint64, b uint64)
```

Assert that two unsigned integers `a` and `b` are NOT equal (`a != b`).

<a id="assert_uint_lt"></a>

### `assert_uint_lt`

```kl
pub func assert_uint_lt(a uint64, b uint64)
```

Assert that unsigned `a` is less than `b` (`a < b`).

<a id="assert_uint_le"></a>

### `assert_uint_le`

```kl
pub func assert_uint_le(a uint64, b uint64)
```

Assert that unsigned `a` is less than or equal to `b` (`a <= b`).

<a id="assert_uint_gt"></a>

### `assert_uint_gt`

```kl
pub func assert_uint_gt(a uint64, b uint64)
```

Assert that unsigned `a` is greater than `b` (`a > b`).

<a id="assert_uint_ge"></a>

### `assert_uint_ge`

```kl
pub func assert_uint_ge(a uint64, b uint64)
```

Assert that unsigned `a` is greater than or equal to `b` (`a >= b`).
