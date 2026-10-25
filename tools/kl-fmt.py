#!/usr/bin/env python3
"""kl-fmt.py - Conservative formatter for Koala (.kl) source files.

Usage:

    python3 tools/kl-fmt.py path/to/file.kl          # print formatted to stdout
    python3 tools/kl-fmt.py path/to/dir              # report which files change
    python3 tools/kl-fmt.py path --diff              # show unified diffs
    python3 tools/kl-fmt.py path -w                  # rewrite files in place
    python3 tools/kl-fmt.py path --check             # exit 1 if any file changes

What it does (whitespace only, never reorders tokens):
  - recompute indentation (4 spaces per block level, continuation lines
    aligned under the opening `(` / `[` of the statement they belong to)
  - normalize spaces around operators, commas and colons
  - strip trailing whitespace
  - collapse runs of blank lines to a single blank line
  - ensure the file ends with exactly one newline
  - wrap standalone `//` `#` `///` comment lines that exceed 100 columns

What it preserves (safety first):
  - the author's original line breaks: expressions are never reflowed and
    tokens never move across lines, so Koala's automatic semicolon
    insertion (which is newline sensitive, see src/parser/koala.l) is
    unaffected
  - string, char, bytes and `\"\"\"` text-block literals verbatim
  - `/* ... */` block comments verbatim
  - the token stream: the scanner mirrors koala.l, including signed numeric
    literals, so a unary `-`/`+` written apart from its operand is kept
    apart (it would otherwise re-lex into a different token)

Comment wrapping is deliberately narrow: only standalone comment lines are
broken, at word boundaries, and never inside fenced ``` code blocks, list or
table lines, or lines containing `://`. Trailing end-of-line comments and
block comments are left alone.
"""

import argparse
import difflib
import sys
from pathlib import Path

# --- format knobs ---------------------------------------------------------

INDENT_WIDTH = 4          # spaces per block level
COMMENT_LIMIT = 100       # wrap standalone comments beyond this column
MAX_BLANK = 1             # collapse longer blank-line runs to this many

# --- token roles ----------------------------------------------------------

NEWLINE = "newline"
WS = "ws"
LINE_COMMENT = "line_comment"      # // ... , # ... , /// ...
BLOCK_COMMENT = "block_comment"    # /* ... */
STRING = "string"
CHAR = "char"
BYTES = "bytes"
TEXT = "text"                      # \"\"\" ... \"\"\"
NUMBER = "number"
IDENT = "ident"
KEYWORD = "keyword"

OPENERS = {"(": "lparen", "[": "lbracket", "{": "lbrace",
           "{|": "lambda", "{||": "lambda"}
CLOSERS = {")": "rparen", "]": "rbracket", "}": "rbrace"}

KEYWORDS = {
    "from", "import", "link", "const", "let", "var", "func", "class",
    "trait", "if", "else", "while", "loop", "for", "switch", "case",
    "break", "continue", "return", "in", "as", "is", "pub", "infer",
    "static", "self", "true", "false", "nil",
    "uint8", "uint16", "uint32", "uint64", "uint",
    "int8", "int16", "int32", "int64", "int",
    "float16", "float32", "float64", "bfloat16", "float",
    "bool", "str", "bytes", "any", "list", "dict", "hashset", "tuple",
    "type", "range", "array",
}
VALUE_KEYWORDS = {"self", "true", "false", "nil"}

# keywords after which koala.l's semicolon() auto-inserts `;` at a newline;
# every other keyword (operators, `list`/`dict`/`func`/`if`/...) suppresses it,
# so the statement continues on the next line
ASI_ENDER_KEYWORDS = {
    "self", "true", "false", "nil",
    "uint8", "uint16", "uint32", "uint64", "uint",
    "int8", "int16", "int32", "int64", "int",
    "float16", "float32", "float64", "bfloat16", "float",
    "bool", "str", "bytes", "any",
    "return", "break", "continue",
}

