# Koala `std/prelude` API Reference

## Contents

**Functions**

| Name | Summary |
|------|---------|
| [`open`](#open) | Opens the file at path and returns a File object. |
| [`print_intern`](#print_intern) | Write objects to `w` with the specified separator and ending. |
| [`print`](#print) | Print objects separated by `sep` and followed by `end`. |
| [`fprintf`](#fprintf) | Format `args` according to `fmt` and write the result to `w`. |
| [`printf`](#printf) | Format `args` according to `fmt` and print the result to stdout. |

## Functions

<a id="open"></a>

### `open`

```kl
pub func open(path str, mode = "r") fs.File?
```

Opens the file at path and returns a File object.

mode selects the access:
- "r": read-only, the file must exist.
- "w": write-only, create or truncate.
- "x": write-only, exclusive create, fail if the file exists.
- "a": write-only, create if absent, always append at the end.
- "r+": read and write, the file must exist.
- "w+": read and write, create or truncate.
- "x+": read and write, exclusive create, fail if the file exists.
- "a+": read and write, create if absent, always append at the end.

Returns nil if the file cannot be opened (missing file, permission denied,
or file already exists for "x"/"x+").
Files created by "w", "x", "a" and their "+" variants get
permission 0644.

Example:
let f = open("hello.txt", "w")
f?.write("hello".to_bytes())
f?.close()

See: File.

<a id="print_intern"></a>

### `print_intern`

```kl
func print_intern(w io.Writer, sep str, end str, objs ...)
```

*`@intrinsic`*

Write objects to `w` with the specified separator and ending.

This function is lowered by the compiler to the `OP_PRINT` instruction.
Each object is formatted through its `Printable` implementation and
written to the specified writer.

<a id="print"></a>

### `print`

```kl
pub func print(objs ..., sep = " ", end = "\n")
```

Print objects separated by `sep` and followed by `end`.

Each object is formatted through its `Printable` implementation.
If no objects are given, `print()` writes only `end`.
The default separator is a space, and the default ending is a newline.
Output is written to `sys.stdout`.

Examples

```kl
print("red", "green", "blue", sep = ", ")
```

<a id="fprintf"></a>

### `fprintf`

```kl
pub func fprintf(w io.Writer, fmt str, args ...) int
```

Format `args` according to `fmt` and write the result to `w`.

Returns the number of bytes written.

<a id="printf"></a>

### `printf`

```kl
pub func printf(fmt str, args ...)
```

Format `args` according to `fmt` and print the result to stdout.

Examples

```kl
printf("Hello, {}!", "world")
printf("x = {}, y = {}", 10, 20)
```
