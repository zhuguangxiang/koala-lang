import shlex, io, os, sys

OUT = sys.argv[1]

def kl(s):
    if s is None:
        return "nil"
    out = []
    for c in s:
        if c == "\\": out.append("\\\\")
        elif c == '"': out.append('\\"')
        elif c == "\n": out.append("\\n")
        elif c == "\t": out.append("\\t")
        elif c == "\r": out.append("\\r")
        else: out.append(c)
    return '"' + "".join(out) + '"'

def kl_list(xs):
    if not xs:
        return "list[str]()"
    return "[" + ", ".join(kl(x) for x in xs) + "]"

def want_lines(xs, indent="    "):
    """`let want = [...]`, wrapped at 100 columns."""
    head = indent + "let want = "
    one = head + kl_list(xs)
    if len(one) <= 100:
        return [one]
    out = [head + "["]
    cur = indent + "    "
    for x in xs:
        item = kl(x) + ","
        if len(cur) + len(item) + 1 > 100 and cur.strip():
            out.append(cur.rstrip())
            cur = indent + "    "
        cur += item + " "
    if cur.strip():
        out.append(cur.rstrip())
    out.append(indent + "]")
    return out

def kl_wrapped(s, prefix, indent="    "):
    """`prefix` followed by the literal `s`, split into `+` pieces at 100 columns."""
    one = indent + prefix + kl(s)
    if len(one) <= 100:
        return [one]
    pieces = []
    cur = ""
    for c in s:
        if len(kl(cur + c)) > 60:
            pieces.append(cur)
            cur = ""
        cur += c
    pieces.append(cur)
    if len(pieces) == 1:
        return [one]
    out = [indent + prefix + kl(pieces[0]) + " +"]
    pad = indent + " " * len(prefix)
    for i, piece in enumerate(pieces[1:]):
        tail = " +" if i < len(pieces) - 2 else ""
        out.append(pad + kl(piece) + tail)
    return out

def comment(text, indent="    "):
    line = indent + "// " + text
    if len(line) > 100:
        line = line[:97] + "..."
    return line

HEADER = '''/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

// Generated from CPython {ver}'s shlex: every expected value below is what
// Python returns for the same input.

import "std/ut"
import "koala/shlex"
'''.replace("{ver}", sys.version.split()[0])

HELPERS = '''
func _check(got list[str]?, want list[str]) {
    ut.assert_true(got != nil)
    let tokens = got!
    ut.assert_eq(tokens.len(), want.len())
    for i in range(want.len()) {
        ut.assert_eq(tokens[i], want[i])
    }
}
'''

# ---------------------------------------------------------------- split
split_inputs = [
    "a b c", "  a   b  ", "", "   ", "add \"buy milk\" -p 1", "a 'b c' d",
    "a\\ b", "\"say \\\"hi\\\"\"", "x\"y z\"w", "'a \"b\"'", "\"a\\\\b\"",
    "\"a\\b\"", "'a\\b'", "a#b c", "a #b c", "\"\" '' x", "a\"\" b",
    "tab\tsep\nnl", "考拉 \"中 文\"", "~/x *.py a=b", "a;b|c&&d", "\\\"",
    "é ß", "echo $HOME '$HOME' \"$HOME\"", "a\\\nb", "\"multi\nline\"",
    "--opt=\"a b\" -x", "'it'\\''s'", "\\\\", "a\\b", "\"\\$x\" \"\\`x\" \"\\n\"",
    "  # only a comment", "a # rest\nb", "x\r\ny",
]
lines = [HEADER, HELPERS]
n = 0
def add_split(s, comments=False, posix=True):
    global n
    n += 1
    try:
        want = shlex.split(s, comments=comments, posix=posix)
    except ValueError as e:
        want = None
    args = kl(s)
    if comments: args += ", comments=true"
    if not posix: args += ", posix=false"
    lines.append("@test")
    lines.append("func test_split_%d() {" % n)
    lines.append(comment(repr(s) + ("" if not comments else ", comments") + ("" if posix else ", non-posix")))
    if want is None:
        lines.append("    ut.assert_true(shlex.split(%s) == nil)" % args)
    else:
        lines.extend(want_lines(want))
        lines.append("    _check(shlex.split(%s), want)" % args)
    lines.append("}")
    lines.append("")

for s in split_inputs:
    add_split(s)
for s in ["a#b c", "a #b c", "  # only a comment", "a # rest\nb", "x 'y # z' # w", "#\n#x\ny"]:
    add_split(s, comments=True)
for s in split_inputs + ["'a'b\"c\"", "a'b'c"]:
    add_split(s, posix=False)