# tokens after which a `-` / `+` is a binary operator (an operand just ended)
OPERAND_END = {IDENT, NUMBER, STRING, CHAR, BYTES, TEXT,
               "rparen", "rbracket", "rbrace", "qmark"}

MULTI_OPS = [
    "<<=", ">>=", "...", "{||", "{|",
    "&&", "||", "==", "!=", ">=", "<=", "+=", "-=", "*=", "/=", "%=",
    "&=", "|=", "^=", "<<", ">>", "?:", "?.", "!.",
]

# binary operators that get one space on each side
BINARY_OPS = {
    "+", "-", "*", "/", "%", "==", "!=", "<", ">", "<=", ">=",
    "&&", "||", "&", "|", "^", "<<", ">>",
    "=", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<=", ">>=",
    "?:", "in", "as", "is",
}
# keywords that take a space before a following `(` / `[` (control and
# declaration keywords); type keywords such as `list`/`dict` do not, so
# `list[T]` stays a generic without an inner space
SPACE_BEFORE_GROUP_KEYWORDS = {
    "if", "while", "for", "switch", "case", "return",
    "in", "as", "is", "let", "var", "const",
}

IDENT_START = set("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_$")
IDENT_CHARS = IDENT_START | set("0123456789")
DIGITS = set("0123456789")


class Tok:
    __slots__ = ("role", "text", "unary", "gap")

    def __init__(self, role, text):
        self.role = role
        self.text = text
        self.unary = False
        self.gap = ""            # whitespace that preceded this token

    def __repr__(self):
        return "Tok(%r,%r)" % (self.role, self.text)


# --- tokenizer (mirrors src/parser/koala.l) -------------------------------

def _lex_number(s, i):
    """Lex a numeric literal at s[i]; sign already consumed by the caller.

    Returns the end index. Mirrors the int/float/hex/oct/bin rules of
    koala.l. An un lexable tail simply stops the literal.
    """
    n = len(s)
    if i < n and s[i] == "0" and i + 1 < n and s[i + 1] in "bBoOxX":
        i += 2
        while i < n and (s[i] in "0123456789abcdefABCDEF_"):
            i += 1
        return i
    # integer part
    while i < n and (s[i] in DIGITS or s[i] == "_"):
        i += 1
    # fraction
    if i < n and s[i] == "." and i + 1 < n and s[i + 1] in DIGITS:
        i += 1
        while i < n and (s[i] in DIGITS or s[i] == "_"):
            i += 1
    # exponent
    if i < n and s[i] in "eE":
        j = i + 1
        if j < n and s[j] in "+-":
            j += 1
        if j < n and s[j] in DIGITS:
            i = j
            while i < n and s[i] in DIGITS:
                i += 1
    return i


