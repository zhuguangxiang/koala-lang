# Koala line-editing API: `koala/readline`

A port of CPython's `readline` module: line editing with the emacs keys of GNU
readline, a history that can be saved to a file, Tab completion, and readline
init files (`~/.inputrc`). The module functions have Python's names,
parameters and defaults. Unlike Python, which wraps the GNU C library, the
editor is written in Koala; native code only switches the terminal to raw mode.
Its behavior was checked key by key against GNU readline 8.3.

| Package | Depends on | Native code |
|---------|-----------|-------------|
| `koala/readline` | `std/os`, `std/sys`, `std/fs` | `libkoala_readline.so`: `tty_raw_on` / `tty_raw_off` |

A typical use is an interactive loop. `readline.input` takes the place of
Python's `input()` once `readline` is imported, and `koala/shlex` splits the
line the way a shell would:

```kl
import "koala/readline"
import "koala/shlex"

func main() {
    readline.read_history_file(".myapp_history")
    while let line = readline.input("> ") {
        if let words = shlex.split(line) {
            print(words)
        }
    }
    readline.write_history_file(".myapp_history")
}
```

Two complete examples are in `demo/`:

- `readline_demo.kl` has a colored prompt, a history kept between runs, Tab
  completion of the commands and history numbers, and a startup hook that
  pre-fills the line.
- `sqlite3_demo.kl shell` is a to-do list shell, with `koala/cli` parsing each
  line and a completer for its commands.

## Contents

