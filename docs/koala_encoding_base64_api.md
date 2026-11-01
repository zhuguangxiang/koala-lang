# Koala `koala/base64` API Reference

RFC 4648 base64 codecs, plus the 76-column MIME layout of RFC 2045.

Every function returns `bytes`, like CPython's `base64` module; only the
`hex` package returns `str`.

## Contents

**Functions**

| Name | Summary |
|------|---------|
| [`encode`](#encode) | Encode `s`, optionally with altchars and without `=` padding. |
| [`decode`](#decode) | Decode base64 text, lax by default or strict with `validate`. |
| [`standard_encode`](#standard_encode) | Encode with the standard alphabet. |
| [`standard_decode`](#standard_decode) | Decode standard-alphabet base64 text. |
| [`url_safe_encode`](#url_safe_encode) | Encode with the URL- and filename-safe alphabet. |
| [`url_safe_decode`](#url_safe_decode) | Decode URL- and filename-safe base64 text. |
| [`encode_mime`](#encode_mime) | Encode as 76-column MIME base64 (RFC 2045). |
| [`decode_mime`](#decode_mime) | Decode base64 text containing line breaks. |

**Reference**

| Topic | Description |
|-------|-------------|
| [`padded`](#padded) | What `padded = false` does. |
| [CPython equivalents](#cpython-equivalents) | Name mapping. |
| [Errors](#errors) | Panic messages. |

## Functions

<a id="encode"></a>

### `encode`

```kl
pub func encode(s bytes, altchars (bytes | str)? = nil, padded = true) bytes
```

Encode `s` with the standard alphabet, mapping `+` and `/` to
`altchars[0]` and `altchars[1]` when `altchars` is given. An empty
sequence encodes to empty bytes.

`altchars` must be exactly two bytes; `nil` means the standard alphabet.
`padded = false` omits the trailing `=`, giving the "raw" form used by
JWT segments and URL parameters.

```kl
base64.encode(b"foobar")                    // b"Zm9vYmFy"
base64.encode(b"\xfb\xff")                  // b"+/8="
base64.encode(b"\xfb\xff", altchars = b"-_") // b"-_8="
base64.encode(b"\xfb\xff", nil, false)      // b"+/8"
```

<a id="decode"></a>

### `decode`

```kl
pub func decode(s bytes | str, altchars (bytes | str)? = nil, validate = false,
                padded = true) bytes
```

Decode base64 text.

By default decoding is lax: bytes outside the alphabet are skipped, so
wrapped, indented or otherwise decorated input decodes without
preprocessing. The padding still has to add up — `Zg` without its `==`
is an error, not a short read.

`validate = true` rejects anything outside the alphabet instead, which is
what CPython's `validate=True` does. `padded = false` expects unpadded
input; see [`padded`](#padded).

A `str` argument is read as ASCII, as CPython does, so
`decode("Zm9vYmFy")` needs no `to_bytes()`.

```kl
base64.decode(b"Zm9vYmFy")                 // b"foobar"
base64.decode(b"Zm9v YmFy")                // b"foobar", whitespace skipped
base64.decode(b"Zg==", validate = true)    // b"f"
base64.decode(b"Zg", padded = false)       // b"f"
```

<a id="standard_encode"></a>

### `standard_encode`

```kl
pub func standard_encode(s bytes, padded = true) bytes
```

Encode with the standard alphabet (RFC 4648 section 4). Same as
`encode(s, nil, padded)`, kept so that the standard and URL-safe calls
read symmetrically.

<a id="standard_decode"></a>

### `standard_decode`

```kl
pub func standard_decode(s bytes | str, padded = true) bytes
```

Decode standard-alphabet base64 text, laxly (no `altchars`, no
`validate`).

<a id="url_safe_encode"></a>

### `url_safe_encode`

```kl
pub func url_safe_encode(s bytes, padded = true) bytes
```

Encode with the URL- and filename-safe alphabet (RFC 4648 section 5),
where `-` and `_` replace `+` and `/`. Keeps the `=` padding unless
`padded` is false.

```kl
base64.url_safe_encode(b"\xfb\xff")             // b"-_8="
base64.url_safe_encode(b"\xfb\xff", false)      // b"-_8", a JWT segment
```

<a id="url_safe_decode"></a>

### `url_safe_decode`

```kl
pub func url_safe_decode(s bytes | str, padded = true) bytes
```

Decode URL- and filename-safe base64 text.

<a id="encode_mime"></a>

### `encode_mime`

```kl
pub func encode_mime(s bytes) bytes
```

Encode `s` as 76-column base64 with a newline after every line
(RFC 2045), the body format of a MIME part.

57 input bytes produce exactly one 76-column line, then a `\n`; a
trailing newline follows every line, including the last. An empty
sequence encodes to empty bytes, and `padded` does not apply — the MIME
layout always pads.

CPython calls this `encodebytes`; see
[`decode_mime`](#decode_mime) for why the name changed.

```kl
base64.encode_mime(b"f")       // b"Zg==\n"
base64.encode_mime(b"foobar")  // b"Zm9vYmFy\n"
base64.encode_mime(bytes(57)).len()  // 77: 76 columns + '\n'
```

<a id="decode_mime"></a>

### `decode_mime`

```kl
pub func decode_mime(s bytes | str) bytes
```

Decode text written by `encode_mime`, or any base64 with embedded
whitespace: this is `decode(s)` with the default lax behaviour, under a
name that says what the input looks like.

```kl
base64.decode_mime(b"Zm9v\nYmFy\n")   // b"foobar"
```

## Reference

<a id="padded"></a>

### `padded`

`padded = false` produces — and expects — the unpadded "raw" form of
RFC 4648 section 3.2, where the trailing `=` is simply absent. CPython
spells the same flag `padded=False`, added for `b64encode`,
`standard_b64encode`, `urlsafe_b64encode` and their decoders.

Encoding drops only the padding; the significant characters of the last
group stay:

| input | `padded = true` | `padded = false` |
|-------|-----------------|------------------|
| `b"f"` | `b"Zg=="` | `b"Zg"` |
| `b"fo"` | `b"Zm8="` | `b"Zm8"` |
| `b"foo"` | `b"Zm9v"` | `b"Zm9v"` (a full group needs no padding) |

Decoding treats `=` as a byte **outside the alphabet**, which is what
CPython documents for `padded=False`: lax mode skips it, so already
padded input still decodes; `validate = true` rejects it with
`base64: only base64 data is allowed`. The character-count rule is
unchanged, so `b"Zm9vYmFyY"` is still one character too many.

```kl
base64.decode(b"Zg", padded = false)                    // b"f"
base64.decode(b"Zg==", padded = false)                  // b"f", '=' skipped
base64.decode(b"Zg==", validate = true, padded = false) // panic
```

<a id="cpython-equivalents"></a>

### CPython equivalents

The names differ from CPython in two ways: the redundant `b64` prefix is
dropped, because the package name already says base64; and `urlsafe` is
respelled `url_safe`, the snake_case the rest of the library uses.
`encodebytes`/`decodebytes` became `encode_mime`/`decode_mime`, because
"bytes" there only records that Python 2's `encode`/`decode` had to be
renamed once `str` and `bytes` were split, while the output is the MIME
body format.

| Koala | CPython | Go |
|-------|---------|-----|
| `encode(s, altchars, padded)` | `b64encode(s, altchars)` | `StdEncoding.EncodeToString` |
| `decode(s, altchars, validate, padded)` | `b64decode(s, altchars, validate)` | `StdEncoding.DecodeString` |
| `standard_encode(s, padded)` | `standard_b64encode(s)` | `StdEncoding` |
| `standard_decode(s, padded)` | `standard_b64decode(s)` | `StdEncoding` |
| `url_safe_encode(s, padded)` | `urlsafe_b64encode(s)` | `URLEncoding` |
| `url_safe_decode(s, padded)` | `urlsafe_b64decode(s)` | `URLEncoding` |
| `encode_mime(s)` | `encodebytes(s)` | *(none)* |
| `decode_mime(s)` | `decodebytes(s)` | *(none)* |

Go has no MIME-line-breaking helper — `encoding/base64.NewEncoder` hands
back a writer and leaves the wrapping to the caller — and no `altchars`
either; its altchars equivalents are the separate `StdEncoding` and
`URLEncoding` values.

<a id="errors"></a>

### Errors

Decoding panics, carrying the message CPython's `binascii` uses:

| Message | Cause |
|---------|-------|
| `base64: incorrect padding` | Missing `=`. |
| `base64: excess padding not allowed` | `=` in the middle, or more than the data needs, under `validate = true`. |
| `base64: only base64 data is allowed` | A byte outside the alphabet under `validate = true`, including `=` when `padded = false`. |
| `base64: number of data characters cannot be 1 more than a multiple of 4` | The character count is 1 past a multiple of 4. |
| `base64: altchars must be exactly 2 bytes` | `altchars` was given but is not two bytes long. |
| `base64: string argument should contain only ASCII characters` | A `str` argument held a non-ASCII character. |

There is no `is_valid` or `try_decode`: the language has exceptions, so
a predicate would only duplicate the decoder and invite callers to race
a check against a decode.