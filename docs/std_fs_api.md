# Koala `std/fs` API Reference

## Contents

**Classes**

| Name | Summary |
|------|---------|
| [`File`](#file) | An opened file providing byte-level read, write and seek access. |
| [`Path`](#path) | The `Path` class represents a structural filesystem path. |

**Functions**

| Name | Summary |
|------|---------|
| [`open`](#open) | Open the file at `path` and return a `File` object. |
| [`_path_str`](#_path_str) | Convert a `Path \| str` argument to the raw `str` path. |
| [`_list_dir`](#_list_dir) |  |
| [`read_bytes`](#read_bytes) | Read the file `path` into `out` and return the count read. |
| [`write_bytes`](#write_bytes) | Write `data` to the file `path`, creating or truncating it. |
| [`write_str`](#write_str) | Encode `data` as UTF-8 and write it to the file `path`, creating or truncating it. |
| [`exists`](#exists) | Return true if the file or directory exists on the physical disk. |
| [`is_file`](#is_file) | Return true if the path points to a regular file. |
| [`is_dir`](#is_dir) | Return true if the path points to a directory. |
| [`size`](#size) | Return the size of the file in bytes. Panics if the file does not exist. |
| [`remove`](#remove) | Permanently delete a file from the disk. Returns true on success. |
| [`rename`](#rename) | Rename or move the file/directory `src` to `dst`. |
| [`mkdir`](#mkdir) | Create a directory, including any missing parent directories (mkdir -p). An existing directory is treated as success. Return true on success, false on failure. |
| [`rmdir`](#rmdir) | Remove an empty directory. Returns false if the directory is not empty. |
| [`list_dir`](#list_dir) | Read the directory contents and return a list of strongly-typed `path.Path` objects. |
| [`read_all`](#read_all) | Read the entire file `path` into a new `bytes`. |
| [`read_str`](#read_str) | Read the entire file `path` and decode it as UTF-8 into a `str`. |

## Classes

<a id="file"></a>

### `File`

```kl
pub class File : io.Reader & io.Writer & io.Seeker & io.Closable {
    pub let path str
    pub let mode str
    pub let closed bool
    pub let fd int
    pub let is_tty bool
    func __init__()
    pub func read(bs bytes) int
    pub func write(bs bytes) int
    pub func write_str(s str) int
    pub func seek(offset int, whence int) int
    pub func tell() int
    pub func truncate(size = -1) bool
    pub func close()
    pub func flush()
}
```
**Bases:** `io.Reader` & `io.Writer` & `io.Seeker` & `io.Closable`


An opened file providing byte-level read, write and seek access.

It implements the `io.Reader`, `io.Writer`, `io.Seeker` and `io.Closable` protocols.
Obtain instances via `fs.open` or `open` only.

See: `fs.open` and `open`.

---

**Fields**

<a id="file.path"></a>

**`pub let path str`**

The filesystem path this file was opened from.

<a id="file.mode"></a>

**`pub let mode str`**

The mode string passed to `fs.open`: `"r"`, `"w"`, `"x"`,
`"a"`, `"r+"`, `"w+"`, `"x+"` or `"a+"`.

<a id="file.closed"></a>

**`pub let closed bool`**

Whether this file has been closed.

<a id="file.fd"></a>

**`pub let fd int`**

The raw file descriptor associated with this file.

<a id="file.is_tty"></a>

**`pub let is_tty bool`**

True if the file is connected to a terminal (tty).

<a id="file.__init__"></a>

**`__init__()`** — *`@native`*

Private constructor.

User cannot directly create instances of this class.

<a id="file.read"></a>

**`read(bs bytes) int`** — *`@native`*

Read up to `bs.len()` bytes into `bs` and return the count read.

Return 0 at EOF, or -1 on error. Panic when writing a closed file.

<a id="file.write"></a>

**`write(bs bytes) int`** — *`@native`*

Write `bs.len()` bytes from `bs` and return the count written.

Return -1 on error. Panic when writing a closed file.

<a id="file.write_str"></a>

**`write_str(s str) int`** — *`@native`*

Write the UTF-8 encoding of the string to the file.

Returns the number of bytes actually written.
Return -1 on error. Panic when writing a closed file.

<a id="file.seek"></a>

**`seek(offset int, whence int) int`** — *`@native`*

Move the file cursor by `offset` relative to `whence` and return the new absolute position.

`whence`: 0 = start of file, 1 = current position, 2 = end of file.

Return -1 on error. Panic when seeking a closed file.

See: `io.SEEK_SET`, `io.SEEK_CUR`, `io.SEEK_END` for the `whence` values.

<a id="file.tell"></a>

**`tell() int`** — *`@native`*

Return the current cursor position.

Return -1 on error. Panic when querying the position of a closed file.

<a id="file.truncate"></a>

**`truncate(size = -1) bool`** — *`@native`*

Truncate the file to `size` bytes.

If `size` is -1, truncate at the current cursor position. The cursor itself is not moved.

Return true on success, false on error. Panic when truncating a closed file.

<a id="file.close"></a>

**`close()`** — *`@native`*

Close the file and release the underlying file descriptor.

Subsequent I/O operations on this file will panic.

<a id="file.flush"></a>

**`flush()`**

Satisfy the `io.Writer` protocol; a raw file is unbuffered, so this only checks
if the file is closed.

<a id="path"></a>

### `Path`

```kl
pub class Path : ToString {
    let path str
    pub func __init__(p str)
    pub func name() str
    pub func ext() str
    pub func stem() str
    pub func parent() Path
    pub func is_absolute() bool
    pub func normalize() Path
    pub func absolute() Path
    pub func to_str() str
    pub func fmt(f Formatter)
    pub func __eq__(rhs Path) bool
    pub func __ne__(rhs Path) bool
    pub func cat(segment Path | str) Path
    pub func hash() int
}
```
**Bases:** `ToString`


The `Path` class represents a structural filesystem path.

It treats paths as clean logical objects rather than raw strings,
automatically handling platform-specific separators (\ for Windows, / for Unix).

---

**Fields**

<a id="path.path"></a>

**`let path str`**

The internal string representation of the path.

<a id="path.__init__"></a>

**`__init__(p str)`**

Create a new `Path` from a string representation.

<a id="path.name"></a>

**`name() str`**

Return the final component of the path (e.g., returns "file.kl" for "/a/b/file.kl").
If the path ends in a separator, returns the directory name.

<a id="path.ext"></a>

**`ext() str`**

Return the file extension without the leading dot (e.g., returns "kl" for "file.kl").
Returns an empty string if no extension exists.

<a id="path.stem"></a>

**`stem() str`**

Return the pure filename without its extension (e.g., returns "file" for "file.kl").

<a id="path.parent"></a>

**`parent() Path`**

Return a new `Path` object representing the parent directory.
Returns self if the path is already the root directory.

<a id="path.is_absolute"></a>

**`is_absolute() bool`**

Return true if the path is absolute.
(e.g., starts with "/" on Unix or "C:\" on Windows).

<a id="path.normalize"></a>

**`normalize() Path`**

Return a normalized version of the path.
Eliminates redundant separators and up-level references (e.g., "a//b", "a/./b", "a/b/../c" -> "a/c").

<a id="path.absolute"></a>

**`absolute() Path`**

Return an absolute version of the path by lexically expanding relative tokens.

<a id="path.to_str"></a>

**`to_str() str`**

Convert the path to its string form.

This returns the path itself without adding a display-oriented wrapper.

<a id="path.fmt"></a>

**`fmt(f Formatter)`**

Format the path for display.

The display form includes the `Path(...)` wrapper.

<a id="path.__eq__"></a>

**`__eq__(rhs Path) bool`**

Compare two paths for equality (Equatable[Path] trait).

<a id="path.__ne__"></a>

**`__ne__(rhs Path) bool`**

Compare two paths for inequality.

<a id="path.cat"></a>

**`cat(segment Path | str) Path`**

Connect a new path segment or another Path to the current one.

Returns a brand new Path object, allowing fluent method chaining.
Thanks to Koala's union type support, it effortlessly accepts str or Path.

Example:
    let p = fs.Path("abc").cat("def").cat("ok")

<a id="path.hash"></a>

**`hash() int`**

Return the hash code of the current path.

## Functions

<a id="open"></a>

### `open`

```kl
pub func open(path str, mode = "r") File?
```

*`@native`*

Open the file at `path` and return a `File` object.

`mode` selects the access:
- `"r"`: read-only, the file must exist.
- `"w"`: write-only, create or truncate.
- `"x"`: write-only, exclusive create, fail if the file exists.
- `"a"`: write-only, create if absent, always append at the end.
- `"r+"`: read and write, the file must exist.
- `"w+"`: read and write, create or truncate.
- `"x+"`: read and write, exclusive create, fail if the file exists.
- `"a+"`: read and write, create if absent, always append at the end.

Return nil if the file cannot be opened (missing file, permission
denied, or file already exists for `"x"`/`"x+"`).
Files created by `"w"`, `"x"`, `"a"` and their `"+"` variants get
permission 0644.

Example:
    let f = fs.open("hello.txt", "w")
    f?.write("hello".to_bytes())
    f?.close()

See: `File`.

<a id="_path_str"></a>

### `_path_str`

```kl
func _path_str(p Path | str) str
```

Convert a `Path | str` argument to the raw `str` path.

Now only used by `list_dir` (which stays a `.kl` wrapper); every other
`Path | str` function is a direct `@native` that reads its argument in C.

<a id="_list_dir"></a>

### `_list_dir`

```kl
func _list_dir(p str) list[str]
```

*`@native`*

<a id="read_bytes"></a>

### `read_bytes`

```kl
pub func read_bytes(path Path | str, out bytes) int
```

*`@native`*

Read the file `path` into `out` and return the count read.

Reading stops when `out` is full or end of file is reached, so
the caller controls the memory used.

Return 0 for an empty file, or -1 if the file cannot be opened or read.
The file is closed before returning.

See: `write_bytes`, `File.read`.

<a id="write_bytes"></a>

### `write_bytes`

```kl
pub func write_bytes(path Path | str, data bytes) bool
```

*`@native`*

Write `data` to the file `path`, creating or truncating it.

All bytes are written or the operation fails; return true on success,
false if the file cannot be opened or written.
The file is closed before returning.

See: `write_str`, `read_bytes`.

<a id="write_str"></a>

### `write_str`

```kl
pub func write_str(path Path | str, data str) bool
```

*`@native`*

Encode `data` as UTF-8 and write it to the file `path`,
creating or truncating it.

All bytes are written or the operation fails; return true on success,
false if the file cannot be opened or written.
The file is closed before returning.

See: `write_bytes`, `read_bytes`.

<a id="exists"></a>

### `exists`

```kl
pub func exists(p Path | str) bool
```

*`@native`*

Return true if the file or directory exists on the physical disk.

Implemented natively: the C side reads a `Path | str` argument directly,
so no `_path_str` normalization shim is needed.

<a id="is_file"></a>

### `is_file`

```kl
pub func is_file(p Path | str) bool
```

*`@native`*

Return true if the path points to a regular file.

<a id="is_dir"></a>

### `is_dir`

```kl
pub func is_dir(p Path | str) bool
```

*`@native`*

Return true if the path points to a directory.

<a id="size"></a>

### `size`

```kl
pub func size(p Path | str) int64
```

*`@native`*

Return the size of the file in bytes. Panics if the file does not exist.

<a id="remove"></a>

### `remove`

```kl
pub func remove(p Path | str) bool
```

*`@native`*

Permanently delete a file from the disk. Returns true on success.

<a id="rename"></a>

### `rename`

```kl
pub func rename(src Path | str, dst Path | str) bool
```

*`@native`*

Rename or move the file/directory `src` to `dst`.

Both paths must be on the same filesystem; a cross-device move returns
false (use a copy + remove sequence for that case). An existing `dst`
file is overwritten; an existing `dst` directory must be empty.
Return true on success, false on failure.

<a id="mkdir"></a>

### `mkdir`

```kl
pub func mkdir(p Path | str) bool
```

*`@native`*

Create a directory, including any missing parent directories (mkdir -p).
An existing directory is treated as success.
Return true on success, false on failure.

<a id="rmdir"></a>

### `rmdir`

```kl
pub func rmdir(p Path | str) bool
```

*`@native`*

Remove an empty directory. Returns false if the directory is not empty.

<a id="list_dir"></a>

### `list_dir`

```kl
pub func list_dir(p Path | str) list[Path]
```

Read the directory contents and return a list of strongly-typed `path.Path` objects.

Each entry is joined with the directory path (e.g. `list_dir("/tmp")`
yields `Path("/tmp/a.txt")`), so the returned paths can be used
directly; the base name is still available via `Path.name`.
The special entries `.` and `..` are skipped and the result is
sorted in ascending order. Panics if the directory cannot be opened.

<a id="read_all"></a>

### `read_all`

```kl
pub func read_all(path Path | str, max = 65_536) bytes
```

Read the entire file `path` into a new `bytes`.

A convenience over `open` + a chunked `File.read` loop: it `stat`s the file
for its size, allocates exactly that many bytes, and reads them in one shot.

Because the whole file is loaded into memory, the read is guarded by `max`:
if the file is larger than `max` bytes it panics *before* allocating. The
default cap is 64 KiB, chosen for memory-constrained (embedded) targets;
pass a larger `max`, or `-1` for unlimited, to override.

For a file that legitimately does not fit in memory, stream it instead:
`fs.open(path)` + `File.read(chunk)` in a loop, or `io.read_all(reader)` /
`io.lines(...)`. `read_all` is meant for small files.

Panics if the file does not exist (via `size`) or cannot be read.

See: `read_str`, `read_bytes`, `write_bytes`.

<a id="read_str"></a>

### `read_str`

```kl
pub func read_str(path Path | str, max = 65_536) str
```

Read the entire file `path` and decode it as UTF-8 into a `str`.

Same size guard and streaming guidance as `read_all`; this is simply
`read_all(path, max).to_str()`.

Panics if the file does not exist, exceeds `max` (default 64 KiB, `-1` for
unlimited), or cannot be read.

See: `read_all`, `write_str`.