def tokenize(src):
    """Split Koala source into a flat token list, preserving newlines."""
    toks = []
    i = 0
    n = len(src)
    while i < n:
        c = src[i]

        if c == "\n":
            toks.append(Tok(NEWLINE, "\n"))
            i += 1
            continue

        if c in " \t\r":
            j = i
            while j < n and src[j] in " \t\r":
                j += 1
            toks.append(Tok(WS, src[i:j]))
            i = j
            continue

        # comments
        if src.startswith("//", i) or c == "#":
            j = src.find("\n", i)
            if j == -1:
                j = n
            toks.append(Tok(LINE_COMMENT, src[i:j]))
            i = j
            continue
        if src.startswith("/*", i):
            j = src.find("*/", i + 2)
            j = n if j == -1 else j + 2
            toks.append(Tok(BLOCK_COMMENT, src[i:j]))
            i = j
            continue

        # text block, bytes, string, char (verbatim, may span lines)
        if src.startswith('"""', i):
            j = src.find('"""', i + 3)
            j = n if j == -1 else j + 3
            toks.append(Tok(TEXT, src[i:j]))
            i = j
            continue
        if c == "b" and i + 1 < n and src[i + 1] == '"':
            j = _scan_quoted(src, i + 2, '"')
            toks.append(Tok(BYTES, src[i:j]))
            i = j
            continue
        if c == '"':
            j = _scan_quoted(src, i + 1, '"')
            toks.append(Tok(STRING, src[i:j]))
            i = j
            continue
        if c == "'":
            j = _scan_quoted(src, i + 1, "'")
            toks.append(Tok(CHAR, src[i:j]))
            i = j
            continue

        # identifier / keyword
        if c in IDENT_START:
            j = i
            while j < n and src[j] in IDENT_CHARS:
                j += 1
            word = src[i:j]
            toks.append(Tok(KEYWORD if word in KEYWORDS else IDENT, word))
            i = j
            continue

        # numeric literal (optionally signed, per koala.l greedy rule)
        if c in DIGITS or ((c in "+-") and i + 1 < n and src[i + 1] in DIGITS):
            start = i
            if src[i] in "+-":
                i += 1
            i = _lex_number(src, i)
            toks.append(Tok(NUMBER, src[start:i]))
            continue

        if c == "@":
            toks.append(Tok("at", "@"))
            i += 1
            continue

        # multi-char operators (longest match first)
        matched = None
        for op in MULTI_OPS:
            if src.startswith(op, i):
                matched = op
                break
        if matched is not None:
            role = OPENERS.get(matched, "op")
            toks.append(Tok(role, matched))
            i += len(matched)
            continue

        # single-char punctuation / operators
        if c in OPENERS:
            toks.append(Tok(OPENERS[c], c))
            i += 1
            continue
        if c in CLOSERS:
            toks.append(Tok(CLOSERS[c], c))
            i += 1
            continue
        if c == ",":
            toks.append(Tok("comma", ","))
            i += 1
            continue
        if c == ";":
            toks.append(Tok("semi", ";"))
            i += 1
            continue
        if c == ":":
            toks.append(Tok("colon", ":"))
            i += 1
            continue
        if c == ".":
            toks.append(Tok("dot", "."))
            i += 1
            continue
        if c == "?":
            toks.append(Tok("qmark", "?"))
            i += 1
            continue
        if c in "!~":
            toks.append(Tok("op", c))
            i += 1
            continue
        # any remaining operator character
        toks.append(Tok("op", c))
        i += 1

    # fold each whitespace run into the following token's `gap`, then drop the
    # WS tokens: `gap` recovers a line's original indent (the first token after
    # a newline) and a trailing comment's original alignment padding
    folded = []
    pending = ""
    for t in toks:
        if t.role == WS:
            pending += t.text
            continue
        t.gap = pending
        pending = ""
        folded.append(t)
    return folded


def _scan_quoted(s, i, quote):
    """Advance past a single-line quoted literal opened before s[i].

    Handles backslash escapes; stops at the matching quote, at a newline
    (Koala string/char/bytes literals are single line) or at end of input.
    """
    n = len(s)
    while i < n:
        ch = s[i]
        if ch == "\\" and i + 1 < n:
            i += 2
            continue
        if ch == quote:
            return i + 1
        if ch == "\n":
            return i
        i += 1
    return n


# --- indentation contexts -------------------------------------------------

class Ctx:
    __slots__ = ("kind", "content_indent", "closer_indent")

    def __init__(self, kind, content_indent, closer_indent):
        self.kind = kind
        self.content_indent = content_indent
        self.closer_indent = closer_indent


# --- spacing --------------------------------------------------------------

def _is_operand_end(tok):
    if tok is None:
        return False
    if tok.role in OPERAND_END:
        return True
    if tok.role == KEYWORD and tok.text in VALUE_KEYWORDS:
        return True
    if tok.role == "op" and tok.text == "!" and not tok.unary:
        return True                        # postfix force-unwrap `(x as T)!`
    return False


def _mark_unary(line_toks):
    """Flag `-` / `+` / `!` tokens by left context.

    `.unary` marks a *prefix* operator: unary minus/plus, or logical not.
    A `!` right after an operand is postfix force-unwrap (`.unary` False).
    """
    prev = None
    for t in line_toks:
        # comments are transparent, so `1 /* c */ + 2` keeps `+` binary
        if t.role in (LINE_COMMENT, BLOCK_COMMENT):
            continue
        if t.role == "op" and t.text in ("-", "+", "!"):
            t.unary = not _is_operand_end(prev)
        prev = t


