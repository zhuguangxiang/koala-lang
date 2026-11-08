# Koala command-line API: `koala/cli`

A port of CPython's `argparse`. The names, the parameter order and the defaults
are Python's wherever the language allows it, and so are the help layout, the
usage line, the error messages and the exit statuses, so a program's `-h`
output and its errors read the same as a Python program's.

| Package | Depends on | Native code |
|---------|-----------|-------------|
| `koala/cli` | `std/sys`, `std/os` | none |

`tools/wombat` is built on it.

## Contents

| Name | Summary |
|------|---------|
| [`ArgumentParser`](#cli-argumentparser) | `ArgumentParser(prog, description, epilog, add_help, formatter_class)`: declare arguments, parse, print help. |
| [`add_argument`](#cli-add_argument) | Declare a positional or an option. |
| [`add_subparsers`](#cli-subparsers) | Declare subcommands -> `SubParsers`. |
| [`SubParsers`](#cli-subparsers) | `add_parser(name, ...)` -> the command's `ArgumentParser`. |
| [`parse_args`](#cli-parse_args) | Parse `sys.args()`, or a list; print and exit on `-h` or an error. |
| [`try_parse_args`](#cli-parse_args) | Parse without exiting; nil instead. |
| [`Namespace`](#cli-namespace) | The parsed values, read with `get(dest)`. |
| `REMAINDER` | `nargs=cli.REMAINDER`: the rest of the command line, as it is. |
| `RawDescriptionHelpFormatter` | `formatter_class=`: keep the description and the epilog as written. |

## Differences from Python

| Python | Koala | Why |
|--------|-------|-----|
| `args.output` | `args.get("output")`, an `any?`: `(args.get("output")! as str)!` | Koala has no dynamic attributes and is statically typed; the value's type is known only after the parse. |
| `type=int` | `type_="int"`; `"str"`, `"int"` and `"float"` | `type` is a keyword, and a type is not a value. |
| `action="store_const"`, `const=` | not provided | `const` is a keyword. |
| `nargs` on an option, `nargs=N` | `nargs` only on a positional: `"?"`, `"*"`, `"+"`, `REMAINDER` | Not implemented yet. |
| `add_argument("-o", "--output", "--out")` | at most two option strings: `name` and `flag` | One parameter per string, no varargs of names. |
| `add_argument_group`, `add_mutually_exclusive_group`, `parents=`, `fromfile_prefix_chars`, `exit_on_error=` | not provided | Not implemented yet; `try_parse_args` covers `exit_on_error=False`. |
| `parse_known_args`, `set_defaults`, `type=` a function | not provided | No function values; the rest not implemented yet. |
| a long usage line is wrapped | not wrapped | Not implemented yet. |
| `prog` defaults to `sys.argv[0]` | defaults to `"prog"` | Koala's `sys.args()` holds the arguments only; pass `prog=`. |
| `ArgumentParser(exit_on_error=False)` raises `ArgumentError` | `try_parse_args` returns nil; `exit_status()` / `exit_message()` | Koala has no exceptions. |
| a bad declaration raises `ValueError` / `ArgumentError` | panics, `cli: ...` | A bad declaration is a bug in the program. |

## Reference

<a id="cli-argumentparser"></a>
### `ArgumentParser(prog = "prog", description = "", epilog = "", add_help = true, formatter_class = "")`

- `prog` - the program name in the usage, the help and the errors.
- `description` / `epilog` - text before and after the argument list. They
  are filled as paragraphs to the terminal width (`COLUMNS`, default 80),
  unless `formatter_class=cli.RawDescriptionHelpFormatter`, which keeps them
  as written.
- `add_help` - add `-h, --help`.

`%(prog)s` in a description, an epilog or a help text is the program name;
`%(default)s` in a help text is the argument's default.

| Member | Python | Notes |
|--------|--------|-------|
| `add_argument(...)` | `add_argument` | See below. |
| `add_subparsers(dest, required, help, metavar) -> SubParsers` | `add_subparsers` | Once per parser. |
| `parse_args(args = nil) -> Namespace` | `parse_args` | `args` defaults to `sys.args()`. |
| `try_parse_args(args = nil) -> Namespace?` | - | `parse_args` without exiting. |
| `exit_status() -> int`, `exit_message() -> str` | - | Why the last `try_parse_args` returned nil. |
| `format_usage() -> str`, `format_help() -> str` | same | Ending with a newline. |
| `print_usage()`, `print_help()` | same | To stdout. |
| `error(message)` | `error` | Prints the usage and `prog: error: message` to stderr, exits 2. |
| `exit(status = 0, message = "")` | `exit` | Prints `message` to stderr, exits with `status`. |
| `prog() -> str` | `prog` | |

<a id="cli-add_argument"></a>
### `add_argument(name, flag = "", action = "store", nargs = "", default = nil, type_ = "str", choices = nil, required = false, help = "", metavar = "", dest = "", version = "")`

`name` is a positional (`"dir"`) or an option string (`"-o"`, `"--output"`);
`flag` is a second option string, so the call reads as Python's:

```kl
parser.add_argument("dir", nargs="?", default=".")
parser.add_argument("-o", "--output", default="./dist", help="where to write (default: %(default)s)")
parser.add_argument("-j", "--jobs", type_="int", default=4)
parser.add_argument("-v", "--verbose", action="count", default=0)
```

| Parameter | Values | The value in the Namespace |
|-----------|--------|----------------------------|
| `action` | `"store"` | the last value given, converted by `type_`; else `default` |
| | `"store_true"` / `"store_false"` | a `bool`; `false` / `true` when absent |
| | `"count"` | an `int`, `default` plus the times given; `default` (nil) when absent |
| | `"append"` | a list of the values given; `default` (nil) when absent |
| | `"version"` | prints `version` and exits 0 |
| `nargs` (positionals only) | `""` | one value |
| | `"?"` | one value or none: `default` |
| | `"*"` | a list, maybe empty |
| | `"+"` | a list of at least one |
| | `cli.REMAINDER` | a list of everything that is left, options included |
| `type_` | `"str"`, `"int"`, `"float"` | the value's type; a list is `list[str]` / `list[int]` / `list[float]` |
| `default` | any value | used when the argument is absent; a `str` default is converted by `type_`, as in Python |
| `choices` | `list[str]` | the allowed texts |
| `required` | options only | |
| `dest` | | the Namespace name; by default the long option without `--`, `-` -> `_`, or the positional's name |
| `metavar` | | the value's name in the usage and the help |

A positional that takes many (`"*"`, `"+"`, `REMAINDER`) must be the last one.

The panics on a declaration: an unknown `action`, `type_` or `nargs`; `nargs`
on an option; an action other than `store` on a positional; `required` on a
positional; a positional after the subparsers or after one that takes many; an
option string that does not start with `-`; a conflicting option string; a
second `add_subparsers`.

<a id="cli-subparsers"></a>
### `add_subparsers(dest = "", required = false, help = "", metavar = "") -> SubParsers` / `SubParsers`

The chosen command's name goes to `dest`, nil when no command was given;
without `dest` it is not stored. The command's own arguments go into the same
Namespace, as in Python; a command not chosen sets none of its values.

| Member | Python | Notes |
|--------|--------|-------|
| `add_parser(name, help = "", description = "", epilog = "", add_help = true, formatter_class = "") -> ArgumentParser` | `add_parser` | Its `prog` is the parent's followed by `name`. `help` is its line in the parent's help. |
| `find(name) -> ArgumentParser?` | - | The parser of a command, e.g. for `tool help <command>`. |
| `names() -> list[str]`, `helps() -> list[str]` | - | In the order they were added. |

```kl
let parser = cli.ArgumentParser(prog="tool")
let sub = parser.add_subparsers(dest="command", metavar="<command>")
let build = sub.add_parser("build", help="Compile")
build.add_argument("-o", "--output", default="./dist")
let run = sub.add_parser("run", help="Run a target")
run.add_argument("target")
run.add_argument("args", nargs=cli.REMAINDER)
```

Commands nest: a command's parser may have subparsers of its own.

<a id="cli-parse_args"></a>
### `parse_args(args = nil) -> Namespace` / `try_parse_args(args = nil) -> Namespace?`

The command line is read as Python reads it:

- `--name value`, `--name=value`, and a unique prefix of a long option
  (`--out` for `--output`); an ambiguous prefix is an error.
- `-o value`, `-ovalue`, and flags clustered as `-vvq`.
- `--` ends the options: what follows is positional.
- A negative number such as `-5` is a value, unless the parser has an option
  that looks like a negative number.
- After a command name, the parent's options are no longer recognized.

`parse_args` prints the help (`-h`) or the version (`--version`) to stdout and
exits 0. On an error it prints the usage and the error to stderr and exits 2:

```
usage: tool [-h] <command> ...
tool: error: argument <command>: invalid choice: 'bild' (choose from 'build', 'run')
```

The messages are Python's: `the following arguments are required: target`,
`unrecognized arguments: --x`, `argument -j/--jobs: invalid int value: 'x'`,
`argument -o/--output: expected one argument`, `ambiguous option: --ver could
match --verbose, --version`, ...

`try_parse_args` returns nil where `parse_args` would exit, and keeps what it
would have printed: `exit_status()` is 0 after `-h` / `--version` and 2 after an
error, and `exit_message()` is the text. Use it in tests, or to handle an error
yourself.

```kl
if parser.try_parse_args(["--jobs", "x"]) == nil {
    print(parser.exit_status())    // 2
    print(parser.exit_message())   // usage: ... error: argument -j/--jobs: invalid int value: 'x'
}
```

<a id="cli-namespace"></a>
### `Namespace`

| Member | Python | Notes |
|--------|--------|-------|
| `get(name) -> any?` | `args.name` | Nil for None. An unknown name panics, as Python's `AttributeError`, so a misspelled name fails at once. |
| `contains(name) -> bool` | `hasattr` | None included. |
| `set(name, value)` | `setattr` | |
| `names() -> list[str]` | `vars(args)` | In the order they were set. |

A value is an `any?`; unwrap it, then cast it to the type `add_argument`
gives it:

```kl
let out = (args.get("output")! as str)!            // has a default: never nil
let jobs = (args.get("jobs")! as int)!
let verbose = (args.get("verbose")! as bool)!
if let name = args.get("name") {                    // nargs="?" without a default
    print((name as str)!)
}
let rest = (args.get("args")! as list[str])!
```

## Help layout

`format_help()` follows Python's `HelpFormatter`: the usage line, the
description, `positional arguments:`, `options:`, the epilog. The help column
is after the widest entry plus two, at most 24; an entry wider than that puts
its help on the next line. Text is wrapped to `COLUMNS - 2` (78 by default).
Options come first in the usage, then the positionals, as in Python.

```
usage: tool [-h] [-v] [-o OUTPUT] [dir]

Build things.

positional arguments:
  dir                   what to build

options:
  -h, --help            show this help message and exit
  -v, --verbose         print more
  -o OUTPUT, --output OUTPUT
                        where to write (default: ./dist)
```

An option that takes a value lists the metavar after each of its strings
(`-o OUTPUT, --output OUTPUT`), as Python up to 3.12 does; Python 3.13 writes
`-o, --output OUTPUT`.

## Example

```kl
import "koala/cli"

func main() {
    let parser = cli.ArgumentParser(prog="tool", description="Build things.")
    parser.add_argument("-v", "--verbose", action="store_true", help="print more")
    parser.add_argument("-o", "--output", default="./dist",
                        help="where to write (default: %(default)s)")
    parser.add_argument("dir", nargs="?", default=".", help="what to build")

    let args = parser.parse_args()
    let dir = (args.get("dir")! as str)!
    let out = (args.get("output")! as str)!
    if (args.get("verbose")! as bool)! {
        print("building", dir, "into", out)
    }
}
```

## Verification

`libs/koala/cli/test/` (31 tests, run with `wombat test cli`):
`test_argparse.kl` (the actions, `nargs`, `type_`, `choices`, defaults and
their conversion, `dest` derivation, prefix matching and ambiguity, clusters,
negative numbers, `--`, every error message, the help and usage text against
Python's output, `%(prog)s` / `%(default)s`, `RawDescriptionHelpFormatter`) and
`test_subparsers.kl` (the shared Namespace, a required command, a command
without `dest`, nested commands, a command's help and errors). `tools/wombat`
uses every part of it.
