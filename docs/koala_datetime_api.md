# Koala `koala/datetime` API Reference

## Contents

**Classes**

| Name | Summary |
|------|---------|
| [`Date`](#date) | A calendar date in the proleptic Gregorian calendar, with no time or timezone. |
| [`DateTime`](#datetime) | A date and time of day, in UTC, with nanosecond precision. |
| [`Time`](#time) | A time of day with nanosecond resolution, without a date or timezone. |

**Functions**

| Name | Summary |
|------|---------|
| [`parse_num`](#parse_num) | Parse a fixed-width all-digit ISO segment into an int; panic on any non-digit. Used by `from_str` so a malformed field reports a datetime-specific message instead of a generic integer-parse failure. |
| [`floor_days`](#floor_days) | Whole days spanned by `ns` nanoseconds, rounded toward negative infinity (matches Python's `timedelta.days` semantics for negative durations). |
| [`is_leap_year`](#is_leap_year) | Return true if `y` is a leap year in the proleptic Gregorian calendar. |
| [`days_in_month`](#days_in_month) | Return the number of days in month `m` (1-12) of year `y`. |
| [`days_from_civil`](#days_from_civil) | Return the day count from the Unix epoch (1970-01-01 == 0) to the civil date `(y, m, d)`; negative before the epoch. |
| [`civil_from_days`](#civil_from_days) | Return the civil date `(year, month, day)` for a day count `z` from the Unix epoch (1970-01-01 == 0); the inverse of `days_from_civil`. |

**Constants**

| Name | Summary |
|------|---------|
| [`MINYEAR`](#minyear) | The smallest year supported by `Date` (matches Python's `MINYEAR`). |
| [`MAXYEAR`](#maxyear) | The largest year supported by `Date` (matches Python's `MAXYEAR`). |
| [`NANOS_PER_SEC`](#nanos_per_sec) |  |
| [`NANOS_PER_MIN`](#nanos_per_min) |  |
| [`NANOS_PER_HOUR`](#nanos_per_hour) |  |
| [`NANOS_PER_DAY`](#nanos_per_day) |  |

## Classes

<a id="date"></a>

### `Date`

```kl
pub class Date : ToString {
    pub let year int
    pub let month int
    pub let day int
    pub func __init__(y int, m int, d int)
    pub static func from_days(z int) Date
    pub static func from_str(s str) Date
    pub static func today() Date
    pub func to_days() int
    pub func weekday() int
    pub func add_days(n int) Date
    pub func __add__(d time.Duration) Date
    pub func __sub__(d time.Duration) Date
    pub func diff(o Date) time.Duration
    pub func hash() int
    pub func replace(y = -1, mo = -1, d = -1) Date
    pub func to_str() str
    pub func fmt(f Formatter)
    pub func __eq__(o Date) bool
    pub func __ne__(o Date) bool
    pub func __lt__(o Date) bool
    pub func __le__(o Date) bool
    pub func __gt__(o Date) bool
    pub func __ge__(o Date) bool
}
```
**Bases:** `ToString`


A calendar date in the proleptic Gregorian calendar, with no time or
timezone.

Build one with `Date(year, month, day)`; dates are immutable, so
arithmetic like `add_days` returns a new `Date`. Internally a date maps
to a day count with the Unix epoch (1970-01-01) as day 0.

---

**Fields**

<a id="date.year"></a>

**`pub let year int`**

The year, `MINYEAR`..`MAXYEAR`.

<a id="date.month"></a>

**`pub let month int`**

The month, 1 (January)..12 (December).

<a id="date.day"></a>

**`pub let day int`**

The day of month, 1..`days_in_month(year, month)`.

<a id="date.__init__"></a>

**`__init__(y int, m int, d int)`**

Create a date from a year, month (1-12) and day.

Panic if the year is outside `MINYEAR`..`MAXYEAR`, the month is
outside 1..12, or the day is outside 1..`days_in_month`.

<a id="date.from_days"></a>

**`static from_days(z int) Date`**

Create the date `z` days from the Unix epoch (1970-01-01 == day 0).

<a id="date.from_str"></a>

**`static from_str(s str) Date`**

Parse an ISO 8601 calendar date `YYYY-MM-DD` (the inverse of [`to_str`](#to_str)).

Panic if the text is malformed or a field is out of range.

<a id="date.today"></a>

**`static today() Date`**

Return today's date in UTC. Koala has no local-timezone detection, so
this is the calendar date from the realtime clock read as UTC.

<a id="date.to_days"></a>

**`to_days() int`**

Return the day count from the Unix epoch to this date; negative
before 1970-01-01.

<a id="date.weekday"></a>

**`weekday() int`**

Return the weekday, Monday == 0 through Sunday == 6.

<a id="date.add_days"></a>

**`add_days(n int) Date`**

Return a new date `n` days after this one; `n` may be negative.

<a id="date.__add__"></a>

**`__add__(d time.Duration) Date`**

Return a date advanced by the whole days in `d` (`date + timedelta`);
any sub-day remainder is ignored.

<a id="date.__sub__"></a>

**`__sub__(d time.Duration) Date`**

Return a date moved back by the whole days in `d` (`date - timedelta`);
any sub-day remainder is ignored.

<a id="date.diff"></a>

**`diff(o Date) time.Duration`**

Return the elapsed time between two dates as a `time.Duration`
(Python's `date - date`); negative when `self` precedes `o`. Koala does
not overload the `-` operator, so the two-operand difference is a method.

<a id="date.hash"></a>

**`hash() int`**

Return a hash consistent with equality (dates compare by day count).
Value-semantic classes must override the default identity hash so that
equal dates hash identically for dict/set keys.

<a id="date.replace"></a>

**`replace(y = -1, mo = -1, d = -1) Date`**

Return a date with the given fields replaced (Python's `date.replace()`).
A negative (or omitted) argument keeps the current value; the result is
re-validated by the `Date` constructor.

<a id="date.to_str"></a>

**`to_str() str`**

Return the ISO-8601 form `YYYY-MM-DD`.

<a id="date.fmt"></a>

**`fmt(f Formatter)`**

Format for display (Printable): the debug-oriented `Date(...)` wrapper.
The raw ISO form is [`to_str`](#to_str).

<a id="date.__eq__"></a>

**`__eq__(o Date) bool`**

Return true if both dates fall on the same day.

<a id="date.__ne__"></a>

**`__ne__(o Date) bool`**

Return true if the two dates differ.

<a id="date.__lt__"></a>

**`__lt__(o Date) bool`**

Return true if `self` is earlier than `o`.

<a id="date.__le__"></a>

**`__le__(o Date) bool`**

Return true if `self` is not later than `o`.

<a id="date.__gt__"></a>

**`__gt__(o Date) bool`**

Return true if `self` is later than `o`.

<a id="date.__ge__"></a>

**`__ge__(o Date) bool`**

Return true if `self` is not earlier than `o`.

<a id="datetime"></a>

### `DateTime`

```kl
pub class DateTime : ToString {
    pub let year int
    pub let month int
    pub let day int
    pub let hour int
    pub let minute int
    pub let second int
    pub let nanosecond int
    pub func __init__(y int, mo int, d int, h int, mi = 0, s = 0, ns = 0)
    pub static func now() DateTime
    pub static func from_instant(i time.Instant) DateTime
    pub static func from_str(s str) DateTime
    pub static func combine(d Date, t Time) DateTime
    static func from_epoch_nanos(ns int) DateTime
    pub func timestamp() float64
    pub func timestamp_nanos() int
    pub static func from_timestamp(secs float64) DateTime
    pub static func from_timestamp_nanos(ns int) DateTime
    pub func to_instant() time.Instant
    pub func add(dur time.Duration) DateTime
    pub func __add__(d time.Duration) DateTime
    pub func __sub__(d time.Duration) DateTime
    pub func diff(o DateTime) time.Duration
    pub func hash() int
    pub func date() Date
    pub func time_of_day() Time
    pub func replace(y = -1, mo = -1, d = -1, h = -1, mi = -1, s = -1, ns = -1) DateTime
    func to_epoch_nanos() int
    pub func to_str() str
    pub func fmt(f Formatter)
    pub func __eq__(o DateTime) bool
    pub func __ne__(o DateTime) bool
    pub func __lt__(o DateTime) bool
    pub func __le__(o DateTime) bool
    pub func __gt__(o DateTime) bool
    pub func __ge__(o DateTime) bool
}
```
**Bases:** `ToString`


A date and time of day, in UTC, with nanosecond precision.

Combines a calendar [`Date`] and a wall-clock [`Time`]. Bridge to `std/time`:
[`to_instant`](#to_instant) / [`from_instant`](#from_instant) convert to/from a
`time.Instant`, and [`add`](#add) shifts by a `time.Duration`.

The canonical string form ([`to_str`](#to_str), `ToString`) is ISO 8601 with a `T`
separator, e.g. `"2026-10-24T13:30:45"`; the display form ([`fmt`](#fmt), `Printable`)
wraps it as `DateTime(...)`.

---

**Fields**

<a id="datetime.year"></a>

**`pub let year int`**

The year (`MINYEAR`..`MAXYEAR`).

<a id="datetime.month"></a>

**`pub let month int`**

The month (1..12).

<a id="datetime.day"></a>

**`pub let day int`**

The day of the month (1..days_in_month).

<a id="datetime.hour"></a>

**`pub let hour int`**

The hour (0..23).

<a id="datetime.minute"></a>

**`pub let minute int`**

The minute (0..59).

<a id="datetime.second"></a>

**`pub let second int`**

The second (0..59; no leap second).

<a id="datetime.nanosecond"></a>

**`pub let nanosecond int`**

The nanosecond (0..999999999).

<a id="datetime.__init__"></a>

**`__init__(y int, mo int, d int, h int, mi = 0, s = 0, ns = 0)`**

Create a `DateTime` from its fields (UTC). Date and time ranges are validated
by reusing [`Date`] and [`Time`]. `mi`, `s`, `ns` default to 0.

<a id="datetime.now"></a>

**`static now() DateTime`**

Return the current UTC date and time (backed by `time.now()`).

<a id="datetime.from_instant"></a>

**`static from_instant(i time.Instant) DateTime`**

Build a `DateTime` from a `time.Instant` (UTC), using its Unix nanoseconds.

<a id="datetime.from_str"></a>

**`static from_str(s str) DateTime`**

Parse an ISO 8601 datetime `YYYY-MM-DDTHH:MM:SS`, optionally with a
`.nnnnnnnnn` nanosecond fraction (the inverse of [`to_str`](#to_str)).

Panic if the text is malformed or a field is out of range.

<a id="datetime.combine"></a>

**`static combine(d Date, t Time) DateTime`**

Combine a `Date` and a `Time` into a `DateTime` (Python's `datetime.combine`).

<a id="datetime.from_epoch_nanos"></a>

**`static from_epoch_nanos(ns int) DateTime`**

Build a `DateTime` from nanoseconds since the Unix epoch.

Negative values (before 1970) are floored so the time-of-day stays in
`[0, 86400e9)`: e.g. `-1` ns is `1969-12-31T23:59:59.999999999`.

<a id="datetime.timestamp"></a>

**`timestamp() float64`**

Return seconds since the Unix epoch as a `float64` (Python's
`datetime.timestamp()`). Because a nanosecond epoch magnitude can exceed
`float64`'s exact-integer range, this is approximate at sub-microsecond
levels; use [`timestamp_nanos`](#timestamp_nanos) for exact values.

<a id="datetime.timestamp_nanos"></a>

**`timestamp_nanos() int`**

Return nanoseconds since the Unix epoch, exact (no float rounding).

<a id="datetime.from_timestamp"></a>

**`static from_timestamp(secs float64) DateTime`**

Build a `DateTime` from seconds since the Unix epoch (Python's
`datetime.fromtimestamp()`).

<a id="datetime.from_timestamp_nanos"></a>

**`static from_timestamp_nanos(ns int) DateTime`**

Build a `DateTime` from exact nanoseconds since the Unix epoch.

<a id="datetime.to_instant"></a>

**`to_instant() time.Instant`**

Convert to a `time.Instant` on the realtime (wall) clock.

<a id="datetime.add"></a>

**`add(dur time.Duration) DateTime`**

Return a new `DateTime` shifted by the duration `dur`.

<a id="datetime.__add__"></a>

**`__add__(d time.Duration) DateTime`**

Return a new datetime advanced by `d` (`datetime + timedelta`).

<a id="datetime.__sub__"></a>

**`__sub__(d time.Duration) DateTime`**

Return a new datetime moved back by `d` (`datetime - timedelta`).

<a id="datetime.diff"></a>

**`diff(o DateTime) time.Duration`**

Return the elapsed time between two datetimes as a `time.Duration`
(Python's `datetime - datetime`); negative when `self` precedes `o`.
Koala does not overload `-`, so the difference is a method.

<a id="datetime.hash"></a>

**`hash() int`**

Return a hash consistent with datetime equality (compared by absolute
nanoseconds since the epoch), overriding the default identity hash.

<a id="datetime.date"></a>

**`date() Date`**

Return the date part.

<a id="datetime.time_of_day"></a>

**`time_of_day() Time`**

Return the time-of-day part.

<a id="datetime.replace"></a>

**`replace(y = -1, mo = -1, d = -1, h = -1, mi = -1, s = -1, ns = -1) DateTime`**

Return a datetime with the given fields replaced (Python's
`datetime.replace()`). A negative (or omitted) argument keeps the current
value; the result is re-validated by the `DateTime` constructor.

<a id="datetime.to_epoch_nanos"></a>

**`to_epoch_nanos() int`**

Nanoseconds since the Unix epoch (1970-01-01T00:00:00Z); negative before it.

<a id="datetime.to_str"></a>

**`to_str() str`**

Return the raw ISO 8601 string (ToString): `YYYY-MM-DDTHH:MM:SS`, with
`.NNNNNNNNN` appended when [`nanosecond`](#nanosecond) is non-zero.

<a id="datetime.fmt"></a>

**`fmt(f Formatter)`**

Format for display (Printable): the debug-oriented `DateTime(...)` wrapper.
The raw ISO form is [`to_str`](#to_str).

<a id="datetime.__eq__"></a>

**`__eq__(o DateTime) bool`**

Return true if equal to `o` (Equatable[DateTime]).

<a id="datetime.__ne__"></a>

**`__ne__(o DateTime) bool`**

Return true if not equal to `o`.

<a id="datetime.__lt__"></a>

**`__lt__(o DateTime) bool`**

Return true if earlier than `o` (Comparable[DateTime]).

<a id="datetime.__le__"></a>

**`__le__(o DateTime) bool`**

Return true if not later than `o`.

<a id="datetime.__gt__"></a>

**`__gt__(o DateTime) bool`**

Return true if later than `o`.

<a id="datetime.__ge__"></a>

**`__ge__(o DateTime) bool`**

Return true if not earlier than `o`.

<a id="time"></a>

### `Time`

```kl
pub class Time : ToString {
    pub let hour int
    pub let minute int
    pub let second int
    pub let nanosecond int
    pub func __init__(h int, m = 0, s = 0, ns = 0)
    pub static func from_nanos(n int) Time
    pub static func from_str(s str) Time
    pub func to_nanos() int
    pub func hash() int
    pub func replace(h = -1, mi = -1, s = -1, ns = -1) Time
    pub func to_str() str
    pub func fmt(f Formatter)
    pub func __eq__(o Time) bool
    pub func __ne__(o Time) bool
    pub func __lt__(o Time) bool
    pub func __le__(o Time) bool
    pub func __gt__(o Time) bool
    pub func __ge__(o Time) bool
}
```
**Bases:** `ToString`


A time of day with nanosecond resolution, without a date or timezone.

Build one with `Time(hour, minute, second, nanosecond)`; `minute`,
`second` and `nanosecond` default to 0. Times are immutable, and the
canonical ordering compares by nanoseconds since midnight.

---

**Fields**

<a id="time.hour"></a>

**`pub let hour int`**

The hour, 0..23.

<a id="time.minute"></a>

**`pub let minute int`**

The minute, 0..59.

<a id="time.second"></a>

**`pub let second int`**

The second, 0..59 (leap seconds are not represented).

<a id="time.nanosecond"></a>

**`pub let nanosecond int`**

The nanosecond within the second, 0..999_999_999.

<a id="time.__init__"></a>

**`__init__(h int, m = 0, s = 0, ns = 0)`**

Create a time from an hour (0-23) and optional minute, second and
nanosecond (each defaulting to 0).

Panic if any component is outside its valid range.

<a id="time.from_nanos"></a>

**`static from_nanos(n int) Time`**

Create the time `n` nanoseconds after midnight.

Panic if `n` is outside 0..86_399_999_999_999 (one day).

<a id="time.from_str"></a>

**`static from_str(s str) Time`**

Parse an ISO 8601 time `HH:MM:SS` or `HH:MM:SS.nnnnnnnnn` (the inverse of
[`to_str`](#to_str)).

Panic if the text is malformed or a field is out of range.

<a id="time.to_nanos"></a>

**`to_nanos() int`**

Return the number of nanoseconds since midnight, 0..86_399_999_999_999.

<a id="time.hash"></a>

**`hash() int`**

Return a hash consistent with time equality (compared by nanoseconds
since midnight), overriding the default identity hash.

<a id="time.replace"></a>

**`replace(h = -1, mi = -1, s = -1, ns = -1) Time`**

Return a time with the given fields replaced (Python's `time.replace()`).
A negative (or omitted) argument keeps the current value; the result is
re-validated by the `Time` constructor.

<a id="time.to_str"></a>

**`to_str() str`**

Return the ISO-8601 form `HH:MM:SS`, adding `.NNNNNNNNN` when the
nanosecond component is non-zero.

<a id="time.fmt"></a>

**`fmt(f Formatter)`**

Format for display (Printable): the debug-oriented `Time(...)` wrapper.
The raw ISO form is [`to_str`](#to_str).

<a id="time.__eq__"></a>

**`__eq__(o Time) bool`**

Return true if both times are the same point within the day.

<a id="time.__ne__"></a>

**`__ne__(o Time) bool`**

Return true if the two times differ.

<a id="time.__lt__"></a>

**`__lt__(o Time) bool`**

Return true if `self` is earlier in the day than `o`.

<a id="time.__le__"></a>

**`__le__(o Time) bool`**

Return true if `self` is not later in the day than `o`.

<a id="time.__gt__"></a>

**`__gt__(o Time) bool`**

Return true if `self` is later in the day than `o`.

<a id="time.__ge__"></a>

**`__ge__(o Time) bool`**

Return true if `self` is not earlier in the day than `o`.

## Functions

<a id="parse_num"></a>

### `parse_num`

```kl
func parse_num(seg str) int
```

Parse a fixed-width all-digit ISO segment into an int; panic on any non-digit.
Used by `from_str` so a malformed field reports a datetime-specific message
instead of a generic integer-parse failure.

<a id="floor_days"></a>

### `floor_days`

```kl
func floor_days(ns int) int
```

Whole days spanned by `ns` nanoseconds, rounded toward negative infinity
(matches Python's `timedelta.days` semantics for negative durations).

<a id="is_leap_year"></a>

### `is_leap_year`

```kl
pub func is_leap_year(y int) bool
```

Return true if `y` is a leap year in the proleptic Gregorian calendar.

<a id="days_in_month"></a>

### `days_in_month`

```kl
pub func days_in_month(y int, m int) int
```

Return the number of days in month `m` (1-12) of year `y`.

Panic if `m` is outside 1..12.

<a id="days_from_civil"></a>

### `days_from_civil`

```kl
func days_from_civil(y int, m int, d int) int
```

Return the day count from the Unix epoch (1970-01-01 == 0) to the civil
date `(y, m, d)`; negative before the epoch.

Howard Hinnant's `days_from_civil`. The era adjustment keeps it correct
under Koala's truncating (toward-zero) integer division.

<a id="civil_from_days"></a>

### `civil_from_days`

```kl
func civil_from_days(z int) (int, int, int)
```

Return the civil date `(year, month, day)` for a day count `z` from the
Unix epoch (1970-01-01 == 0); the inverse of `days_from_civil`.

## Constants

<a id="minyear"></a>

### `MINYEAR`

```kl
pub const MINYEAR = 1
```

The smallest year supported by `Date` (matches Python's `MINYEAR`).

<a id="maxyear"></a>

### `MAXYEAR`

```kl
pub const MAXYEAR = 9999
```

The largest year supported by `Date` (matches Python's `MAXYEAR`).

<a id="nanos_per_sec"></a>

### `NANOS_PER_SEC`

```kl
const NANOS_PER_SEC = 1_000_000_000
```

<a id="nanos_per_min"></a>

### `NANOS_PER_MIN`

```kl
const NANOS_PER_MIN = 60_000_000_000
```

<a id="nanos_per_hour"></a>

### `NANOS_PER_HOUR`

```kl
const NANOS_PER_HOUR = 3_600_000_000_000
```

<a id="nanos_per_day"></a>

### `NANOS_PER_DAY`

```kl
const NANOS_PER_DAY = 86_400_000_000_000
```