def _is_asi_ender(tok):
    """Mirror koala.l semicolon(): True when a newline right after `tok`
    auto-inserts `;`, i.e. `tok` can legally end a statement. Its complement
    (operators, open brackets, `,` `.` `:` ...) suppresses insertion, which is
    why a continued statement keeps its operator at the end of the line.
    """
    if tok is None:
        return False
    r = tok.role
    if r in (NUMBER, STRING, CHAR, BYTES, TEXT, IDENT):
        return True
    if r == KEYWORD:
        return tok.text in ASI_ENDER_KEYWORDS
    if r in ("rparen", "rbracket", "rbrace", "qmark"):
        return True
    if r == "op" and tok.text == "!":
        return True                        # NOT (postfix force-unwrap)
    return False


def _last_code_token(line):
    """Last non-comment token of a line. Comments never update koala.l's
    ps->token, so they are transparent to semicolon insertion."""
    for t in reversed(line.tokens):
        if t.role not in (LINE_COMMENT, BLOCK_COMMENT):
            return t
    return None


def _is_continuation(lines, idx):
    """True when line `idx` is an operand continuation of a multi-line
    expression: the previous line ends with a token that suppresses ASI (an
    operator, `,`, `.`, ...). Comment-only lines are transparent and a blank
    line ends the statement. A trailing block opener (`{` / `{|`) is excluded:
    the next line is then a block body, indented by the context stack, not a
    hand-aligned operand.
    """
    j = idx - 1
    while j >= 0:
        prev = lines[j]
        if not prev.tokens:                # blank line breaks the continuation
            return False
        last = _last_code_token(prev)
        if last is None:                   # comment-only line: look further up
            j -= 1
            continue
        if _is_asi_ender(last):
            return False
        return last.role not in ("lbrace", "lambda")
    return False


def _need_space(prev, cur, line_toks):
    """Return True when exactly one space separates prev and cur."""
    pr, cr = prev.role, cur.role

    # member access (`.`, optional `?.`, force `!.`) hugs both sides
    if pr == "dot" or cr == "dot" \
            or cur.text in ("?.", "!.") or prev.text in ("?.", "!."):
        return False
    # postfix optional-type `?` hugs left; do NOT suppress the space after
    # it, so `int? = nil` keeps its space before `=` (falls through below)
    if cr == "qmark":
        return False

    # parens and square brackets hug their contents
    if pr in ("lparen", "lbracket"):
        return False
    if cr in ("rparen", "rbracket"):
        return False
    # braces use block style: one inner space unless the block is empty
    if pr in ("lbrace", "lambda"):
        return cr != "rbrace"
    if cr == "rbrace":
        return pr not in ("lbrace", "lambda")

    # commas / semicolons: hug left, space right
    if cr in ("comma", "semi"):
        return False
    if pr in ("comma", "semi"):
        return True

    # annotations
    if pr == "at":
        return False

    # colon: bound / slice / dict / case / base, via _colon_space
    if cr == "colon" or pr == "colon":
        return _colon_space(prev, cur, line_toks)

    # `~` is always prefix; `!` is prefix (logical not) or postfix (force
    # unwrap) per left context, as flagged by _mark_unary
    if pr == "op" and prev.text == "~":
        return False
    if pr == "op" and prev.text == "!" and prev.unary:
        return False                       # no space after a prefix `!`
    if cr == "op" and cur.text in ("!", "~"):
        if cur.text == "!" and not cur.unary:
            return False                   # postfix force-unwrap hugs left
        return True                        # prefix: a space before it

    # `-` / `+`: unary keeps a space only before a numeric literal so the
    # token stream does not merge into a signed literal (koala.l greedy rule)
    if pr == "op" and prev.text in ("-", "+") and prev.unary:
        return cur.role == NUMBER
    if cr == "op" and cur.text in ("-", "+"):
        if cur.unary:
            # space before a unary sign only when the left token is an
            # operator/keyword that already wants a trailing space
            return pr in ("op", "keyword") or prev.text in BINARY_OPS
        return True

    # ellipsis: `args ...T` -> space before, none after
    if cr == "op" and cur.text == "...":
        return _is_operand_end(prev) or pr == KEYWORD
    if pr == "op" and prev.text == "...":
        return False

    # opening ( [ : a call/index/generic hugs the name; a control keyword or
    # a binary operator takes a space (`if (`, `return (`, `= [`, `x in [`)
    if cr in ("lparen", "lbracket"):
        if pr == "op" and prev.text in BINARY_OPS:
            return True
        if pr == KEYWORD and prev.text in SPACE_BEFORE_GROUP_KEYWORDS:
            return True
        # a tuple/array return type keeps its space: `func f() (T, T)`
        if pr == "rparen" and _closes_func_params(line_toks, prev):
            return True
        return False

    # a block brace / lambda always takes a space before it
    if cr in ("lbrace", "lambda"):
        return True

    # binary operators and keywords get a space on each side
    if cr == "op" and cur.text in BINARY_OPS:
        return True
    if pr == "op" and prev.text in BINARY_OPS:
        return True
    if cr == KEYWORD or pr == KEYWORD:
        return True

    # default: separate operands / identifiers
    return True


