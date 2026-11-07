# Koala `koala/uuid` API Reference

UUIDs (RFC 9562) with an API that mirrors Python's `uuid` module. Implemented
in pure Koala: randomness and digests come from `koala/crypto`, the clock from
`std/time`. There is no native code and no dependency on `libuuid`.

## Contents

**Classes**

| Name | Summary |
|------|---------|
| [`UUID`](#uuid) | A 128-bit universally unique identifier, immutable, with Python's member names. |

**Functions**

| Name | Summary |
|------|---------|
| [`uuid3`](#uuid3) | A name-based (version 3) UUID: the MD5 of `ns` followed by `name`, truncated to 128 bits. |
| [`uuid4`](#uuid4) | A random (version 4) UUID, 122 random bits. |
| [`uuid5`](#uuid5) | A name-based (version 5) UUID: the SHA-1 of `ns` followed by `name`, truncated to 128 bits. |
| [`uuid7`](#uuid7) | A time-ordered (version 7) UUID, per RFC 9562. |

**Constants**

| Name | Summary |
|------|---------|
| [`RFC_4122`](#variants) | The RFC 4122 variant, the one every UUID generated here uses. |
| [`RESERVED_NCS`](#variants) | The variant of a UUID whose variant bits are `0xx`. |
| [`RESERVED_MICROSOFT`](#variants) | The variant of a UUID whose variant bits are `110`. |
| [`RESERVED_FUTURE`](#variants) | The variant of a UUID whose variant bits are `111`. |
| [`NAMESPACE_DNS`](#name-spaces) | Name-space for fully-qualified domain names. |
| [`NAMESPACE_URL`](#name-spaces) | Name-space for URLs. |
| [`NAMESPACE_OID`](#name-spaces) | Name-space for ISO object identifiers. |
| [`NAMESPACE_X500`](#name-spaces) | Name-space for X.500 distinguished names. |
| [`NIL`](#nil-and-max) | The nil UUID, all 128 bits zero. |
| [`MAX`](#nil-and-max) | The max UUID, all 128 bits one. |

## Differences from Python

The Koala API is the Python API wherever the language allows it. Three deltas
are unavoidable, and all of them are deliberate:

| Python | Koala | Why |
|--------|-------|-----|
| `u.version`, `u.variant`, `u.hex`, `u.bytes`, … (attributes) | `u.version()`, `u.variant()`, `u.hex()`, `u.to_bytes()`, … (methods) | Koala has no computed properties. |
| `u.bytes` | `u.to_bytes()` | `bytes` is a type keyword, so it cannot be a method name. |
| `u.int` (128-bit integer) | not offered | `int` is 64 bits wide; use `hex()`, `to_str()` or `to_bytes()`. |
| `uuid.UUID(s)` raises `ValueError` | panics; `try_parse` returns nil | Koala has no exceptions. |

Two conveniences are added on top of Python's surface: `UUID.try_parse` for a
nil-returning parse, and the `UUID.from_bytes` / `UUID.from_hex` factories.

Not provided, because a pure-Koala library cannot honour them portably:
`uuid1()` and `uuid6()` (both need the host MAC address and a stable node id),
`uuid8()`, and the `SafeUUID` / `is_safe` reporting of Python 3.7+.

## Classes

<a id="uuid"></a>

### `UUID`

```kl
pub class UUID {
    pub func __init__(x str | bytes)
    pub static func parse(s str) UUID
    pub static func try_parse(s str) UUID?
    pub static func from_bytes(b bytes) UUID
    pub static func from_hex(s str) UUID

    pub func to_bytes() bytes
    pub func bytes_le() bytes
    pub func hex() str
    pub func to_str() str
    pub func urn() str
    pub func fmt(f Formatter)
    pub func version() int?
    pub func variant() str

    pub func time_low() int
    pub func time_mid() int
    pub func time_hi_version() int
    pub func clock_seq_hi_variant() int
    pub func clock_seq_low() int
    pub func node() int
    pub func time() int
    pub func clock_seq() int
    pub func fields() (int, int, int, int, int, int)

    pub func is_nil() bool
    pub func hash() int
    pub func __eq__(rhs UUID) bool
    pub func __ne__(rhs UUID) bool
    pub func __lt__(rhs UUID) bool
    pub func __le__(rhs UUID) bool
    pub func __gt__(rhs UUID) bool
    pub func __ge__(rhs UUID) bool
}
```

A 128-bit universally unique identifier. The value is the sixteen octets in
network byte order, the layout the canonical text form spells out.

Instances are immutable: the constructor copies its input, and every accessor
that returns bytes hands out a fresh copy.

The class conforms to `Printable`, `ToString`, `Equatable`, `Hashable` and
`Comparable`, so `print(u)`, `format("{}", u)`, `==`, `<` and `hash()` all work
directly.

#### `__init__`

```kl
pub func __init__(x str | bytes)
```

Build a UUID from its text form or from sixteen raw octets.

A `str` is accepted in the shapes Python allows: 32 hexadecimal digits,
optionally separated by dashes, optionally wrapped in braces, and optionally
carrying a `urn:uuid:` prefix. Case does not matter.

A `bytes` value must hold exactly sixteen octets in network byte order.

Panics on a malformed string or a byte count other than sixteen.

```kl
let a = uuid.UUID("d4ecc5a3-2665-4046-baf7-1ea19d9fddf6")
let b = uuid.UUID("d4ecc5a326654046baf71ea19d9fddf6")            // same value
let c = uuid.UUID("urn:uuid:d4ecc5a3-2665-4046-baf7-1ea19d9fddf6") // same value
let d = uuid.UUID("{D4ECC5A3-2665-4046-BAF7-1EA19D9FDDF6}")        // same value
```

#### `parse` / `try_parse`

```kl
pub static func parse(s str) UUID
pub static func try_parse(s str) UUID?
```

`parse` is the named form of the string constructor and panics on bad input;
`try_parse` returns nil instead, for untrusted text.

```kl
let u = uuid.UUID.try_parse("nope")
if u == nil {
    print("not a uuid")
} else {
    print(u!.to_str())
}
```

#### `from_bytes` / `from_hex`

```kl
pub static func from_bytes(b bytes) UUID
pub static func from_hex(s str) UUID
```

Explicit factories for the two constructor shapes. Both panic on invalid input,
exactly like the constructor.

#### `to_bytes` / `bytes_le` / `hex`

```kl
pub func to_bytes() bytes
pub func bytes_le() bytes
pub func hex() str
```

`to_bytes` is the sixteen octets in network byte order; `bytes_le` is the
little-endian layout (the first three fields reversed), used by Microsoft's
GUID format; `hex` is the 32 lowercase hexadecimal digits without dashes. Every
call returns a fresh copy.

#### `to_str` / `urn` / `fmt`

```kl
pub func to_str() str
pub func urn() str
pub func fmt(f Formatter)
```

The canonical 36-character text, the same text behind a `urn:uuid:` prefix, and
the `Formatter` writer. `print(u)` and `format("{}", u)` go through `fmt` and
produce the canonical text, matching Python's `str(u)`.

#### `version` / `variant`

```kl
pub func version() int?
pub func variant() str
```

`version` is the version nibble, 1 to 8, or nil when the variant is not RFC
4122 — the same case where Python returns `None`, so the nil UUID reports nil.
`variant` is one of the four variant constants.

#### Fields

```kl
pub func time_low() int
pub func time_mid() int
pub func time_hi_version() int
pub func clock_seq_hi_variant() int
pub func clock_seq_low() int
pub func node() int
pub func time() int
pub func clock_seq() int
pub func fields() (int, int, int, int, int, int)
```

The raw fields, with the same names and masks as Python. `time` and `clock_seq`
are the masked combinations (60 and 14 bits). `fields` returns all six fields
in Python's order, so a destructive read is a single call:

```kl
let (time_low, time_mid, time_hi_version, cs_hi, cs_low, node) = u.fields()
```

#### `is_nil` / `hash` / comparisons

```kl
pub func is_nil() bool
pub func hash() int
pub func __eq__(rhs UUID) bool
pub func __ne__(rhs UUID) bool
pub func __lt__(rhs UUID) bool
pub func __le__(rhs UUID) bool
pub func __gt__(rhs UUID) bool
pub func __ge__(rhs UUID) bool
```

`is_nil` is true when every octet is zero. Equality compares the octets, and
ordering compares them from the most significant down — the same order as
Python's unsigned 128-bit `int`. `hash` is consistent with `__eq__`, which is
what `dict[UUID, T]` requires.

## Functions

<a id="uuid4"></a>

### `uuid4`

```kl
pub func uuid4() UUID
```

A random (version 4) UUID, 122 random bits. The bits come from the OS CSPRNG
through `crypto.random_bytes`. Equivalent to Python's `uuid.uuid4()`.

<a id="uuid7"></a>

### `uuid7`

```kl
pub func uuid7() UUID
```

A time-ordered (version 7) UUID, per RFC 9562. The first 48 bits hold the Unix
time in milliseconds, so values generated in different milliseconds sort
chronologically; the remaining 74 bits are random. Two values generated within
one millisecond are ordered by those random bits, not by call order — the same
caveat Python's `uuid.uuid7` has.

```kl
let id = uuid.uuid7()
print(id.version())   // 7
```

<a id="uuid5"></a>

### `uuid5`

```kl
pub func uuid5(ns UUID, name str) UUID
```

A name-based (version 5) UUID: the SHA-1 of `ns` followed by the UTF-8 bytes of
`name`, truncated to 128 bits. Deterministic — the same name-space and name
always give the same UUID. Equivalent to Python's `uuid.uuid5`.

<a id="uuid3"></a>

### `uuid3`

```kl
pub func uuid3(ns UUID, name str) UUID
```

A name-based (version 3) UUID, using MD5 instead of SHA-1. Deterministic, kept
for interoperability with values minted elsewhere; prefer `uuid5` for new
name-based UUIDs. Equivalent to Python's `uuid.uuid3`.

```kl
// Reference vectors, identical to Python's output.
print(uuid.uuid5(uuid.NAMESPACE_DNS, "python.org").to_str())
// 886313e1-3b8a-5372-9b90-0c9aee199e5d
print(uuid.uuid3(uuid.NAMESPACE_DNS, "python.org").to_str())
// 6fa459ea-ee8a-3ca4-894e-db77e160355e
```

## Constants

<a id="variants"></a>

### Variants

```kl
pub const RFC_4122 = "specified in RFC 4122"
pub const RESERVED_NCS = "reserved for NCS compatibility"
pub const RESERVED_MICROSOFT = "reserved for Microsoft compatibility"
pub const RESERVED_FUTURE = "reserved for future definition"
```

The four strings `variant()` can return, character for character the values
Python's `uuid` module reports.

<a id="name-spaces"></a>

### Name-spaces

```kl
pub let NAMESPACE_DNS = UUID("6ba7b810-9dad-11d1-80b4-00c04fd430c8")
pub let NAMESPACE_URL = UUID("6ba7b811-9dad-11d1-80b4-00c04fd430c8")
pub let NAMESPACE_OID = UUID("6ba7b812-9dad-11d1-80b4-00c04fd430c8")
pub let NAMESPACE_X500 = UUID("6ba7b814-9dad-11d1-80b4-00c04fd430c8")
```

The standard name-spaces for `uuid3` and `uuid5`, the values RFC 9562 appendix
B defines and Python exposes under the same names.

<a id="nil-and-max"></a>

### `NIL` and `MAX`

```kl
pub let NIL = UUID("00000000-0000-0000-0000-000000000000")
pub let MAX = UUID("ffffffff-ffff-ffff-ffff-ffffffffffff")
```

The all-zero and all-one UUIDs. Neither has RFC 4122 variant bits, so both
report a nil `version()`, and their variants are `RESERVED_NCS` and
`RESERVED_FUTURE` respectively — again matching Python.

## Example

```kl
import "koala/uuid"

func main() {
    let id = uuid.uuid4()
    print(id)                            // 0f8fad5b-d9cb-469f-a165-70867728950e
    print(id.version(), id.variant())    // 4 specified in RFC 4122
    print(id.urn())                      // urn:uuid:0f8fad5b-...

    let name_based = uuid.uuid5(uuid.NAMESPACE_URL, "https://example.com/")
    print(name_based.to_str())           // dd2c1780-811a-5296-81c5-178a0ef488bc

    let parsed = uuid.UUID.try_parse(name_based.to_str())
    print(parsed! == name_based)         // true
}
```