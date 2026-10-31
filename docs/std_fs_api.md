# Koala `std/fs` API Reference

## Contents

**Classes**

| Name | Summary |
|------|---------|
| [`File`](#file) | An opened file providing byte-level read, write and seek access. |
| [`Path`](#path) | The `Path` class represents a structural filesystem path. |
| [`Stat`](#stat) | Metadata of a file, directory or symbolic link, as returned by `fs.stat`. |
| [`Walker`](#walker) | An iterator over a directory tree, returned by `fs.walk`. |

**Functions**

| Name | Summary |
|------|---------|
| [`open`](#open) | Open the file at `path` and return a `File` object. |
| [`_path_str`](#_path_str) | Convert a `Path \| str` argument to the raw `str` path. |
| [`_try_list_dir`](#_try_list_dir) |  |
| [`read_bytes`](#read_bytes) | Read the file `path` into `out` and return the count read. |
| [`write_bytes`](#write_bytes) | Write `data` to the file `path`, creating or truncating it. |
| [`write_str`](#write_str) | Encode `data` as UTF-8 and write it to the file `path`, creating or truncating it. |
| [`exists`](#exists) | Return true if the file or directory exists on the physical disk. |
| [`is_file`](#is_file) | Return true if the path points to a regular file. |
| [`is_dir`](#is_dir) | Return true if the path points to a directory. |
| [`is_symlink`](#is_symlink) | Return true if the path itself is a symbolic link. |
| [`stat`](#stat) | Return the metadata (size, modification time, mode) of `p`. |
| [`real_path`](#real_path) | Return the absolute path of `p` with every symbolic link, `.` and `..` resolved against the disk. |
| [`size`](#size) | Return the size of the file in bytes. Panics if the file does not exist. |
| [`remove`](#remove) | Permanently delete a file from the disk. Returns true on success. |
| [`rename`](#rename) | Rename or move the file/directory `src` to `dst`. |
| [`mkdir`](#mkdir) | Create a directory, including any missing parent directories (mkdir -p). An existing directory is treated as success. Return true on success, false on failure. |
| [`rmdir`](#rmdir) | Remove an empty directory. Returns false if the directory is not empty. |
| [`remove_all`](#remove_all) | Remove `p` and, if it is a directory, everything below it (rm -r). |
| [`copy`](#copy) | Copy the content and the permission bits of the file `src` to `dst`, creating or truncating `dst` (like `cp src dst`). |
| [`walk`](#walk) | Walk the directory tree below `top`, yielding `(dir, dirs, files)` for `top` and every directory below it. |
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
    pub func parts() list[str]
    pub func with_name(new_name str) Path
    pub func with_ext(new_ext str) Path
    pub func relative_to(base Path | str) Path?
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

<a id="path.parts"></a>

**`parts() list[str]`**

Return the components of the path, without empty and `.` components.

An absolute path starts with its root (`"/"`, or a drive like `"C:/"`).
`..` is kept: this is lexical, use `normalize` first to fold it.

Example:
    fs.Path("/usr//lib/./x.so").parts()   // ["/", "usr", "lib", "x.so"]
    fs.Path("a/../b").parts()             // ["a", "..", "b"]

<a id="path.with_name"></a>

**`with_name(new_name str) Path`**

Return a new path with the final component replaced by `new_name`.

Panic if the path has no final component (empty, `"."`, `".."` or a
root), or if `new_name` is empty or contains a separator.

Example:
    fs.Path("src/main.kl").with_name("util.kl")   // Path("src/util.kl")

<a id="path.with_ext"></a>

**`with_ext(new_ext str) Path`**

Return a new path with the extension replaced by `new_ext`, given
without the leading dot (as `ext` returns it); an empty `new_ext`
removes the extension.

Panic as `with_name` does, or if `new_ext` starts with a dot.

Example:
    fs.Path("src/main.kl").with_ext("klc")   // Path("src/main.klc")
    fs.Path("a.tar.gz").with_ext("")         // Path("a.tar")

<a id="path.relative_to"></a>

**`relative_to(base Path | str) Path?`**

Return this path relative to `base`, or nil if it is not below `base`.

Purely lexical: both paths are normalized first, symbolic links are
not resolved, and an absolute path is never below a relative one (or
the other way round). The same path gives `Path(".")`.

Example:
    fs.Path("/a/b/c.kl").relative_to("/a")   // Path("b/c.kl")
    fs.Path("/a/b").relative_to("/x")        // nil

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

<a id="stat"></a>

### `Stat`

```kl
pub class Stat {
    pub let size int
    pub let mtime float64
    pub let mtime_ns int
    pub let mode int
    pub func __init__(sz int, mt float64, mt_ns int, md int)
    pub func is_file() bool
    pub func is_dir() bool
    pub func is_symlink() bool
}
```

Metadata of a file, directory or symbolic link, as returned by `fs.stat`.

A snapshot taken when `fs.stat` was called; it does not change when the
file changes afterwards.

See: `fs.stat`.

---

**Fields**

<a id="stat.size"></a>

**`pub let size int`**

The size in bytes (for a symbolic link from `fs.stat(p, false)`, the
length of the target path).

<a id="stat.mtime"></a>

**`pub let mtime float64`**

The last modification time, in seconds since the Unix epoch.

<a id="stat.mtime_ns"></a>

**`pub let mtime_ns int`**

The last modification time, in nanoseconds since the Unix epoch.

Use it rather than `mtime` to compare times exactly (e.g. "is the
output older than its source").

<a id="stat.mode"></a>

**`pub let mode int`**

The raw `st_mode`: the file type bits and the permission bits
(`mode & 0o777`).

<a id="stat.__init__"></a>

**`__init__(sz int, mt float64, mt_ns int, md int)`**

Create a `Stat`; `fs.stat` is the normal way to get one.

<a id="stat.is_file"></a>

**`is_file() bool`**

Return true if this is a regular file.

<a id="stat.is_dir"></a>

**`is_dir() bool`**

Return true if this is a directory.

<a id="stat.is_symlink"></a>

**`is_symlink() bool`**

Return true if this is a symbolic link.

Only a `Stat` from `fs.stat(p, false)` can be a link: by default
`fs.stat` follows links and describes the target.

<a id="walker"></a>

### `Walker`

```kl
pub class Walker : Iterator[(Path, list[Path], list[Path])] {
    let stack list[Path]
    var pending list[Path]? = nil
    var ready (Path, list[Path], list[Path])? = nil
    pub func __init__(top Path)
    pub func has_next() bool
    pub func next_strict() (Path, list[Path], list[Path])
    pub func next() (Path, list[Path], list[Path])?
}
```
**Bases:** `Iterator[(Path, list[Path], list[Path])]`


An iterator over a directory tree, returned by `fs.walk`.

Each step yields `(dir, dirs, files)` for one directory: `dir` is the
directory itself, `dirs` its subdirectories and `files` every other entry,
all as full paths (as `list_dir` returns them), sorted by name.

Directories are visited top-down and depth-first: a directory comes before
its subdirectories, which follow in the order of `dirs`. The subdirectories
are read only when the next step is taken, so removing entries from `dirs`
before that skips them (pruning).

Symbolic links are never followed: a link, also one to a directory, is
listed in `files`. A directory that cannot be read (permission denied, or
removed meanwhile) is skipped without an error.

See: `fs.walk`.

---

**Fields**

<a id="walker.stack"></a>

**`let stack list[Path]`**

Directories still to visit; the last one is visited next.

<a id="walker.pending"></a>

**`var pending list[Path]? = nil`**

The `dirs` list of the step returned last, pushed on the next step.

<a id="walker.ready"></a>

**`var ready (Path, list[Path], list[Path])? = nil`**

The step found by `has_next` and not yet returned.

<a id="walker.__init__"></a>

**`__init__(top Path)`**

Create a `Walker` starting at `top`; `fs.walk` is the normal way to get one.

<a id="walker.has_next"></a>

**`has_next() bool`**

Return true if another directory is available, reading it if necessary.

<a id="walker.next_strict"></a>

**`next_strict() (Path, list[Path], list[Path])`**

Return the next `(dir, dirs, files)` and advance. Panic if exhausted.

<a id="walker.next"></a>

**`next() (Path, list[Path], list[Path])?`**

Return the next `(dir, dirs, files)`, or nil when the whole tree is visited.

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
    if let f = fs.open("hello.txt", "w") {
        f.write("hello".to_bytes())
        f.close()
    }

See: `File`.

<a id="_path_str"></a>

### `_path_str`

```kl
func _path_str(p Path | str) str
```

Convert a `Path | str` argument to the raw `str` path.

Used by the `.kl` functions (`walk`, `read_all`); every `@native` function
reads a `Path | str` argument directly in C.

<a id="_try_list_dir"></a>

### `_try_list_dir`

```kl
func _try_list_dir(p Path | str) list[Path]?
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

<a id="is_symlink"></a>

### `is_symlink`

```kl
pub func is_symlink(p Path | str) bool
```

*`@native`*

Return true if the path itself is a symbolic link.

The link is not followed, so a dangling link (whose target is missing)
is still a link, while `exists` and `is_file` follow it and return false.

<a id="stat"></a>

### `stat`

```kl
pub func stat(p Path | str, follow_links = true) Stat?
```

*`@native`*

Return the metadata (size, modification time, mode) of `p`.

By default symbolic links are followed and the target is described; pass
`follow_links = false` to describe the link itself (lstat), which is the
only way to get a `Stat` whose `is_symlink()` is true.

Return nil if `p` cannot be stat'ed (missing path, permission denied);
`os.errno()` / `os.strerror()` tell why.

Example:
    if let st = fs.stat("main.kl") {
        print(st.size, st.mtime_ns, st.is_file())
    }

See: `Stat`, `size`, `is_symlink`.

<a id="real_path"></a>

### `real_path`

```kl
pub func real_path(p Path | str) Path?
```

*`@native`*

Return the absolute path of `p` with every symbolic link, `.` and `..`
resolved against the disk.

Unlike `Path.absolute` and `Path.normalize`, which only rewrite the
string, this follows links, so `a/link/..` ends up in the link target's
parent. Return nil if `p` (or a directory on the way) does not exist or
cannot be read; `os.errno()` tells why.

Example:
    let here = fs.real_path(".")!   // the working directory, links resolved

See: `Path.absolute`, `Path.normalize`.

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

<a id="remove_all"></a>

### `remove_all`

```kl
pub func remove_all(p Path | str) bool
```

Remove `p` and, if it is a directory, everything below it (rm -r).

Symbolic links are never followed: a link, also one to a directory, is
removed itself and its target is left alone.

Stop at the first entry that cannot be removed and return false; the
entries removed before it stay removed. Return false if `p` does not
exist; `os.errno()` tells why. Return true when `p` is gone.

See: `remove`, `rmdir`.

<a id="copy"></a>

### `copy`

```kl
pub func copy(src Path | str, dst Path | str) bool
```

*`@native`*

Copy the content and the permission bits of the file `src` to `dst`,
creating or truncating `dst` (like `cp src dst`).

A symbolic link `src` is followed and its target is copied. `dst` is the
new file's path, not a directory to copy into.

Return true on success, false on failure: `src` is missing, unreadable or
a directory, `dst` cannot be written, or both name the same file;
`os.errno()` tells why. On a failure while writing, `dst` may be left
partly written.

See: `rename`, `walk`.

<a id="walk"></a>

### `walk`

```kl
pub func walk(top Path | str) Walker
```

Walk the directory tree below `top`, yielding `(dir, dirs, files)` for
`top` and every directory below it.

See `Walker` for the order, pruning and symbolic links. A `top` that is
not a readable directory yields nothing.

Example:
    for (dir, dirs, files) in fs.walk("src") {
        for f in files {
            if f.ext() == "kl" { print(f) }
        }
    }

See: `Walker`, `list_dir`.

<a id="list_dir"></a>

### `list_dir`

```kl
pub func list_dir(p Path | str) list[Path]
```

*`@native`*

Read the directory contents and return a list of strongly-typed `path.Path` objects.

Each entry is joined with the directory path as `dir + "/" + name`
(e.g. `list_dir("/tmp")` yields `Path("/tmp/a.txt")`; no extra `/` when
`p` ends with one, and `p` is not normalized), so the returned paths can
be used directly; the base name is still available via `Path.name`.
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
