# Koala `koala/hex` API Reference

## Contents

**Functions**

| Name | Summary |
|------|---------|
| [`encode`](#encode) | Encode `b` as a lowercase hexadecimal string. |
| [`decode`](#decode) | Decode the hexadecimal string `s` into bytes. |

## Functions

<a id="encode"></a>

### `encode`

```kl
pub func encode(b bytes) str
```

Encode `b` as a lowercase hexadecimal string.

An empty byte sequence encodes to the empty string. Every byte,
including `0x00`, becomes exactly two digits.

There is no uppercase encoder, matching Go's `encoding/hex` and
CPython's `binascii`, neither of which offers one: callers that need
uppercase write `encode(b).upper()`.

<a id="decode"></a>

### `decode`

```kl
pub func decode(s str) bytes
```

Decode the hexadecimal string `s` into bytes.

Lowercase and uppercase digits may be mixed freely. The empty string
decodes to empty bytes.

Errors

Panic if `s` has an odd number of characters, or if any character is
not a hexadecimal digit.
