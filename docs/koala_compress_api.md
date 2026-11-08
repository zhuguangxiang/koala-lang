# Koala compression API: `koala/zlib`, `koala/gzip`, `koala/zipfile`

A 1:1 port of CPython's compression modules: `zlib`, `gzip` and `zipfile`.
Every name, parameter order, default and return type is Python's, so a Python
file ports by changing the import line.

The DEFLATE engine is **zlib 1.3.1**, vendored unmodified in
`libs/koala/zlib/native/zlib/` (upstream `LICENSE` and `VERSION` are kept
there), compiled into `libkoala_zlib.so`. No system `libz` is needed, and
because CPython 3.14.4 also ships zlib 1.3.1, the compressed bytes are
**identical** to Python's for the same input, level and window size - not just
equivalent, byte for byte. `gzip` and `zipfile` are pure Koala on top of it.

| Package | Depends on | Native code |
|---------|-----------|-------------|
| `koala/zlib` | - | vendored zlib 1.3.1 + `zlib_shim.c`, `compressobj.c`, `decompressobj.c` |
| `koala/gzip` | `koala/zlib` | none |
| `koala/zipfile` | `koala/zlib` | none |
| `koala/tarfile` | `koala/gzip` | none |

`koala/tarfile` - the 1:1 port of CPython's `tarfile` - is documented apart,
in [`koala_tarfile_api.md`](koala_tarfile_api.md), because its API surface
(`TarInfo`, `TarFile`, the PEP 706 filters) is much larger than the three
modules below; it uses `koala/gzip` for its `:gz` modes.

## Contents

**`koala/zlib`**