for s in ["\"abc", "'abc", "abc\\", "a \"b", "\"a\\"]:
    add_split(s)
open(os.path.join(OUT, "test_split.kl"), "w").write("\n".join(lines))

# ---------------------------------------------------------------- quote / join
lines = [HEADER]
quotes = ["", "abc", "a b", "it's", "$HOME", "é", "a-b.c/d:e=f@g%h+i,j_K",
          "*", "\n", "'", "\"", "a'b\"c", "~user", "=", "考拉", "tab\there"]
for i, s in enumerate(quotes):
    lines.append("@test")
    lines.append("func test_quote_%d() {" % i)
    lines.append("    ut.assert_eq(shlex.quote(%s), %s)" % (kl(s), kl(shlex.quote(s))))
    lines.append("}")
    lines.append("")
joins = [["echo", "a b"], [], ["it's", "", "x"], ["a", "$b", "c d e"]]
for i, xs in enumerate(joins):
    lines.append("@test")
    lines.append("func test_join_%d() {" % i)
    lines.append("    ut.assert_eq(shlex.join(%s), %s)" % (kl_list(xs), kl(shlex.join(xs))))
    lines.append("}")
    lines.append("")
lines.append("@test")
lines.append("func test_join_round_trip() {")
for xs in [["a b", "it's", "$x", "", "考拉"], ["--opt=\"q\"", "\\"]]:
    lines.append("    {")
    lines.append("        let parts = %s" % kl_list(xs))
    lines.append("        let back = shlex.split(shlex.join(parts))!")
    lines.append("        ut.assert_eq(back.len(), parts.len())")
    lines.append("        for i in range(parts.len()) { ut.assert_eq(back[i], parts[i]) }")
    lines.append("    }")
lines.append("}")
open(os.path.join(OUT, "test_quote.kl"), "w").write("\n".join(lines))

# ---------------------------------------------------------------- lexer
lines = [HEADER, HELPERS, '''
// The tokens of `lex` up to its eof; nil on a lexing error.
func _tokens(lex shlex.shlex) list[str]? {
    let out = list[str]()
    for token in lex { out.append(token) }
    if lex.error().len() > 0 { return nil }
    return out
}
''']
m = 0
def lexcase(desc, text, posix=False, punct=None, ws=None, extra=None):
    global m
    m += 1
    kw = {}
    if posix: kw["posix"] = True
    if punct is not None: kw["punctuation_chars"] = punct
    lex = shlex.shlex(text, **kw)
    if ws is not None: lex.whitespace_split = ws
    if extra: extra(lex)
    try:
        want = list(lex)
        err = None
    except ValueError as e:
        want = None
        err = str(e)
    args = "text"
    if posix: args += ", posix=true"
    if punct is True: args += ", punctuation_chars=true"
    elif isinstance(punct, str): args += ", punctuation_chars=" + kl(punct)
    lines.append("@test")
    lines.append("func test_lexer_%d() {" % m)
    lines.append(comment(desc))
    lines.extend(kl_wrapped(text, "let text = "))
    lines.append("    let lex = shlex.shlex(%s)" % args)
    if ws is not None: lines.append("    lex.whitespace_split = %s" % ("true" if ws else "false"))
    if extra: lines.append(extra.kl)
    if want is None:
        lines.append("    ut.assert_true(_tokens(lex) == nil)")
        lines.append("    ut.assert_eq(lex.error(), %s)" % kl(err))
    else:
        lines.extend(want_lines(want))
        lines.append("    _check(_tokens(lex), want)")
        lines.append("    ut.assert_eq(lex.lineno, %d)" % lex.lineno)
    lines.append("}")
    lines.append("")

base = "a+b c=d \"q x\" 's' #c\nnext -o=v 1.5 x_y é"
lexcase("non-posix default", base)
lexcase("posix", base, posix=True)
lexcase("non-posix, whitespace_split", base, ws=True)
lexcase("posix, whitespace_split", base, posix=True, ws=True)
p = "a && b; c|d (e) >f g>>h ~/x.py *.c a=1"
lexcase("punctuation_chars=true", p, punct=True)
lexcase("punctuation_chars=true, posix", p, posix=True, punct=True)
lexcase("punctuation_chars=true, whitespace_split", p, punct=True, ws=True)
lexcase("punctuation_chars=\"|\"", "a|b||c d;e", punct="|")
lexcase("posix escapes and quotes", "a\\ b \"c\\\"d\" 'e\\f' \"g\\h\" i\"j k\"l", posix=True)
lexcase("non-posix keeps quotes", "'a b'\"c\" d'e'", )
lexcase("posix empty quotes", "'' \"\" x''", posix=True)
lexcase("lines and comments", "one # c1\ntwo\n\nthree # c2\n", posix=True)
lexcase("unclosed quote", "a 'b", posix=True)
lexcase("trailing escape", "a b\\", posix=True)
lexcase("unclosed quote, non-posix", "a \"b")

