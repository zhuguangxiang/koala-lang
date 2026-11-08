# Koala shell-lexer API: `koala/shlex`

A port of CPython's `shlex`: splitting a line the way a POSIX shell splits a
command line, quoting a string for a shell, and the `shlex` lexer class behind
both. Every name, parameter order, default and result is Python's; the lexer is
a line-for-line port of CPython 3.14's `read_token`, so it gives the same
tokens for the same input and settings, its `debug` trace included.

| Package | Depends on | Native code |
|---------|-----------|-------------|
| `koala/shlex` | `std/io`, `std/fs`, `std/sys` | none |

A typical use is the step before `koala/cli` in an interactive loop: the
shell splits a program's command line, but a line read from the user has to be
split by the program.

```kl
let words = shlex.split("add \"buy milk\" -p 1")!   // ["add", "buy milk", "-p", "1"]
let args = parser.try_parse_args(words)
```

## Contents

| Name | Summary |
|------|---------|
| [`split`](#shlex-split) | `split(s, comments=false, posix=true)` -> `list[str]?`: split like a shell. |
| [`join`](#shlex-join) | `join(split_command)` -> `str`: the inverse of `split`. |
| [`quote`](#shlex-quote) | `quote(s)` -> `str`: `s` as one shell word. |
| [`shlex`](#shlex-shlex) | The lexer: `get_token`, `push_token`, `read_token`, `push_source`, `sourcehook`, iteration, and its settings. |

## Differences from Python

| Python | Koala | Why |
|--------|-------|-----|
| `split` raises `ValueError("No closing quotation")` / `("No escaped character")` | `split` returns nil | Koala has no exceptions, and a bad line typed by a user is not a bug to panic on. |
| the lexer raises the same `ValueError` | `get_token()` / `read_token()` return nil and `lex.error()` is the message; every later call returns nil | Same reason; the lexer keeps the message. |
| `lex.posix`, `lex.infile`, `lex.instream`, `lex.punctuation_chars` | `lex.posix()`, `infile()`, `instream()`, `punctuation_chars()` | They are also the constructor's parameter names, and Koala does not let a parameter share a field's name. The other attributes are fields with Python's names. |
| `error_leader(infile=None, lineno=None)` | `error_leader(infile=nil, line=nil)` | `lineno` is a field. |
| `instream` is any file-like object | a `str`, `io.StringIO`, `fs.File` or `io.BufReader` | Koala's streams are distinct classes, not one protocol. |
| `instream` is the object given | `instream()` is an `io.Reader`: the given stream, or the `io.StringIO` made for a `str` | A union cannot be a return type. |
| subclass and override `sourcehook` | not possible; the default is built in | Koala classes are final. |
| `sourcehook` returns `(name, file)` | `(str, fs.File)?`, nil with `error()` set when the file cannot be opened | The default always opens a file. |
| `pushback` and `filestack` are deques | `pushback` is a `list[str]`, the next token first; `filestack` is private | |
| `for token in lex` stops at `eof` | the same, and also at a lexing error | |

## Reference

<a id="shlex-split"></a>
### `split(s, comments = false, posix = true) -> list[str]?`

Split `s` as a POSIX shell does: on whitespace, keeping quoted text together
and removing the quotes, with `\` escaping the next character outside single
quotes. Returns nil on an unclosed quote or a trailing `\`.

- `comments` - treat `#` as the start of a comment running to the end of
  the line.
- `posix=false` - the old non-POSIX rules: quotes stay in the words and `\`
  does not escape.

| Input | Result |
|-------|--------|
| `a  b` | `["a", "b"]` |
| `add "buy milk" -p 1` | `["add", "buy milk", "-p", "1"]` |
| `'a "b"'` | `["a \"b\""]` |
| `a\ b` | `["a b"]` |
| `"say \"hi\""` | `["say \"hi\""]` |
| `x"y z"w` | `["xy zw"]` |
| `'it'\''s'` | `["it's"]` |
| `a #b c` | `["a", "#b", "c"]`; with `comments=true`, `["a"]` |
| `"abc` | nil |

<a id="shlex-join"></a>
### `join(split_command) -> str`

Each argument through `quote()`, joined with spaces. `split(join(args))` gives
`args` back.

```kl
shlex.join(["echo", "a b", "it's"])   // echo 'a b' 'it'"'"'s'
```

<a id="shlex-quote"></a>
### `quote(s) -> str`

`s` as it is when it holds only the safe characters
`%+,-./0123456789:=@`, letters, digits and `_`; `''` when it is empty; else
in single quotes, a single quote written as `'"'"'`.

```kl
shlex.quote("file.txt")   // file.txt
shlex.quote("a b")        // 'a b'
shlex.quote("$HOME")      // '$HOME'
shlex.quote("")           // ''
```

<a id="shlex-shlex"></a>
### `shlex(instream = nil, infile = nil, posix = false, punctuation_chars = false)`

The lexer. `instream` is the input - a `str`, an `io.StringIO`, an `fs.File`
or an `io.BufReader` - and `sys.stdin` without one. `infile` is its name, for
`error_leader()` and for relative `source` files. `posix` turns on the POSIX
rules. `punctuation_chars` is `true` for `();<>|&`, or a `str` of the
characters to return as tokens of their own, so that `a&&b;c` gives `a`,
`&&`, `b`, `;`, `c`; it also adds `~-./*?=` to `wordchars`.

The default, non-POSIX lexer is not a shell splitter: a character outside
`wordchars` is a token of its own (`a+b` gives `a`, `+`, `b`) and quotes stay
in the token. `split` uses POSIX mode with `whitespace_split = true`.

| Member | Python | Notes |
|--------|--------|-------|
| `get_token() -> str?` | `get_token` | The next token: a pushed-back one, or one read from the input. Follows `source` inclusions. `eof` at the end; nil on an error. |
| `push_token(tok)` | `push_token` | The next `get_token` returns `tok`. |
| `read_token() -> str?` | `read_token` | A raw token, ignoring the pushed-back tokens and `source`. |
| `push_source(newstream, newfile = nil)` | `push_source` | Read `newstream` first; at its end the lexer returns to the current input. |
| `pop_source()` | `pop_source` | Close the current input and go back to the previous one. |
| `sourcehook(newfile) -> (str, fs.File)?` | `sourcehook` | The file to read for `source <newfile>`: double quotes removed, relative to the directory of `infile()`, opened. |
| `error_leader(infile = nil, line = nil) -> str` | `error_leader` | `"conf.txt", line 3: `, the current file and line by default; `"None"` without a file name. |
| `for token in lex` | iteration | The tokens up to `eof`. |
| `error() -> str` | the `ValueError` message | `"No closing quotation"`, `"No escaped character"`, `"[Errno 2] No such file or directory: 'x'"`, or `""`. |
| `posix() -> bool`, `infile() -> str?`, `instream() -> io.Reader`, `punctuation_chars() -> str` | attributes | See the differences. |

The settings and the state are fields, as in Python:

| Field | Default | Meaning |
|-------|---------|---------|
| `commenters` | `"#"` | Characters that start a comment to the end of the line. |
| `wordchars` | letters, digits, `_`; Latin-1 letters in POSIX mode | Characters of a word. |
| `whitespace` | `" \t\r\n"` | Characters between tokens. |
| `whitespace_split` | `false` | Split on whitespace only: everything else is part of a word. |
| `quotes` | `"'\""` | Quote characters. |
| `escape` | `"\\"` | Escape characters (POSIX mode). |
| `escapedquotes` | `"\""` | Quotes inside which `escape` works (POSIX mode). |
| `source` | nil | A word, such as `"source"`, that includes the file named after it. |
| `debug` | `0` | 1 to 3 print a trace, as Python's. |
| `lineno` | `1` | The current line. |
| `eof` | nil in POSIX mode, `""` otherwise | What `get_token` returns at the end. |
| `token`, `state`, `pushback` | | The lexer's working state. |

```kl
// A configuration file: `key value` lines, `#` comments, `include <file>`.
let lex = shlex.shlex(fs.open("app.conf")!, "app.conf", posix=true)
lex.whitespace_split = true
lex.source = "include"
while let key = lex.get_token() {
    let value = lex.get_token()
    if value == nil {
        print(lex.error_leader() + "missing value for " + key)
        break
    }
    print(key, "=", value)
}
if lex.error().len() > 0 { print(lex.error_leader() + lex.error()) }
```

## Verification

`libs/koala/shlex/test/` (132 tests, run with `wombat test shlex`) is
generated by `test/gen_tests.py` from CPython 3.14.4's own `shlex`: every
expected value is what Python returns for the same input. `test_split.kl`
covers `split` in POSIX and non-POSIX mode, with and without comments, quotes,
escapes, Unicode and the errors; `test_quote.kl` covers `quote`, `join` and the
round trip; `test_lexer.kl` covers the lexer's modes, `whitespace_split`,
`punctuation_chars`, custom settings, line counting, every default attribute,
`push_token`, `push_source`, `source` inclusion of real files, `error_leader`,
the stream inputs and the errors. The `debug` trace was compared with
CPython's for the same input: it differs only where Python prints an object's
address.