def _colon_space(prev, cur, line_toks):
    """Spacing around `:`.

    Five shapes exist in Koala, told apart by the innermost bracket that
    encloses the colon on this line plus a leading `case` keyword:
      - generic bound `func f[T : C]` -> a space on each side
      - base class   `class X : B`    -> a space on each side
      - dict literal `{k: v}`         -> hug left, space after
      - case label   `case 1:`        -> hug left, space after
      - slice        `a[1:2]`         -> no space either side
    A `[` introduces a bound only when it opens a declaration's generic
    parameter list (`func`/`class`/`trait`/`type` NAME `[`); any other `[`
    is a subscript, so its colon is a slice.
    """
    target = cur if cur.role == "colon" else prev
    oi = _innermost_opener(line_toks, target)
    opener = line_toks[oi] if oi is not None else None
    is_case = bool(line_toks) and line_toks[0].role == KEYWORD \
        and line_toks[0].text == "case"

    if opener is not None and opener.role == "lbracket":
        kind = "bound" if _is_generic_params(line_toks, oi) else "slice"
    elif opener is None:
        kind = "case" if is_case else "base"
    else:
        kind = "dict"                       # {k: v}, or a stray `:` in (...)

    if kind in ("bound", "base"):
        return True                         # a space on each side
    if cur.role == "colon":                 # space before the colon?
        return False                        # dict / case / slice hug left
    return kind != "slice"                  # space after, except a slice


def _innermost_opener(line_toks, target):
    """Index of the opener of the innermost unclosed bracket before `target`.

    Returns None when `target` sits at bracket depth zero on its line.
    """
    idx = None
    for k, t in enumerate(line_toks):
        if t is target:
            idx = k
            break
    if idx is None:
        return None
    stack = []
    for k, t in enumerate(line_toks[:idx]):
        if t.role in ("lparen", "lbracket", "lbrace", "lambda"):
            stack.append(k)
        elif t.role in ("rparen", "rbracket", "rbrace"):
            if stack:
                stack.pop()
    return stack[-1] if stack else None


def _is_generic_params(line_toks, oi):
    """True when the `[` at index `oi` opens a declaration's generic parameter
    list, written `func`/`class`/`trait`/`type` NAME `[`. Only such a list may
    hold a `T : Bound` colon; every other `[` is a subscript (a slice colon).
    """
    if oi - 2 < 0:
        return False
    name, kw = line_toks[oi - 1], line_toks[oi - 2]
    return name.role == IDENT and kw.role == KEYWORD \
        and kw.text in ("func", "class", "trait", "type")