def custom(lex):
    lex.wordchars += "-."
    lex.commenters = ";"
    lex.quotes = "|"
custom.kl = '''    lex.wordchars += "-."
    lex.commenters = ";"
    lex.quotes = "|"'''
lexcase("custom wordchars, commenters, quotes", "a-b.c ;comment\n|q x| #d e+f", extra=custom)

def noesc(lex):
    lex.escape = ""
noesc.kl = '''    lex.escape = ""'''
lexcase("posix without escapes", "a\\b \"c\\\"d", posix=True, extra=noesc)


# ---- eof, accessors, wordchars
for posix in (False, True):
    for punct in (False, True, "|;"):
        lex = shlex.shlex("x", posix=posix, punctuation_chars=punct)
        args = "\"x\", posix=%s, punctuation_chars=%s" % (str(posix).lower(),
                 str(punct).lower() if isinstance(punct, bool) else kl(punct))
        m += 1
        lines.append("@test")
        lines.append("func test_lexer_%d() {" % m)
        lines.append(comment("eof, posix(), punctuation_chars(), the default attributes"))
        lines.append("    let lex = shlex.shlex(%s)" % args)
        if lex.eof is None:
            lines.append("    ut.assert_true(lex.eof == nil)")
        else:
            lines.append("    ut.assert_eq(lex.eof!, %s)" % kl(lex.eof))
        lines.append("    ut.assert_eq(lex.posix(), %s)" % str(posix).lower())
        lines.append("    ut.assert_eq(lex.punctuation_chars(), %s)" % kl(lex.punctuation_chars))
        lines.extend(kl_wrapped(lex.wordchars, "let chars = "))
        lines.append("    ut.assert_eq(lex.wordchars, chars)")
        lines.append("    ut.assert_eq(lex.commenters, %s)" % kl(lex.commenters))
        lines.append("    ut.assert_eq(lex.whitespace, %s)" % kl(lex.whitespace))
        lines.append("    ut.assert_eq(lex.quotes, %s)" % kl(lex.quotes))
        lines.append("    ut.assert_eq(lex.escape, %s)" % kl(lex.escape))
        lines.append("    ut.assert_eq(lex.escapedquotes, %s)" % kl(lex.escapedquotes))
        lines.append("    ut.assert_false(lex.whitespace_split)")
        lines.append("    ut.assert_true(lex.state! == \" \")")
        lines.append("    ut.assert_eq(lex.lineno, 1)")
        lines.append("    ut.assert_true(lex.source == nil)")
        lines.append("}")
        lines.append("")

# ---- push_token, read_token, pushback
lex = shlex.shlex("a b c", posix=True)
t1 = lex.get_token(); lex.push_token("x"); lex.push_token("y")
pb = list(lex.pushback)
t2 = lex.get_token(); t3 = lex.get_token(); t4 = lex.get_token()
lex.push_token("z"); raw = lex.read_token()
t5 = lex.get_token(); t6 = lex.get_token()
lines.append("""@test
func test_push_token() {
    let lex = shlex.shlex("a b c", posix=true)
    ut.assert_eq(lex.get_token()!, %s)
    lex.push_token("x")
    lex.push_token("y")
    // the last pushed comes first
    ut.assert_eq(lex.pushback.len(), %d)
    ut.assert_eq(lex.pushback[0], %s)
    ut.assert_eq(lex.get_token()!, %s)
    ut.assert_eq(lex.get_token()!, %s)
    ut.assert_eq(lex.get_token()!, %s)
    // read_token ignores the pushed-back tokens
    lex.push_token("z")
    ut.assert_eq(lex.read_token()!, %s)
    ut.assert_eq(lex.get_token()!, %s)
    ut.assert_true(lex.get_token() == %s)
}
""" % (kl(t1), len(pb), kl(pb[0]), kl(t2), kl(t3), kl(t4), kl(raw), kl(t5), kl(t6)))