| Name | Summary |
|------|---------|
| [`input`](#readline-input) | `input(prompt="")` -> `str?`: read one edited line; nil at the end of the input. |
| [History](#readline-history) | `add_history`, `clear_history`, `get_current_history_length`, `get_history_item`, `remove_history_item`, `replace_history_item`, `get_history_length`, `set_history_length`, `set_auto_history`, `read_history_file`, `write_history_file`, `append_history_file`. |
| [Completion](#readline-completion) | `set_completer` / `get_completer`, `get_completion_type`, `get_begidx`, `get_endidx`, `set_completer_delims` / `get_completer_delims`, `set_completion_display_matches_hook`. |
| [Init files](#readline-init) | `parse_and_bind`, `read_init_file`. |
| [The line being edited](#readline-line) | `get_line_buffer`, `insert_text`, `redisplay`, `set_startup_hook`, `set_pre_input_hook`. |
| [`Completer`, `DisplayMatchesHook`, `Hook`](#readline-traits) | The traits that take the place of Python's callback functions. |
| [`Editor`](#readline-editor) | One editor with its own history, bindings and settings; the module functions use a shared one. |
| [`Terminal`, `MemoryTerminal`](#readline-terminal) | Where an editor reads keys and draws; a terminal in memory for tests. |
| [Keys](#readline-keys) | The default key bindings and the command names. |

## Differences from Python

| Python | Koala | Why |
|--------|-------|-----|
| the builtin `input()` edits once `readline` is imported | `readline.input(prompt)` | Koala's builtin `input` is not replaced by importing a package. |
| `input()` raises `EOFError` on Ctrl-D | `input()` returns nil | Koala has no exceptions; `while let line = readline.input(...)` ends the loop. |
| Ctrl-C raises `KeyboardInterrupt` | the line is dropped and `input()` returns `""` | Same reason; the program decides whether to stop. |
| `set_completer(function)` | `set_completer(completer Completer?)`: an object with `complete(text, state) str?` | Koala has no free-standing function values to pass; a trait is the typed equivalent. |
| `set_completion_display_matches_hook(function)` | takes a `DisplayMatchesHook` | Same reason. |
| `set_startup_hook(function)` / `set_pre_input_hook(function)` | take a `Hook` | Same reason. |
| `read_history_file`, `write_history_file`, `append_history_file`, `read_init_file` raise `OSError` | return false | A missing history file is the normal first run, not a bug. |
| `remove_history_item` / `replace_history_item` raise `ValueError` for a bad position | panic | A bad position is a bug in the program. |
| `editing-mode vi` and the vi commands | ignored; only the emacs keymap exists | Not implemented. |
| `readline.backend` (3.13) | not provided | |
| the editor is GNU readline or libedit | the editor is Koala; an `Editor` can be created on any `Terminal` | Several editors, each with its own history, and tests without a terminal. |

## Reference

<a id="readline-input"></a>
### `input(prompt = "") -> str?`

Read a line from the terminal after `prompt` and return it without its newline.
It returns nil at the end of the input: Ctrl-D on an empty line, or no more
input. Ctrl-C drops the line and returns `""`.

- On a terminal, the line is edited with the [keys](#readline-keys) below. A
  line that is not empty and not the same as the previous one goes into the
  history, unless `set_auto_history(false)`.
- On a pipe, a file, or a terminal that cannot move the cursor (`TERM=dumb`,
  `cons25`, `emacs`), it prints the prompt and reads a plain line.
- A paste of several lines accepts the first one. The other lines are the
  lines read next, as if typed.
- In `prompt`, text between `"\x01"` and `"\x02"` takes no columns, as with GNU
  readline's `RL_PROMPT_START_IGNORE` / `RL_PROMPT_END_IGNORE`. ANSI color
  sequences never take any.
- The line is drawn on one row and scrolls sideways when it is wider than the
  screen. A wide character (CJK, emoji) takes two columns, and a control
  character shows as `^X`.

Before the first line read on the terminal, the init file is read, as GNU
readline does: `$INPUTRC`, else `~/.inputrc`.

<a id="readline-history"></a>
### History

| Function | Result |
|----------|--------|
| `add_history(line)` | Append `line` to the history. |
| `clear_history()` | Remove every line. |
| `get_current_history_length() -> int` | The number of lines. |
| `get_history_item(index) -> str?` | Line `index`, counted from 1, as Python; nil when there is none. |
| `remove_history_item(pos)` | Remove line `pos`, counted from 0; panics when there is none. |
| `replace_history_item(pos, line)` | Replace line `pos`, counted from 0; panics when there is none. |
| `get_history_length() -> int` | The most lines a history file keeps; -1 (the default) for all. |
| `set_history_length(length)` | Set it; -1 for all. |
| `set_auto_history(enabled)` | Whether `input` adds the lines it reads; true by default. |
| `read_history_file(filename = nil) -> bool` | Append the lines of `filename`, `~/.history` by default. False when it cannot be read. |
| `write_history_file(filename = nil) -> bool` | Write the history, keeping its last `get_history_length()` lines. False when it cannot be written. |
| `append_history_file(nelements, filename = nil) -> bool` | Append the last `nelements` lines to `filename`, which must exist. |

A history file has one line per entry, as GNU readline writes it without
timestamps. While a line is being edited, a line recalled from the history can
be changed; the change lasts for this edit only, as in GNU readline.

<a id="readline-completion"></a>
### Completion

Tab inserts a tab until it is bound to completion, as in Python:

```kl
readline.parse_and_bind("tab: complete")
readline.set_completer(Commands())        // see Completer below
```

| Function | Result |
|----------|--------|
| `set_completer(completer = nil)` | What Tab completes; nil for file names. |
| `get_completer() -> Completer?` | The completer set. |
| `get_completion_type() -> int` | Why the completer is called: `'\t'` (9) to complete, `'?'` (63) to list the matches. |
| `get_begidx() -> int` / `get_endidx() -> int` | Where the word being completed starts and ends in `get_line_buffer()`, in characters. |
| `set_completer_delims(string)` / `get_completer_delims() -> str` | The characters that end the word. The default is GNU readline's, which includes `-`, so `--name` has to be completed after setting `" \t\n"`. |
| `set_completion_display_matches_hook(hook = nil)` | How the matches are listed; nil for readline's columns. |

The behavior is GNU readline's:

- The word is the text before the cursor, back to a delimiter. The completer is
  called with `state` 0, 1, 2, ... until it returns nil.
- With one match, the word is replaced by it. No space is added after it; as
  in Python, a completer that wants one returns `"word "`.
- With several matches, the word is replaced by their common start and the
  terminal beeps. A second Tab that changes nothing lists them in columns.
  When there are `completion-query-items` (100) or more, it first asks
  `Display all N possibilities? (y or n)`. When the list fills the screen, it
  pauses at `--More--`: Space shows the next screen, Enter the next line, and
  `q` stops.
- Without a completer, Tab completes file names, and a directory gets a `/`.
- `completion-ignore-case`, `show-all-if-ambiguous`, `page-completions`,
  `mark-directories` and `completion-query-items` change this, as in GNU
  readline.

<a id="readline-init"></a>
### Init files: `parse_and_bind(string)`, `read_init_file(filename = nil) -> bool`

`parse_and_bind` runs one line of a readline init file. `read_init_file` reads
a whole file: `filename`, else the file read last, else `$INPUTRC`, else
`~/.inputrc`. It returns false when the file cannot be read.

```
tab: complete                     # a key name bound to a command
"\C-x\C-k": kill-whole-line       # a key sequence bound to a command
"\eh": "hello"                    # a key sequence bound to a macro, typed when pressed
set completion-ignore-case on     # a variable
$if mode=emacs
...
$else
...
$endif
```

- Key names are those of GNU readline: `Control-x`, `C-x`, `Meta-x`, `M-x`,
  `Tab`, `Return`, `Escape`, `Rubout`, `Del`, `Space`, `Newline`, `LFD`. In a key
  sequence, `\C-x` is Control-x, `\M-x` and `\ex` are Escape then x, and `\\`,
  `\"`, `\'`, `\a`, `\b`, `\d`, `\f`, `\n`, `\r`, `\t`, `\v`, `\NNN` (octal) and
  `\xHH` are as in GNU readline.
- A key sequence that is the start of a longer bound sequence waits for the
  next key.
- The variables are `bell-style` (`audible` beeps; `none` and `visible` are silent),
  `completion-ignore-case`, `completion-query-items`, `mark-directories`,
  `page-completions` and `show-all-if-ambiguous`. Other variables, and
  `editing-mode vi`, are accepted and ignored.
- In `$if`, `mode=emacs` holds, `term=NAME` compares with `$TERM`, and an
  application name holds for `koala`. `$include` reads another file.
- C-x C-r (`re-read-init-file`) reads the init file again.

<a id="readline-line"></a>
### The line being edited

| Function | Result |
|----------|--------|
| `get_line_buffer() -> str` | The line being edited. |
| `insert_text(string)` | Insert `string` at the cursor. |
| `redisplay()` | Draw the prompt and the line again. |
| `set_startup_hook(function = nil)` | Called before the prompt of each line is shown. |
| `set_pre_input_hook(function = nil)` | Called after the prompt is shown, before the first key is read. |

A startup hook that calls `insert_text` gives the line a default value:

```kl
class Default : readline.Hook {
    func __init__() {}
    func call() { readline.insert_text("yes") }
}

readline.set_startup_hook(Default())
let answer = readline.input("Continue? ")     // the line starts as "yes"
readline.set_startup_hook()
```

<a id="readline-traits"></a>
### `Completer`, `DisplayMatchesHook`, `Hook`

```kl
pub trait Completer {
    func complete(text str, state int) str?
}

pub trait DisplayMatchesHook {
    func display(substitution str, matches list[str], longest_match_length int)
}

pub trait Hook {
    func call()
}
```

- `Completer.complete(text, state)` returns match number `state` of `text`, or
  nil after the last one. While it runs, `get_line_buffer()`, `get_begidx()`
  and `get_endidx()` say where `text` is in the line.
- `DisplayMatchesHook.display` lists the matches in place of readline. It gets
  the common start, the sorted matches and the length of the longest one. It
  is called with the terminal out of raw mode, so `print` works as usual; the
  prompt and the line are drawn again afterwards.

```kl
class Commands : readline.Completer {
    let words list[str]
    func __init__() { words = ["add", "list", "done"] }
    func complete(text str, state int) str? {
        var n = 0
        for w in words {
            if w.starts_with(text) {
                if n == state { return w + " " }
                n += 1
            }
        }
        return nil
    }
}
```

<a id="readline-editor"></a>
### `Editor(term = nil)`

An editor: the line, the kill ring, the history, the key bindings and the
settings. `term` is the [`Terminal`](#readline-terminal) it uses; nil is the
process's terminal. Every module function calls the same method on one shared
`Editor`, so the methods have the module functions' names and meaning:
`readline(prompt)` is `input(prompt)`, and `add_history`,
`set_completer`, `parse_and_bind`, ... are the same.

An editor created on the real terminal reads the init file before its first
line. An editor on another terminal reads one only when `read_init_file` is
called.

```kl
let editor = readline.Editor()
editor.parse_and_bind("tab: complete")
while let line = editor.readline("> ") {
    print("got", line)
}
```

<a id="readline-terminal"></a>
### `Terminal`, `MemoryTerminal(keys, width = 80, height = 24, interactive = true, suspend = 26, quit = 28)`

`Terminal` is where an editor reads keys and draws:

```kl
pub trait Terminal {
    func read_byte() int            // the next input byte, -1 at the end
    func write(text str)
    func flush()
    func columns() int
    func rows() int
    func set_raw(on bool) bool      // false when the terminal cannot
    func is_interactive() bool      // false: read plain lines, as from a pipe
    func suspend_key() int          // the byte of the suspend key, -1 for none
    func quit_key() int             // the byte of the quit key, -1 for none
    func suspend()                  // stop the program; returns once it is continued
    func quit()                     // quit the program; returns if it does not quit
}
```

The real terminal reads stdin and draws on stdout. Its size comes from stdout
and is 80 x 24 when unknown. It turns on bracketed paste while a line is being
read. Its suspend and quit keys are the terminal's own (`stty susp`, `stty quit`;
none after `stty -isig`), and `suspend()` / `quit()` send SIGTSTP / SIGQUIT to the
process group, as the terminal driver does.

`MemoryTerminal` replays the bytes of `keys` as typed input and keeps what is
written. `output()` is everything written, `clear_output()` forgets it, and
`raw()` says whether raw mode is on. `interactive=false` makes it behave as a
pipe. `suspend` and `quit` are the bytes of its suspend and quit keys, -1 for none.
Its `suspend()` and `quit()` do nothing but are kept in `signals()`: `"suspend"` or
`"quit"`, with `" in raw mode"` when raw mode was on.

```kl
let term = readline.MemoryTerminal("ab\x1b[DX\r", width=40)
let line = readline.Editor(term).readline("> ")    // "aXb"
```

<a id="readline-keys"></a>
### Keys

The default bindings are GNU readline's emacs keymap:

| Keys | Command |
|------|---------|
| Left Right, C-b C-f | `backward-char`, `forward-char` |
| M-b M-f, C-Left C-Right | `backward-word`, `forward-word` |
| Home End, C-a C-e | `beginning-of-line`, `end-of-line` |
| Backspace, C-h | `backward-delete-char` |
| Delete, C-d | `delete-char`; C-d on an empty line ends the input |
| C-k, C-u | `kill-line`, `unix-line-discard` |
| C-w | `unix-word-rubout`: the whitespace-delimited word before the cursor |
| M-d, M-Backspace | `kill-word`, `backward-kill-word` |
| C-y, M-y | `yank`, then `yank-pop` through older kills |
| M-. M-_ | `yank-last-arg`: the last word of the previous history line, older ones on repeat |
| C-t, M-t | `transpose-chars`, `transpose-words` |
| M-u M-l M-c | `upcase-word`, `downcase-word`, `capitalize-word` |
| M-\\ | `delete-horizontal-space` |
| C-_, C-x C-u | `undo` |
| M-r | `revert-line` |
| Up Down, C-p C-n | `previous-history`, `next-history` |
| M-< M-> | `beginning-of-history`, `end-of-history` |
| C-r C-s | `reverse-search-history`, `forward-search-history` |
| Tab | `tab-insert` until bound to `complete` |
| M-? M-=, M-* | `possible-completions`, `insert-completions` |
| C-v C-q | `quoted-insert`: the next key, as it is |
| C-l | `clear-screen` |
| C-g, C-x C-g | `abort` |
| C-x C-r | `re-read-init-file` |
| Enter | `accept-line` |
| C-c | drop the line: `input` returns `""` |
| C-z, C-\ (the terminal's suspend and quit keys) | suspend the program, or quit it, as with signal keys on; the line is kept and edited on afterwards |

Other commands that can be bound are `kill-whole-line`, `backward-kill-line` (the same as
`unix-line-discard`) and `self-insert`.

- Consecutive kills go into one entry of the kill ring. A Meta kill starts a
  new entry, because GNU readline's Escape prefix resets the kill state.
- In an incremental search, the line shows the latest match of what is typed.
  C-r and C-s go to the next older or newer match, Backspace drops a
  character, C-g goes back to the line before the search, and Enter accepts
  the match. Any other key ends the search on the match and then does its
  usual job. A search that fails keeps the last match and beeps, and a
  Backspace after it acts as GNU readline's does.

## Verification

`libs/koala/readline/test/` (193 tests, run with `wombat test readline`) drives
editors on a `MemoryTerminal`:

- `test_editor.kl` (49): reading, drawing, scrolling, wide characters, prompts,
  plain-line input and the suspend and quit keys.
- `test_editing.kl` (39): every editing command and the kill ring.
- `test_history.kl` (37): the history functions, the history files and the
  searches.
- `test_completion.kl` (34): completion, the listing, the query, the pager and
  the hooks.
- `test_bindings.kl` (34): `parse_and_bind`, key sequences, macros, variables,
  `$if`, init files and the startup and pre-input hooks.

Each expected result was taken from GNU readline 8.3, through Python's
`input()` in a pseudo-terminal given the same keys. The suspend and quit keys were
also checked on a real pseudo-terminal, with the program run as a job: it stops
on Ctrl-Z with the terminal back in its normal mode, keeps the line after `fg`,
dies of SIGQUIT on Ctrl-\, follows `stty susp`, and treats the keys as keys after
`stty -isig`, as GNU readline does.