| Name | Summary |
|------|---------|
| [`compress`](#zlib-compress) | Compress `data` in one call: `compress(data, level=-1, wbits=15)`. |
| [`decompress`](#zlib-decompress) | Decompress one stream: `decompress(data, wbits=15, bufsize=16384)`. |
| [`crc32`](#zlib-checksums) | CRC-32 (IEEE 802.3), chainable through `value`. |
| [`adler32`](#zlib-checksums) | Adler-32, the checksum the zlib format appends. |
| [`compressobj`](#zlib-compressobj) | A `Compress` for chunked compression. |
| [`decompressobj`](#zlib-decompressobj) | A `Decompress` for chunked decompression. |
| [`Compress`](#zlib-compress) | The stream compressor: `compress()`, `flush()`, `copy()`. |
| [`Decompress`](#zlib-decompress) | The stream decompressor: `decompress()`, `flush()`, `copy()`, plus `eof`, `unused_data`, `unconsumed_tail`. |

**`koala/gzip`**

| Name | Summary |
|------|---------|
| [`compress`](#gzip-compress) | A complete gzip member: `compress(data, compresslevel=9, mtime=0)`. |
| [`decompress`](#gzip-decompress) | Every member in `data`, concatenated. |
| [`open`](#gzip-open) | `open(filename, mode="rb", compresslevel=9)` -> `GzipFile`. |
| [`GzipFile`](#gzip-gzipfile) | Reading and writing a gzip file. |

**`koala/zipfile`**

| Name | Summary |
|------|---------|
| [`is_zipfile`](#zipfile-is_zipfile) | Is this path or these bytes a ZIP archive? |
| [`ZipInfo`](#zipfile-zipinfo) | One member's metadata: `filename`, `date_time`, `CRC`, ... |
| [`ZipFile`](#zipfile-zipfile) | The archive: `namelist`, `read`, `writestr`, `extract`, ... |

## Differences from Python

The API is the Python API wherever the language allows it. These deltas are
unavoidable, and all of them are deliberate:

| Python | Koala | Why |
|--------|-------|-----|
| raises `zlib.error`, `gzip.BadGzipFile`, `zipfile.BadZipFile`, `ValueError`, `EOFError`, `KeyError` | panics with the same message text, prefixed `zlib: ` / `gzip: ` / `zipfile: ` | Koala has no exceptions; a panic carries one message. |
| `with gzip.open(...) as f:` | `f.close()` (idempotent) | Koala has no `with`, and the runtime does not run finalizers yet. |
| `gzip.open(f, "rt", encoding=...)` | panics; use `"rb"` / `"wb"` | The text modes wrap the byte stream in `io.TextIOWrapper`; Koala has no equivalent. |
| `gzip.GzipFile(filename, mode, compresslevel, fileobj, mtime)` | `GzipFile(filename, mode, compresslevel, mtime)` | Koala's file types are distinct classes, not one protocol, so a gzip member is handled whole in memory instead of through a `fileobj`. |
| `f.mode`, `f.mtime` (attributes) | `f.mode()`, `f.mtime()` (methods) | Koala rejects a constructor parameter that shadows a field, and both are also keyword arguments. |
| `ZipInfo(filename=..., date_time=...)` | `ZipInfo(arcname, dt)` | Same rule: `filename` and `date_time` are field names. Positional calls are unaffected. |
| `ZipFile.write(filename, arcname, ...)` | `ZipFile.write(path, arcname, ...)` | `filename` is the archive's own attribute. |
| `zipfile.ZIP_BZIP2`, `ZIP_LZMA` | declared, but writing panics | Koala has no bzip2 or LZMA codec. |
| `zipfile.Path`, `ZipFile.open()`, `pwd=` | not provided (`pwd` panics) | They need file-object protocols and a decryption layer Koala does not have. |

Two behaviours are stricter than Python's, in the direction of failing loudly:

- `zipfile.ZipFile.read()` always verifies the member's CRC-32 and panics on a
  mismatch, where Python only does so when the caller opted in.
- `zlib.Compress._close()` and `zlib.Decompress._close()` are Koala-only and
  underscore-private: `gzip` and `zipfile` call them because a finalizer would
  never run. Do not call them from application code.

## `koala/zlib`

<a id="zlib-compress"></a>
### `compress(data, level = Z_DEFAULT_COMPRESSION, wbits = MAX_WBITS) -> bytes`

Compress `data` in one call.

`level` is `-1` ("zlib's choice") or `0`..`9`. `wbits` selects the wrapper:
`8`..`15` for the zlib format (RFC 1950, default `15`), `-8`..`-15` for a raw
DEFLATE stream (RFC 1951), `16 + window` for gzip (RFC 1952), so `31` is the
usual gzip choice.

```kl
zlib.compress(b"hello world")
// 789ccb48cdc9c95728cf2fca4901001a0b045d
```

<a id="zlib-decompress"></a>
### `decompress(data, wbits = MAX_WBITS, bufsize = DEF_BUF_SIZE) -> bytes`

Decompress one stream. `wbits` uses the same numbering as `compress()`, and
`47` accepts any of the three wrappers. `bufsize` is only the initial output
buffer size - the result does not depend on it.

```kl
zlib.decompress(zlib.compress(b"hello world"))   // b"hello world"
zlib.decompress(gzip_bytes, 47)                  // any wrapper, like Python
```

<a id="zlib-checksums"></a>
### `crc32(data, value = 0) -> int` / `adler32(data, value = 1) -> int`

The checksums, chainable: pass the previous result as `value` to checksum a
stream chunk by chunk. Both return the unsigned 32-bit value Python returns.

```kl
zlib.crc32(b"hello world")            // 222957957
zlib.crc32(b"ab", zlib.crc32(b"x"))   // 2063186536
zlib.adler32(b"hello world")          // 436929629
```

<a id="zlib-compressobj"></a>
### `compressobj(level = Z_DEFAULT_COMPRESSION, method = DEFLATED, wbits = MAX_WBITS, memLevel = DEF_MEM_LEVEL, strategy = Z_DEFAULT_STRATEGY, zdict = nil) -> Compress`

<a id="zlib-compress"></a>
### `Compress`

| Member | Python | Notes |
|--------|--------|-------|
| `compress(data) -> bytes` | `Compress.compress` | May return `b""`: the engine buffers until it has a block, so concatenate everything including `flush()`. |
| `flush(mode = Z_FINISH) -> bytes` | `Compress.flush` | `mode` is a `Z_*_FLUSH` constant. After `Z_FINISH` the stream is done. |
| `copy() -> Compress` | `Compress.copy` | An independent compressor that shares the output produced so far. |
| `_close()` | - | Internal; see the deviations table. |

```kl
let co = zlib.compressobj(6)
let out = Buffer()
out.write(co.compress(b"hello "))
out.write(co.compress(b"world"))
out.write(co.flush())
// out.to_bytes() == zlib.compress(b"hello world")
```

<a id="zlib-decompressobj"></a>
### `decompressobj(wbits = MAX_WBITS, zdict = nil) -> Decompress`

<a id="zlib-decompress"></a>
### `Decompress`

The three attributes are **fields**, as in Python, because they are data the
engine writes rather than computations:

| Field | Python | Notes |
|-------|--------|-------|
| `eof bool` | `Decompress.eof` | The end of the compressed stream has been reached. |
| `unused_data bytes` | `Decompress.unused_data` | Bytes found after the end of the stream; anything offered after `eof` is appended here. |
| `unconsumed_tail bytes` | `Decompress.unconsumed_tail` | Input the last `decompress()` call did not consume because `max_length` was reached. Feed it back: `chunk = d.unconsumed_tail`. |

| Member | Python | Notes |
|--------|--------|-------|
| `decompress(data, max_length = 0) -> bytes` | `Decompress.decompress` | `max_length = 0` means no limit. |
| `flush(length = DEF_BUF_SIZE) -> bytes` | `Decompress.flush` | Does not panic on truncated input; returns what could be decompressed. |
| `copy() -> Decompress` | `Decompress.copy` | An independent decompressor in the same state. |
| `_close()` | - | Internal; see the deviations table. |

One documented difference: CPython keeps the pending tail *twice* - in
`unconsumed_tail` and inside the engine - so `decompress(b"")` after a
`max_length` call resumes the stream there (and drops it in some edge cases).
Here the tail lives only in `unconsumed_tail`, which is the documented usage
and has no edge cases:

```kl
let d = zlib.decompressobj()
let first = d.decompress(packed, 8)     // at most 8 bytes
let rest = d.decompress(d.unconsumed_tail)
```

### Constants

| Constant | Value | Meaning |
|----------|-------|---------|
| `DEFLATED` | 8 | The DEFLATE method (Python's name: there is no `Z_DEFLATED`). |
| `MAX_WBITS` | 15 | The largest window: 32 KiB. |
| `DEF_MEM_LEVEL` | 8 | Default `memLevel`. |
| `DEF_BUF_SIZE` | 16384 | Default chunk size for `decompress()` and `flush()`. |
| `Z_DEFAULT_COMPRESSION` | -1 | "zlib's choice" (currently 6). |
| `Z_NO_COMPRESSION` | 0 | Store, do not compress. |
| `Z_BEST_SPEED` / `Z_BEST_COMPRESSION` | 1 / 9 | Level aliases. |
| `Z_DEFAULT_STRATEGY` | 0 | Normal Huffman coding. |
| `Z_FILTERED` / `Z_HUFFMAN_ONLY` / `Z_RLE` / `Z_FIXED` | 1 / 2 / 3 / 4 | Compression strategies. |
| `Z_NO_FLUSH` / `Z_PARTIAL_FLUSH` / `Z_SYNC_FLUSH` / `Z_FULL_FLUSH` / `Z_FINISH` / `Z_BLOCK` / `Z_TREES` | 0 / 1 / 2 / 3 / 4 / 5 / 6 | `flush()` modes. |
| `ZLIB_VERSION` | `"1.3.1"` | The version the engine was compiled from. |
| `ZLIB_RUNTIME_VERSION` | `"1.3.1"` | The version the loaded engine reports. |

## `koala/gzip`

The gzip container (RFC 1952): a ten-byte header, a raw DEFLATE stream from
`koala/zlib`, and an eight-byte trailer holding the CRC-32 and the length. The
header and the trailer are written byte for byte like CPython's - XFL is 2 at
level 9 and 4 at level 1 (0 otherwise), OS is 255, no file name is stored -
so `gzip.compress()` output is identical to Python's.

<a id="gzip-compress"></a>
### `compress(data, compresslevel = 9, mtime = 0) -> bytes`

```kl
gzip.compress(b"hello world")
// 1f8b08000000000002ffcb48cdc9c95728cf2fca49010085114a0d0b000000
gzip.compress(b"hello world", 9, 1234)   // mtime in the header
```

Python's default `mtime` is 0 as well, which keeps archives reproducible.

<a id="gzip-decompress"></a>
### `decompress(data) -> bytes`

Decompresses **every** member in `data` and concatenates them, exactly as
CPython does (so `gzip.compress(a) + gzip.compress(b)` round-trips to
`a + b`).

<a id="gzip-open"></a>
### `open(filename, mode = "rb", compresslevel = 9) -> GzipFile`

`mode` is `"rb"` or `"wb"`; anything else panics (see the deviations table).

<a id="gzip-gzipfile"></a>
### `GzipFile`

| Member | Python | Notes |
|--------|--------|-------|
| `name str` (field) | `GzipFile.name` | The file this object reads or writes. |
| `closed bool` (field) | `GzipFile.closed` | True once `close()` ran. |
| `mode() -> str` | `GzipFile.mode` | `"rb"` or `"wb"`. |
| `mtime() -> int` | `GzipFile.mtime` | Read: the header's timestamp. Write: what `close()` will store. |
| `read(size = -1) -> bytes` | `read` | |
| `read1(size = -1) -> bytes` | `read1` | Same as `read`: the member is already in memory. |
| `readline(size = -1) -> bytes` | `readline` | Includes the trailing newline when present. |
| `readlines() -> list[bytes]` | `readlines` | |
| `peek(n = 1) -> bytes` | `peek` | Does not move the read position. |
| `seek(offset, whence = 0) -> int` | `seek` | `whence` 0/1/2; seeking past the end is allowed. |
| `tell() -> int` | `tell` | |
| `rewind()` | `rewind` | Back to the start. |
| `write(data) -> int` | `write` | Buffers; returns the byte count. |
| `writelines(lines)` | `writelines` | |
| `flush()` | `flush` | Nothing to do: output is produced by `close()`. |
| `close()` | `close` | Writes the file in write mode; idempotent. |

```kl
let w = gzip.open("hello.txt.gz", "wb")
w.write(b"hello world")
w.close()                       // this is what writes the gzip member

let r = gzip.open("hello.txt.gz")
print(r.read())                 // b"hello world"
r.close()
```

### Constants

| Constant | Value | Meaning |
|----------|-------|---------|
| `FTEXT` | 1 | Header flag: probably ASCII text. |
| `FHCRC` | 2 | Header flag: a 16-bit header CRC follows. |
| `FEXTRA` | 4 | Header flag: an extra field follows. |
| `FNAME` | 8 | Header flag: the original file name follows. |
| `FCOMMENT` | 16 | Header flag: a comment follows. |
| `READ` / `WRITE` | 1 / 2 | The values behind Python's `GzipFile.mode` constants. |

## `koala/zipfile`

ZIP archives as APPNOTE.TXT defines them: local file headers, a central
directory, and an end-of-central-directory record. Reading parses the central
directory and decompresses a member on demand; writing records members and
lays the bytes down in `close()`. `ZIP_DEFLATED` members go through
`koala/zlib` with a raw DEFLATE stream, so the compressed data matches
CPython's.

<a id="zipfile-is_zipfile"></a>
### `is_zipfile(source) -> bool`

`source` is a path (`str`) or the archive bytes; a path that does not exist is
simply not an archive, as in Python.

```kl
zipfile.is_zipfile("archive.zip")     // true / false
zipfile.is_zipfile(archive_bytes)     // same question, no file needed
```

<a id="zipfile-zipinfo"></a>
### `ZipInfo(arcname = "NoName", dt = nil) -> ZipInfo`

One member's metadata. Every attribute is a **field**, with Python's name:

| Field | Python | Meaning |
|-------|--------|---------|
| `filename str` | `filename` | The member name inside the archive. |
| `date_time (int,int,int,int,int,int)` | `date_time` | `(year, month, day, hour, minute, second)`; defaults to 1980-01-01. |
| `compress_type int` | `compress_type` | `ZIP_STORED`, `ZIP_DEFLATED`, ... |
| `comment bytes` / `extra bytes` | `comment` / `extra` | The member comment and extra field. |
| `CRC int` | `CRC` | CRC-32 of the uncompressed data. |
| `compress_size int` / `file_size int` | `compress_size` / `file_size` | Sizes in bytes. |
| `header_offset int` | `header_offset` | Where the local header starts. |
| `create_system int` / `create_version int` / `extract_version int` | same | Who made it, and what is needed to read it. |
| `flag_bits int` / `volume int` | same | General purpose flags, volume number. |
| `internal_attr int` / `external_attr int` | same | Attributes, Unix mode in the high bits. |
| `orig_filename str` | `orig_filename` | The name as stored. |

`is_dir()` is True when the name ends in `/`.

```kl
let info = zipfile.ZipInfo("notes.txt")
let (year, month, day, hour, minute, second) = info.date_time
```

<a id="zipfile-zipfile"></a>
### `ZipFile(file, mode = "r", compression = ZIP_STORED, allowZip64 = true, compresslevel = nil) -> ZipFile`

`mode` is `"r"`, `"w"`, `"x"` (refuse to overwrite) or `"a"` (keep the members
already there). `compression` is the default for members written without an
explicit choice; `compresslevel` is the DEFLATE level (`nil` = the engine
default).

| Member | Python | Notes |
|--------|--------|-------|
| `filename str` (field) | `filename` | The archive path. |
| `comment bytes` (field) | `comment` | The archive comment. |
| `mode() -> str` | `mode` | The mode string. |
| `namelist() -> list[str]` | `namelist` | Member names, in central-directory order. |
| `infolist() -> list[ZipInfo]` | `infolist` | A fresh list of the members. |
| `getinfo(name) -> ZipInfo` | `getinfo` | Panics with Python's `KeyError` wording when absent. |
| `read(name, pwd = nil) -> bytes` | `read` | Verifies the CRC-32; `pwd` panics (no encryption). |
| `writestr(arcname, data, compress_type = nil, compresslevel = nil)` | `writestr` | `arcname` is a `ZipInfo` or a `str`; `data` is `bytes` or `str`. |
| `write(path, arcname = "", compress_type = nil, compresslevel = nil)` | `write` | Adds a file from disk; `arcname` defaults to its base name. |
| `extract(member, path = "") -> str` | `extract` | Writes the file, creating directories; returns the path. |
| `extractall(path = "", members = nil)` | `extractall` | |
| `printdir()` | `printdir` | Prints a table of the contents. |
| `testzip() -> str?` | `testzip` | The first member whose CRC-32 fails, or `nil`. |
| `close()` | `close` | Writes the archive in write modes; idempotent. |

```kl
let zf = zipfile.ZipFile("out.zip", "w", zipfile.ZIP_DEFLATED)
zf.writestr("hello.txt", "hello world")
zf.close()                                  // the archive appears here

let zr = zipfile.ZipFile("out.zip")
print(zr.namelist())                        // [hello.txt]
print(zr.read("hello.txt"))                 // b"hello world"
print(zr.testzip())                         // nil: every CRC is fine
zr.close()
```

### Constants

| Constant | Value | Meaning |
|----------|-------|---------|
| `ZIP_STORED` | 0 | No compression. |
| `ZIP_DEFLATED` | 8 | DEFLATE (supported). |
| `ZIP_BZIP2` | 12 | Declared; writing panics. |
| `ZIP_LZMA` | 14 | Declared; writing panics. |

## Verification

The test suites are golden-vector tests, not just round trips: every expected
value was produced by CPython 3.14.4 with zlib 1.3.1, so a change that alters
the compressed bytes fails a test.

- `libs/koala/zlib/test/test_zlib.kl` - 28 cases: `compress`/`decompress` at
  several levels and window sizes, raw DEFLATE and gzip wrappers, both
  checksums (including chaining), chunked compression (equal to one-shot),
  every flush mode, `copy()`, dictionaries, `max_length`/`unconsumed_tail`,
  `unused_data`/`eof`, the constant set, and the failure messages.
- `libs/koala/gzip/test/test_gzip.kl` - 13 cases: header bytes with `mtime`
  and level, multi-member concatenation, write/read/seek/peek/readline, and
  the failure messages.
- `libs/koala/zipfile/test/test_zipfile.kl` - 11 cases: reading an archive
  built by CPython, writing stored and deflated members, extracting, and the
  failure messages.

`wombat build --all` / `build-and-test.sh` runs them with the rest of the
workspace.