def _closes_func_params(line_toks, target):
    """True when `target` is the `)` closing a func's parameter list.

    Lets a tuple/array return type keep its separating space, as in
    `func divmod(a, b) (T, T)`, while a chained call `f()(x)` stays tight.
    """
    fi = None
    for k, t in enumerate(line_toks):
        if t.role == KEYWORD and t.text == "func":
            fi = k
            break
    if fi is None:
        return False
    depth = 0                 # nesting of [ ] / { } before the param list
    paren = 0                 # nesting inside the parameter list
    opened = False
    for t in line_toks[fi + 1:]:
        if not opened:
            if t.role in ("lbracket", "lbrace", "lambda"):
                depth += 1
            elif t.role in ("rbracket", "rbrace"):
                depth -= 1
            elif t.role == "lparen" and depth == 0:
                opened = True
                paren = 1
            continue
        if t.role == "lparen":
            paren += 1
        elif t.role == "rparen":
            paren -= 1
            if paren == 0:
                return t is target
    return False


# --- comment wrapping -----------------------------------------------------

_FENCE = "```"
_LIST_MARKERS = ("-", "*", "+")


def _comment_prefix(text):
    """Return the comment marker plus its single following space, if any."""
    if text.startswith("///"):
        head = "///"
    elif text.startswith("//"):
        head = "//"
    elif text.startswith("#"):
        head = "#"
    else:
        return ""
    rest = text[len(head):]
    if rest.startswith(" "):
        return head + " "
    return head


def _is_directive(stripped):
    """True for a lit/FileCheck directive such as `CHECK:`, `CHECK-LABEL:`,
    `IR-NOT:` or `RUN:`. Wrapping one would split it across lines and the test
    harness would mis-read it, so these are never wrapped. Any leading
    all-caps label (`TODO:`, `NOTE:`, ...) is treated the same safe way.
    """
    if not stripped or not ("A" <= stripped[0] <= "Z"):
        return False
    i, n = 1, len(stripped)
    while i < n and ("A" <= stripped[i] <= "Z"
                     or stripped[i].isdigit() or stripped[i] == "_"):
        i += 1
    return i < n and stripped[i] in "-:"


def _is_unwrappable(body):
    """A comment body that must not be word-wrapped."""
    stripped = body.strip()
    if not stripped:
        return True
    if _is_directive(stripped):
        return True
    if stripped.startswith(_FENCE):
        return True
    if "://" in body:
        return True
    if stripped.startswith(_LIST_MARKERS):
        # a markdown list item; wrapping would need hanging indentation
        after = stripped[0]
        if after in "-*+":
            return True
    if stripped[0].isdigit():
        # numbered list item "1. ..."
        if ". " in stripped[:5]:
            return True
    if "|" in stripped:            # a markdown table row
        return True
    return False


def wrap_comment(line, limit):
    """Break an over-long standalone comment line into several lines.

    `line` is the already-indented, trailing-whitespace-stripped comment
    line. Only an over-long line's own length is reduced; short lines are
    never joined. Returns a list of physical lines (already indented)."""
    indent = len(line) - len(line.lstrip(" "))
    comment = line[indent:]
    prefix = _comment_prefix(comment)
    if not prefix:
        return [line]
    body = comment[len(prefix):]
    if _is_unwrappable(body):
        return [line]

    pad = " " * indent
    budget = limit - indent - len(prefix)
    if budget < 8 or indent + len(comment) <= limit:
        return [line]                 # too tight, or already short enough

    out = []
    cur = ""
    for w in body.split(" "):
        if w == "":
            continue
        if cur == "":
            cur = w
        elif len(cur) + 1 + len(w) <= budget:
            cur = cur + " " + w
        else:
            out.append(pad + prefix + cur)
            cur = w
    if cur != "":
        out.append(pad + prefix + cur)
    return out or [line]


# --- line model & formatting ----------------------------------------------