# ---- push_source / pop_source
lex = shlex.shlex("a b", posix=True)
seq = [lex.get_token()]
lex.push_source("x y", "inner.txt")
seq.append(lex.infile); seq.append(lex.lineno)
seq += [lex.get_token(), lex.get_token(), lex.get_token(), lex.infile]
lines.append("""@test
func test_push_source() {
    let lex = shlex.shlex("a b", posix=true)
    ut.assert_eq(lex.get_token()!, %s)
    lex.push_source("x y", "inner.txt")
    ut.assert_eq(lex.infile()!, %s)
    ut.assert_eq(lex.lineno, %d)
    ut.assert_eq(lex.get_token()!, %s)
    ut.assert_eq(lex.get_token()!, %s)
    // the end of the pushed source returns to the first one
    ut.assert_eq(lex.get_token()!, %s)
    ut.assert_true(lex.infile() == %s)
    ut.assert_true(lex.get_token() == nil)
}
""" % (kl(seq[0]), kl(seq[1]), seq[2], kl(seq[3]), kl(seq[4]), kl(seq[5]), kl(seq[6])))

# ---- source inclusion through sourcehook, with files
import tempfile
d = "/tmp/koala_shlex_test"
os.makedirs(d + "/sub", exist_ok=True)
files = {
    "main.txt": "one source sub/inc.txt two\nsource \"sub/q.txt\" three\n",
    "sub/inc.txt": "in1 in2\n",
    "sub/q.txt": "quoted\n",
}
for name, text in files.items():
    open(os.path.join(d, name), "w").write(text)
with open(os.path.join(d, "main.txt")) as f:
    lex = shlex.shlex(f, os.path.join(d, "main.txt"), posix=True)
    lex.source = "source"
    lex.whitespace_split = True
    want = list(lex)
writes = []
for i, (n_, t) in enumerate(files.items()):
    writes.extend(kl_wrapped(t, "let text%d = " % i))
    writes.append("    fs.write_str(%s, text%d)" % (kl(os.path.join(d, n_)), i))
writes = "\n".join(writes)
lines.append("""@test
func test_source_inclusion() {
    fs.mkdir(%s)
    fs.mkdir(%s)
%s
    let main = %s
    let lex = shlex.shlex(fs.open(main)!, main, posix=true)
    lex.source = "source"
    lex.whitespace_split = true
%s
    _check(_tokens(lex), want)
}

@test
func test_source_missing_file() {
    let lex = shlex.shlex("a source nothere.txt b", posix=true)
    lex.source = "source"
    ut.assert_eq(lex.get_token()!, "a")
    ut.assert_true(lex.get_token() == nil)
    // without whitespace_split the name stops at the '.', as in Python
    ut.assert_eq(lex.error(), "[Errno 2] No such file or directory: 'nothere'")
}
""" % (kl(d), kl(d + "/sub"), writes, kl(os.path.join(d, "main.txt")), "\n".join(want_lines(want))))

# ---- error_leader
a = shlex.shlex("x\ny", "conf.txt"); a.get_token(); a.get_token()
b = shlex.shlex("x")
lines.append("""@test
func test_error_leader() {
    let lex = shlex.shlex("x\\ny", "conf.txt")
    lex.get_token()
    lex.get_token()
    ut.assert_eq(lex.error_leader(), %s)
    ut.assert_eq(lex.error_leader("other.txt", 7), %s)
    ut.assert_eq(shlex.shlex("x").error_leader(), %s)
}
""" % (kl(a.error_leader()), kl(a.error_leader("other.txt", 7)), kl(b.error_leader())))

# ---- streams
lines.append("""@test
func test_stringio_and_file() {
    let from_io = shlex.shlex(io.StringIO("p 'q r'"), posix=true)
    _check(_tokens(from_io), ["p", "q r"])

    let path = "/tmp/koala_shlex_test_stream.txt"
    fs.write_str(path, "f1 \\"f 2\\"\\n# c\\nf3 考拉\\n")
    let from_file = shlex.shlex(fs.open(path)!, path, posix=true)
    // a character outside wordchars is a token of its own, as in Python
    _check(_tokens(from_file), ["f1", "f 2", "f3", "考", "拉"])
    ut.assert_eq(from_file.lineno, 4)
}

@test
func test_error_is_sticky() {
    let lex = shlex.shlex("ok 'open", posix=true)
    ut.assert_eq(lex.get_token()!, "ok")
    ut.assert_true(lex.get_token() == nil)
    ut.assert_eq(lex.error(), "No closing quotation")
    ut.assert_true(lex.get_token() == nil)
    ut.assert_true(lex.read_token() == nil)
}
""")

lines[0] = lines[0].replace('import "koala/shlex"', 'import "std/fs"\nimport "std/io"\nimport "koala/shlex"')
open(os.path.join(OUT, "test_lexer.kl"), "w").write("\n".join(lines))
print("split", n, "lexer", m)
