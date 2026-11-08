# Koala archive API: `koala/tarfile`

A 1:1 port of CPython 3.14's `tarfile`. Every name, parameter order, default
and return type is Python's, so a Python file ports by changing the import
line.

The headers this module writes are **byte for byte** what CPython writes for
the same `TarInfo`, for all three formats - `cmp` of a Python archive and a
Koala archive holding the same members reports no difference. GNU `tar` and
CPython both read what it writes, and it reads what both of them write.

| Package | Depends on | Native code |
|---------|-----------|-------------|
| `koala/tarfile` | `koala/gzip` (-> `koala/zlib`) | none |

`koala/gzip` supplies the `:gz` half of the mode string; `bzip2`, `xz` and
`lzma` are not provided, because Koala has no such codec.

## Contents

**module**

| Name | Summary |
|------|---------|
| [`open`](#tarfile-open) | `open(name, mode="r", ...)` -> `TarFile`. |
| [`is_tarfile`](#tarfile-is_tarfile) | Is this path, or these bytes, a tar archive? |
| [`TarInfo`](#tarfile-tarinfo) | One member's metadata: `name`, `size`, `typeflag`, ... |
| [`TarFile`](#tarfile-tarfile) | The archive: `getmembers`, `add`, `extractall`, ... |
| [`fully_trusted_filter`](#tarfile-filters) | PEP 706 `"fully_trusted"`: extract as recorded. |
| [`tar_filter`](#tarfile-filters) | PEP 706 `"tar"`: refuse the escapes, keep the rest. |
| [`data_filter`](#tarfile-filters) | PEP 706 `"data"`: the default; also drops special files and ownership. |
| [`apply_filter`](#tarfile-filters) | Run the filter a `filter=` string names. |

**constants** - `BLOCKSIZE` (512), `RECORDSIZE` (10240), `LENGTH_NAME` (100),
`LENGTH_LINK` (100), `LENGTH_PREFIX` (155), `USTAR_FORMAT` (0), `GNU_FORMAT`
(1), `PAX_FORMAT` (2), `DEFAULT_FORMAT` (`PAX_FORMAT`), `ENCODING` (`"utf-8"`),
`NUL`, `GNU_MAGIC`, `POSIX_MAGIC`, the fifteen type flags `REGTYPE`, `AREGTYPE`,
`LNKTYPE`, `SYMTYPE`, `CHRTYPE`, `BLKTYPE`, `DIRTYPE`, `FIFOTYPE`, `CONTTYPE`,
`GNUTYPE_LONGNAME`, `GNUTYPE_LONGLINK`, `GNUTYPE_SPARSE`, `XHDTYPE`, `XGLTYPE`,
`SOLARIS_XHDTYPE`, and the sets `SUPPORTED_TYPES`, `REGULAR_TYPES`,
`GNU_TYPES`, `PAX_FIELDS`, `PAX_NUMBER_FIELDS`.

## Reference

<a id="tarfile-open"></a>
### `open(name = nil, mode = "r", fileobj = nil, fmt = DEFAULT_FORMAT, enc = ENCODING, errs = "surrogateescape", deref = false, ign_zeros = false, paxhdr = nil, dbg = nil, errlevel = nil) -> TarFile`

Open an archive. `mode` is Python's:

| mode | meaning |
|------|---------|
| `"r"`, `"r:*"` | read, finding the compression out from the first bytes |
| `"r:"`, `"r:gz"` | read, plain or gzipped |
| `"a"`, `"a:"` | append after the last member |
| `"w"`, `"w:"`, `"w:gz"` | write a new archive |
| `"x"`, `"x:"`, `"x:gz"` | write a new archive, refusing an existing file |
| `"r|"`, `"r|gz"`, `"w|"`, `"w|gz"` | the stream modes; identical here, since a member is handled whole in memory |

`bz2`, `xz` and `zst` panic with Python's wording ("... compression is not
supported"), and any other compression with `unknown compression`. `fileobj`
must be `nil`: Koala has no file-object protocol.

`errs` (Python's `errors`) and `dbg` (its `debug`) and `errlevel` are accepted
for signature parity; a name is decoded as UTF-8 and a failure is a panic.

<a id="tarfile-is_tarfile"></a>
### `is_tarfile(source: str | bytes) -> bool`

True when the path - or the archive bytes - is a tar archive, gzip or not. A
path that does not exist is `false`, not an error, as in Python. An empty
archive (`512` zero bytes) is an archive: its first block is the
end-of-archive one.

<a id="tarfile-tarinfo"></a>
### `TarInfo`

`TarInfo(name = "")`. The fields are Python's: `name`, `mode` (0644), `uid`,
`gid`, `size`, `mtime`, `chksum`, `typeflag` (`REGTYPE`), `linkname`, `uname`,
`gname`, `devmajor`, `devminor`, `offset`, `offset_data`, `pax_headers`,
`sparse`.

The predicates are Python's too - `isreg()`, `isfile()`, `isdir()`, `issym()`,
`islnk()`, `ischr()`, `isblk()`, `isfifo()`, `issparse()`, `isdev()`,
`is_supported()` - as are `path()`, `linkpath()`, `tobuf(format)`,
`create_ustar_header()`, `create_gnu_header()`, `create_pax_header()`,
`apply_pax_info(headers)` and the static
`TarInfo.create_pax_global_header(headers)`.
`copy()` replaces Python's `replace(**kwargs)`.

<a id="tarfile-tarfile"></a>
### `TarFile`

The fields are Python's: `name`, `members`, `format`, `encoding`, `errors`,
`dereference`, `ignore_zeros`, `errorlevel`, `debug`, `extraction_filter`,
`pax_headers`, `closed`, `offset`.

| method | notes |
|--------|-------|
| `mode()`, `comptype()` | the access and the compression of the mode string |
| `getmembers()`, `getnames()`, `getmember(name)` | read the archive; `getmember` panics on a missing name |
| `next()`, `iter()` | one member at a time; `iter()` answers a `TarFileIter` |
| `gettarinfo(name, arcname, fileobj)` | a `TarInfo?` from disk; `fileobj` must be `nil` |
| `add(name, arcname, recursive = true, filter)` | add a path, recursing into a directory in name order |
| `addfile(tarinfo, data: bytes?)` | add one member and its data |
| `print_list(verbose = true, subset)` | Python's `list()`, renamed |
| `extractfile(member: str \| TarInfo)` | `bytes?`: a link answers what it points at |
| `extractall(path = ".", subset, numeric_owner = false, filter)` | extract every member |
| `extract(member, path = "", set_attrs = true, numeric_owner = false, filter)` | extract one |
| `makedir`, `makefile`, `makeunknown`, `makelink`, `chown`, `chmod`, `utime` | the per-type helpers Python leaves open for overriding |
| `makefifo`, `makedev` | panic: no `mkfifo`/`mknod` in `std/fs` |
| `close()` | write the archive out; idempotent |

<a id="tarfile-filters"></a>
### The PEP 706 filters

`fully_trusted_filter(member, dest_path)`, `tar_filter(member, dest_path)` and
`data_filter(member, dest_path)` all answer a `TarInfo` - a filtered copy -
and panic where Python raises `FilterError`. `apply_filter(name, member,
dest_path)` runs the one that `name` selects.

`data_filter` strips the setuid, setgid and sticky bits (`mode = -1` means
"leave the mode alone", Python's `None`), nulls the ownership (`uid`/`gid`
`-1`, `uname`/`gname` `""`), and refuses `..`, absolute paths, links whose
target is outside the destination, and devices/FIFOs. `tar_filter` does the
name and link checks but keeps everything else.

## Reading

```kl
import "koala/tarfile"

let tf = tarfile.open("a.tar.gz", "r:gz")   // "r" or "r:*" also work
for member in tf.iter() {
    print(member.name + " " + str(member.size))
}
let data = tf.extractfile("a.txt")          // bytes?, file members only
tf.close()                                  // idempotent; Koala has no `with`
```

`getmembers()` reads the whole archive and answers the same list `iter()`
fills as it goes, in archive order. `getmember(name)` panics with Python's
`KeyError` wording when the name is not there.

The three header styles are all understood on the way in: `ustar` (with the
`prefix` field split), GNU (the `L`/`K` long-name records, sparse members) and
pax (the `x` and `g` records, plus Solaris' `X`). An old v7 archive - no magic,
`AREGTYPE` for files - reads as well.

## Writing

```kl
let tf = tarfile.open("out.tar", "w")
let info = tarfile.TarInfo("hello.txt")
info.size = 5
info.mtime = 1600000000
tf.addfile(info, b"hello")
tf.close()
```

`add(path)` takes a file from disk and recurses into a directory - the entries
are added **in name order**, as `sorted(os.listdir(name))` does in Python, so
the archive is reproducible. `gettarinfo()` converts a path into a `TarInfo`
without adding it, and turns a second hard link into an `LNKTYPE` member
pointing at the first, which is what Python's `_inodes` bookkeeping does.

`close()` appends the two end-of-archive blocks, pads to a whole `RECORDSIZE`
and writes the file - gzipped when the mode said `:gz`.

## Extracting

```kl
tf.extractall("/tmp/out")                       // filter="data", as in Python
tf.extractall("/tmp/out", filter="tar")         // keep ownership and modes
tf.extract("a.txt", "/tmp/out", filter="fully_trusted")
```

The default filter is PEP 706's `"data"`: a member whose name climbs out of the
destination, a link pointing out of it, or a device/FIFO is **refused with a
panic** carrying Python's `FilterError` text, and ownership is not restored.
`"tar"` applies the same name check but keeps ownership, modes and special
files. `"fully_trusted"` extracts everything as recorded.

`filter=` takes the *name* of a filter, not a function: Koala has no function
values. The functions themselves are exported, so a caller that needs one
calls it directly - `tarfile.data_filter(member, "/tmp/out")` - and a caller
with its own policy applies it to the result of `member.copy()`.

## Differences from Python

The API is Python's wherever the language allows it. These deltas are
unavoidable, and all of them are deliberate:

| Python | Koala | Why |
|--------|-------|-----|
| raises `TarError`, `ReadError`, `FilterError`, `KeyError`, `ValueError` | panics with the same message text, prefixed `tarfile: ` | Koala has no exceptions. |
| `TarInfo.typeflag` is a 1-character `str` | `bytes` | A v7 regular file is marked with a NUL, which a `str` cannot hold. Compare with `tarfile.REGTYPE`, never with `"0"`. |
| `TarInfo.replace(**kw)`; keyword arguments everywhere | `copy()` plus field assignment; shorter keyword names (`file` for `name`, `nm` for `name` of `TarInfo`, `fmt` for `format`, `enc`, `errs`, `deref`, `ign_zeros`, `paxhdr`, `dbg`, `errlevel`) | Koala has no keyword arguments, and a constructor parameter may not shadow a field. The **fields** keep Python's names. |
| `TarFile.list()`, `TarFile.mode`, `TarFile.comptype` | `print_list()`, `mode()`, `comptype()` | `list` is a Koala keyword; `mode` is also a constructor parameter. |
| `TarFile.addfile(tarinfo, fileobj)` | `addfile(tarinfo, data: bytes?)` | Koala's file types are distinct classes, not one protocol, so a member is handled whole in memory. |
| `TarFile(fileobj=...)`, `gettarinfo(fileobj=...)` | panics | Same reason: there is no file-object protocol to hand over. |
| `list()` prints local time | the time column is **UTC** | Koala has no `time.localtime()`. |
| `filter=` takes a function | a string: `"fully_trusted"`, `"tar"`, `"data"` | No function values; the functions are exported for direct calls. |
| `errorlevel`, `debug` | accepted, unused (a failure is always a panic) | There is no way to continue after a panic. |
| `uname`/`gname` filled in from `pwd`/`grp` | left empty | Koala has no user/group database. |
| `TarFile.makefifo()`, `makedev()` | panic | `os.mkfifo()` and `mknod()` are not in `std/os`. |
| `name="-"`, `fileobj` for stdin/stdout | not provided | There is no standard stream object to close. |
| `bzip2`, `xz`, `lzma` modes | panic with Python's wording | Koala has no such codec. |

Two behaviours are stricter than Python's, in the direction of failing loudly:
`is_tarfile()` panics on a gzip member that does not decompress, where Python
answers `False`; and every extraction error is a panic, since `errorlevel` has
nothing to select.

## Header helpers

The block code is exported, because a caller that writes its own header needs
it: `blocks(count)`, `nti(b, off, len)` (octal -> int, panics on garbage),
`nts(b, off, len)` (string field), `itn(buf, v, len, format)`, `stn(buf, s,
len)`, `is_zero_block(b)`, `calc_chksums(b)`, `posix_split_name(name)`,
`gnu_long_header(name, typ)`, `pax_generic_header(headers, typ)`,
`parse_pax(b)`, `trim_slashes(s)`.

The formatting helpers `print_list()` uses are exported too - `filemode()`,
`perm_string()`, `owner_string()`, `size_string()`, `utc_datetime()` - so a
caller can print a table of its own without copying the `ls -l` layout.

## Verification

`libs/koala/tarfile/test/test_tarfile.kl` (33 tests, run with `koala --test`)
covers the constants, the `TarInfo` defaults, the header round trip, the
checksum, write and read, pax long and unicode names, GNU long names, the
`ustar` prefix split, gzip, append mode, `iter()`, `add()` of a file and of a
directory, `extractall`, `extract()` of one member, every PEP 706 rule, the
refusals (`../`, an absolute path, a symlink out of the tree, a device) and the
panics.

Outside the suite, the same members were archived with CPython 3.14.4 and with
this module: the two files compare equal for `USTAR_FORMAT`, `GNU_FORMAT` and
`PAX_FORMAT`.