class Line:
    __slots__ = ("tokens", "blank")

    def __init__(self, tokens, blank=False):
        self.tokens = tokens
        self.blank = blank


def split_lines(toks):
    """Group tokens into logical lines separated by NEWLINE tokens."""
    lines = []
    cur = []
    for t in toks:
        if t.role == NEWLINE:
            lines.append(Line(cur))
            cur = []
        else:
            cur.append(t)
    if cur:
        lines.append(Line(cur))
    return lines


def _starts_with_closer(line):
    return bool(line.tokens) and line.tokens[0].role in \
        ("rparen", "rbracket", "rbrace")


def format_source(src):
    toks = tokenize(src)
    lines = split_lines(toks)

    stack = []
    out = []
    stmt_indent = 0

    for idx, line in enumerate(lines):
        if not line.tokens:
            out.append(_PhysicalLine("", []))
            continue

        _mark_unary(line.tokens)

        # decide this line's indent from the context stack
        cont = _is_continuation(lines, idx)
        if _starts_with_closer(line) and stack:
            indent = stack[-1].closer_indent
        elif stack and stack[-1].kind == "align":
            indent = stack[-1].content_indent       # A2: align under bracket
        elif cont:
            # operator continuation outside any bracket: keep the author's
            # hand-alignment (never reflowed, so ASI stays intact)
            indent = len(line.tokens[0].gap.expandtabs(INDENT_WIDTH))
        elif stack:
            indent = stack[-1].content_indent
        else:
            indent = 0

        # a block opener on a continuation line still closes at the statement's
        # own indent, so track where the current statement began
        if not cont:
            stmt_indent = indent

        body, stack = _render_line(line.tokens, indent, stack, stmt_indent)
        text = (" " * indent + body) if body else ""
        out.append(_PhysicalLine(text, line.tokens))

    return _assemble(out)


class _PhysicalLine:
    __slots__ = ("text", "tokens", "blank")

    def __init__(self, text, tokens):
        self.text = text
        self.tokens = tokens
        self.blank = (text.strip() == "")


def _render_line(tokens, indent, stack, stmt_indent=None):
    """Emit one logical line's tokens, updating the indent stack.

    Returns (body_text, stack). Verbatim multi-line tokens (text blocks,
    block comments) are emitted untouched, including their own newlines.
    """
    if stmt_indent is None:
        stmt_indent = indent
    parts = []
    col = indent
    prev = None
    i = 0
    n = len(tokens)
    while i < n:
        t = tokens[i]
        text = t.text

        if prev is not None:
            if t.role == LINE_COMMENT and t.gap:
                # keep the author's alignment padding before a trailing comment
                parts.append(t.gap)
                col += len(t.gap)
            elif _need_space(prev, t, tokens):
                parts.append(" ")
                col += 1

        # verbatim multi-line token: emit as-is, keep internal newlines
        if t.role in (TEXT, BLOCK_COMMENT) and "\n" in text:
            parts.append(text)
            # the column after such a token is its last physical line length
            col = len(text.split("\n")[-1])
            prev = t
            i += 1
            continue

        parts.append(text)
        col += len(text)

        # maintain the indent stack for brackets
        if t.role in ("lbrace", "lambda"):
            stack.append(Ctx("block", stmt_indent + INDENT_WIDTH, stmt_indent))
        elif t.role in ("lparen", "lbracket"):
            stack.append(Ctx("align", col, indent))
        elif t.role in ("rparen", "rbracket", "rbrace"):
            if stack:
                stack.pop()

        prev = t
        i += 1

    return "".join(parts).rstrip(), stack


def _assemble(items):
    """Collapse blank runs, trim leading blanks, ensure one final newline."""
    result = []
    for it in items:
        if it.blank:
            # count consecutive blanks against MAX_BLANK
            trailing = 0
            for prev in reversed(result):
                if prev.blank:
                    trailing += 1
                else:
                    break
            if trailing < MAX_BLANK:
                result.append(it)
            continue
        result.append(it)

    # drop leading blank lines
    while result and result[0].blank:
        result.pop(0)

    # expand each item into physical lines, applying comment wrapping
    physical = []
    for it in result:
        if it.blank:
            physical.append("")
            continue
        rendered = _maybe_wrap(it)
        physical.extend(rendered)

    text = "\n".join(physical)
    if text and not text.endswith("\n"):
        text += "\n"
    return text


