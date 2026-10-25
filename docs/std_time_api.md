# Koala `std/time` API Reference

## Contents

**Classes**

| Name | Summary |
|------|---------|
| [`Instant`](#instant) | A point in time on a specific clock. |
| [`Duration`](#duration) | A span of time with nanosecond resolution. |

**Functions**

| Name | Summary |
|------|---------|
| [`sys_clock_realtime_nanos`](#sys_clock_realtime_nanos) | Return nanoseconds since the Unix epoch on the realtime clock. |
| [`sys_clock_monotonic_nanos`](#sys_clock_monotonic_nanos) | Return nanoseconds from an unspecified starting point on the monotonic clock. |
| [`sys_sleep_nanos`](#sys_sleep_nanos) | Block the current thread for `ns` nanoseconds. |
| [`now`](#now) | Return the current instant on the realtime (wall) clock. |
| [`monotonic`](#monotonic) | Return the current instant on the monotonic clock. |
| [`sleep`](#sleep) | Block the current thread for the duration `d`. |
| [`sleep_until`](#sleep_until) | Block the current thread until the monotonic instant `t`. |
| [`unix_secs`](#unix_secs) | Return the current Unix time in whole seconds. |
| [`unix_nanos`](#unix_nanos) | Return the current Unix time in nanoseconds. |

**Constants**

| Name | Summary |
|------|---------|
| [`CLOCK_REALTIME`](#clock_realtime) | Identifier of the system realtime (wall) clock. |
| [`CLOCK_MONOTONIC`](#clock_monotonic) | Identifier of the monotonic clock, which never goes backward. |

## Classes

<a id="instant"></a>

### `Instant`

```kl
pub class Instant {
    let epoch_ns int
    let clock int
    pub func __init__(_ns int, _clock int)
    pub static func from_unix_ns(ns int) Instant
    pub func __add__(d Duration) Instant
    pub func __sub__(d Duration) Instant
    pub func diff(other Instant) Duration
    pub func __eq__(other Instant) bool
    pub func __ne__(other Instant) bool
    pub func __lt__(other Instant) bool
    pub func __le__(other Instant) bool
    pub func __gt__(other Instant) bool
    pub func __ge__(other Instant) bool
    pub func to_unix_ns() int64
    pub func to_unix_ms() int64
    pub func to_unix_secs() int64
    pub func fmt(f Formatter)
    pub func to_str() str
}
```

A point in time on a specific clock.

Create instants with `time.now()` (realtime) or `time.monotonic()`.
Arithmetic with `Duration` and comparisons work only between instants of
the same clock; mixing clocks panics.

---

**Fields**

<a id="instant.epoch_ns"></a>

**`let epoch_ns int`**

Nanoseconds since the epoch of `clock`.

<a id="instant.clock"></a>

**`let clock int`**

Identifier of the clock this instant belongs to.

<a id="instant.__init__"></a>

**`__init__(_ns int, _clock int)`**

Create an instant from a raw nanosecond timestamp and clock id;
prefer `time.now()` and `time.monotonic()`.

<a id="instant.from_unix_ns"></a>

**`static from_unix_ns(ns int) Instant`**

Create a realtime (`CLOCK_REALTIME`) instant from Unix nanoseconds.

The controlled counterpart to `to_unix_ns()`: it hides the clock id — a
private detail of `std/time` — from callers that only want a wall-clock
instant at a given epoch timestamp.

<a id="instant.__add__"></a>

**`__add__(d Duration) Instant`**

Return a new instant shifted forward by the duration `d` (`self + d`).

<a id="instant.__sub__"></a>

**`__sub__(d Duration) Instant`**

Return a new instant shifted backward by the duration `d` (`self - d`).

<a id="instant.diff"></a>

**`diff(other Instant) Duration`**

Return the elapsed duration between two instants of the same clock
(`self - other`); positive when `self` is later than `other`.

<a id="instant.__eq__"></a>

**`__eq__(other Instant) bool`**

Return true if `self == other`.

<a id="instant.__ne__"></a>

**`__ne__(other Instant) bool`**

Return true if `self != other`.

<a id="instant.__lt__"></a>

**`__lt__(other Instant) bool`**

Return true if `self < other`.

<a id="instant.__le__"></a>

**`__le__(other Instant) bool`**

Return true if `self <= other`.

<a id="instant.__gt__"></a>

**`__gt__(other Instant) bool`**

Return true if `self > other`.

<a id="instant.__ge__"></a>

**`__ge__(other Instant) bool`**

Return true if `self >= other`.

<a id="instant.to_unix_ns"></a>

**`to_unix_ns() int64`**

Return nanoseconds since the Unix epoch; meaningful for realtime instants only.

<a id="instant.to_unix_ms"></a>

**`to_unix_ms() int64`**

Return whole milliseconds since the Unix epoch.

<a id="instant.to_unix_secs"></a>

**`to_unix_secs() int64`**

Return whole seconds since the Unix epoch.

<a id="instant.fmt"></a>

**`fmt(f Formatter)`**

Format the instant for display.

<a id="instant.to_str"></a>

**`to_str() str`**

Convert the instant to a string.

This is a plain method, NOT a `ToString` conformance (so `str(i)` is a compile error):
the result is the debug-oriented form `Instant(epoch_ns=..., clock=...)`, same shape as `fmt`.
Use `to_unix_ns()` for the raw numeric timestamp.

<a id="duration"></a>

### `Duration`

```kl
pub class Duration {
    let ns int
    pub func __init__(_ns int)
    pub func __add__(d Duration) Duration
    pub func __sub__(d Duration) Duration
    pub func __mul__(n int) Duration
    pub func __div__(n int) Duration
    pub static func from_days(d int) Duration
    pub static func from_hours(h int) Duration
    pub static func from_secs(s int) Duration
    pub static func from_millis(ms int) Duration
    pub static func from_micros(us int) Duration
    pub static func from_nanos(ns int) Duration
    pub func to_ns() int64
    pub func to_ms() int64
    pub func to_secs() int64
    pub func fmt(f Formatter)
    pub func to_str() str
    pub func __eq__(d Duration) bool
    pub func __ne__(d Duration) bool
}
```

A span of time with nanosecond resolution.

Create durations with the `from_days`, `from_hours`, `from_secs`,
`from_millis`, `from_micros` and `from_nanos` constructors, combine them
with `+`, `-`, `*` and `/`, and
pass them to `time.sleep`.

---

**Fields**

<a id="duration.ns"></a>

**`let ns int`**

Length of the duration in nanoseconds.

<a id="duration.__init__"></a>

**`__init__(_ns int)`**

Create a duration from a raw nanosecond count; prefer the `from_*` constructors.

<a id="duration.__add__"></a>

**`__add__(d Duration) Duration`**

Return the sum of two durations (`self + d`).

<a id="duration.__sub__"></a>

**`__sub__(d Duration) Duration`**

Return the difference of two durations (`self - d`).

<a id="duration.__mul__"></a>

**`__mul__(n int) Duration`**

Return the duration scaled by the integer `n` (`self * n`).

<a id="duration.__div__"></a>

**`__div__(n int) Duration`**

Return the duration divided by the integer `n` (`self / n`); panics if `n` is zero.

<a id="duration.from_days"></a>

**`static from_days(d int) Duration`**

Create a duration of `d` days (24-hour days).

<a id="duration.from_hours"></a>

**`static from_hours(h int) Duration`**

Create a duration of `h` hours.

<a id="duration.from_secs"></a>

**`static from_secs(s int) Duration`**

Create a duration of `s` seconds.

<a id="duration.from_millis"></a>

**`static from_millis(ms int) Duration`**

Create a duration of `ms` milliseconds.

<a id="duration.from_micros"></a>

**`static from_micros(us int) Duration`**

Create a duration of `us` microseconds.

<a id="duration.from_nanos"></a>

**`static from_nanos(ns int) Duration`**

Create a duration of `ns` nanoseconds.

<a id="duration.to_ns"></a>

**`to_ns() int64`**

Return the length in whole nanoseconds.

<a id="duration.to_ms"></a>

**`to_ms() int64`**

Return the length in whole milliseconds, truncated.

<a id="duration.to_secs"></a>

**`to_secs() int64`**

Return the length in whole seconds, truncated.

<a id="duration.fmt"></a>

**`fmt(f Formatter)`**

Format the duration for display.
Produce a string using the largest unit that fits, truncated:
`2s`, `500ms`, `30us` or `10ns`.

<a id="duration.to_str"></a>

**`to_str() str`**

Convert the duration to its canonical string form: the nanosecond count
followed by the unit suffix `ns`, e.g. `"1500000000ns"`.

The suffix keeps the result self-describing and distinguishes it from plain integer text.
If you need the raw integer nanoseconds, use `to_ns()`;
for human-readable display (`"1s"`, `"500ms"`), use `fmt` / `format()`.

<a id="duration.__eq__"></a>

**`__eq__(d Duration) bool`**

Return true if both durations have the same length.

<a id="duration.__ne__"></a>

**`__ne__(d Duration) bool`**

Return true if the durations differ in length.

## Functions

<a id="sys_clock_realtime_nanos"></a>

### `sys_clock_realtime_nanos`

```kl
func sys_clock_realtime_nanos() int
```

*`@native`*

Return nanoseconds since the Unix epoch on the realtime clock.

<a id="sys_clock_monotonic_nanos"></a>

### `sys_clock_monotonic_nanos`

```kl
func sys_clock_monotonic_nanos() int
```

*`@native`*

Return nanoseconds from an unspecified starting point on the monotonic clock.

<a id="sys_sleep_nanos"></a>

### `sys_sleep_nanos`

```kl
func sys_sleep_nanos(ns int)
```

*`@native`*

Block the current thread for `ns` nanoseconds.

<a id="now"></a>

### `now`

```kl
pub func now() Instant
```

Return the current instant on the realtime (wall) clock.

<a id="monotonic"></a>

### `monotonic`

```kl
pub func monotonic() Instant
```

Return the current instant on the monotonic clock.

Monotonic instants never go backward, which makes them the right choice
for measuring elapsed time and for `sleep_until`.

<a id="sleep"></a>

### `sleep`

```kl
pub func sleep(d Duration)
```

Block the current thread for the duration `d`.

Durations of zero or less return immediately.

<a id="sleep_until"></a>

### `sleep_until`

```kl
pub func sleep_until(t Instant)
```

Block the current thread until the monotonic instant `t`.

Panics if `t` is not a monotonic instant; returns immediately once `t`
has passed.

<a id="unix_secs"></a>

### `unix_secs`

```kl
pub func unix_secs() int
```

Return the current Unix time in whole seconds.

<a id="unix_nanos"></a>

### `unix_nanos`

```kl
pub func unix_nanos() int
```

Return the current Unix time in nanoseconds.

## Constants

<a id="clock_realtime"></a>

### `CLOCK_REALTIME`

```kl
const CLOCK_REALTIME = 0
```

Identifier of the system realtime (wall) clock.

<a id="clock_monotonic"></a>

### `CLOCK_MONOTONIC`

```kl
const CLOCK_MONOTONIC = 1
```

Identifier of the monotonic clock, which never goes backward.