def _maybe_wrap(item):
    """Return the physical lines for one formatted logical line."""
    text = item.text
    if "\n" in text:
        # contains a verbatim multi-line token (text block / block comment):
        # keep untouched, interior lines are content
        return [text]
    toks = item.tokens
    # standalone comment line: the only case we word-wrap
    if len(toks) == 1 and toks[0].role == LINE_COMMENT:
        return wrap_comment(text, COMMENT_LIMIT)
    # a trailing comment that pushes the line over the limit moves onto its own
    # line above the code; ASI-safe, since a comment never updates ps->token
    if (len(toks) >= 2 and toks[-1].role == LINE_COMMENT
            and len(text) > COMMENT_LIMIT and text.endswith(toks[-1].text)):
        comment = toks[-1].text
        indent = len(text) - len(text.lstrip(" "))
        code = text[:-len(comment)].rstrip()
        return wrap_comment(" " * indent + comment, COMMENT_LIMIT) + [code]
    return [text]


# --- driver ---------------------------------------------------------------

def collect_kl_files(path):
    p = Path(path)
    if p.is_file():
        return [p]
    if p.is_dir():
        return sorted(p.rglob("*.kl"))
    return []


class HelpOnErrorParser(argparse.ArgumentParser):
    """ArgumentParser that prints the full help on any argument error."""

    def error(self, message):
        self.print_help(sys.stderr)
        sys.stderr.write("\nerror: %s\n" % message)
        sys.exit(2)


def main():
    global COMMENT_LIMIT
    ap = HelpOnErrorParser(
        description="Conservative whitespace formatter for Koala (.kl) sources."
    )
    ap.add_argument(
        "src",
        metavar="input file or directory",
        help="a single .kl file, or a directory to scan recursively for .kl",
    )
    ap.add_argument("-w", "--write", action="store_true",
                    help="rewrite files in place")
    ap.add_argument("--check", action="store_true",
                    help="exit 1 if any file would change; write nothing")
    ap.add_argument("--diff", action="store_true",
                    help="print a unified diff of each change")
    ap.add_argument("--no-wrap", action="store_true",
                    help="disable comment wrapping at %d columns" % COMMENT_LIMIT)
    args = ap.parse_args()

    if args.no_wrap:
        COMMENT_LIMIT = 10 ** 9

    files = collect_kl_files(args.src)
    if not files:
        ap.error("no .kl file found at: %s" % args.src)

    single = len(files) == 1 and Path(args.src).is_file()
    stdout_mode = single and not args.write and not args.check and not args.diff
    changed = 0
    for path in files:
        original = path.read_text(encoding="utf-8")
        try:
            formatted = format_source(original)
        except Exception as exc:  # never destroy a file on a formatter bug
            sys.stderr.write("kl-fmt: %s: %s\n" % (path, exc))
            continue

        if stdout_mode:                 # single file, no flags -> emit source
            sys.stdout.write(formatted)
            continue

        if formatted == original:
            continue
        changed += 1

        if args.diff:
            diff = difflib.unified_diff(
                original.splitlines(keepends=True),
                formatted.splitlines(keepends=True),
                fromfile=str(path) + " (original)",
                tofile=str(path) + " (formatted)",
            )
            sys.stdout.writelines(diff)

        if args.write:
            path.write_text(formatted, encoding="utf-8")
            print("kl-fmt: formatted %s" % path)
        elif not args.diff:
            print("kl-fmt: %s would change" % path)

    if args.check and changed:
        sys.exit(1)
    if not single and not args.write:
        print("kl-fmt: %d of %d file(s) would change" % (changed, len(files)))


if __name__ == "__main__":
    main()
