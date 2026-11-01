/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "re_core.h"
#include "testhelp.h"

/*
 * T(pat, text, caps...): search `text` for `pat` and compare the capture
 * vector with the expected one; expectations are written like Go's
 * FindStringSubmatchIndex. NOMATCH() expects no match.
 */

static int match_with(const char *pat, uint32_t flags, const char *text, size_t start, int mode,
                      ptrdiff_t *caps, int *ncaps)
{
    ReError err;
    ReRegex *re = kl_re_compile(pat, strlen(pat), flags, &err);
    int rc;

    if (!re) return -100;
    *ncaps = 2 * (kl_re_ngroups(re) + 1);
    rc = kl_re_exec(re, text, strlen(text), start, mode, caps);
    kl_re_free(re);
    return rc;
}

static int expect(const char *pat, uint32_t flags, const char *text, size_t start, int mode,
                  int nwant, ...)
{
    ptrdiff_t caps[64], want[64];
    int ncaps = 0, rc;
    va_list ap;

    rc = match_with(pat, flags, text, start, mode, caps, &ncaps);
    if (nwant == 0) return rc == RE_NOMATCH;
    if (rc != RE_MATCH || ncaps != nwant) return 0;

    va_start(ap, nwant);
    for (int i = 0; i < nwant; i++) want[i] = va_arg(ap, int);
    va_end(ap);

    return memcmp(caps, want, (size_t)nwant * sizeof(ptrdiff_t)) == 0;
}

#define FIND(pat, text, n, ...) expect(pat, 0, text, 0, RE_MODE_SEARCH, n, __VA_ARGS__)
#define NOMATCH(pat, text)      expect(pat, 0, text, 0, RE_MODE_SEARCH, 0)
#define UFIND(pat, text, n, ...) expect(pat, RE_FLAG_UNICODE, text, 0, RE_MODE_SEARCH, n, __VA_ARGS__)

static int fails(const char *pat, int code)
{
    ReError err;
    ReRegex *re = kl_re_compile(pat, strlen(pat), 0, &err);

    if (re) {
        kl_re_free(re);
        return 0;
    }
    return err.code == code;
}

/* The spans of the successive matches of `pat` in `text` are the `nspan`
 * (start, end) pairs given. */
static int iter_spans(const char *pat, const char *text, int nspan, ...)
{
    ReRegex *re = kl_re_compile(pat, strlen(pat), 0, NULL);
    ReIter it = RE_ITER_INIT;
    ptrdiff_t caps[64];
    int n = 0, ok = re != NULL;
    va_list ap;

    va_start(ap, nspan);
    while (ok && kl_re_iter_next(re, text, strlen(text), &it, caps) == RE_MATCH) {
        if (n >= nspan || caps[0] != va_arg(ap, int) || caps[1] != va_arg(ap, int)) ok = 0;
        n++;
    }
    va_end(ap);
    kl_re_free(re);
    return ok && n == nspan;
}

/* a text of `n` copies of `c` */
static char *repeat_char(char c, size_t n)
{
    char *s = malloc(n + 1);

    memset(s, c, n);
    s[n] = '\0';
    return s;
}

int reCoreTest(void)
{
    /* ---------- literals and basic search ---------- */
    test_cond("Literal", FIND("a+b", "xaab", 2, 1, 4));
    test_cond("No match", NOMATCH("a+b", "xxx"));
    test_cond("Empty pattern matches at 0", FIND("", "abc", 2, 0, 0));
    test_cond("Empty pattern at the end", FIND("", "", 2, 0, 0));
    test_cond("Search resumes at the start offset",
              expect("", 0, "abc", 2, RE_MODE_SEARCH, 2, 2, 2));
    test_cond("Start beyond the text never matches", expect("", 0, "abc", 4, RE_MODE_SEARCH, 0));
    test_cond("Multi-byte literal",
              FIND("\xe4\xbd\xa0\xe5\xa5\xbd", "x\xe4\xbd\xa0\xe5\xa5\xbd", 2, 1, 7));
    test_cond("Dot matches a whole rune", FIND(".", "\xe4\xbd\xa0", 2, 0, 3));
    test_cond("Class matches a multi-byte rune",
              FIND("[\xe4\xbd\xa0\xe5\xa5\xbd]+", "xx\xe4\xbd\xa0\xe5\xa5\xbdy", 2, 2, 8));
    test_cond("Dot matches an invalid byte as one rune", FIND(".", "\xff", 2, 0, 1));
    test_cond("An invalid byte does not match a multi-byte literal",
              NOMATCH("\xe4\xbd\xa0", "\xe4\xbd"));

    /* ---------- alternation and priority ---------- */
    test_cond("Alternation is leftmost-first, not longest", FIND("a|ab", "ab", 2, 0, 1));
    test_cond("Alternation tries every branch", FIND("x|y|z", "..z", 2, 2, 3));
    test_cond("Empty alternative", FIND("a|", "b", 2, 0, 0));
    test_cond("Alternation inside a group", FIND("(a|ab)(c|bcd)", "abcd", 6, 0, 4, 0, 1, 1, 4));

    /* ---------- quantifiers ---------- */
    test_cond("Greedy star", FIND("a*", "aaa", 2, 0, 3));
    test_cond("Lazy star", FIND("a*?", "aaa", 2, 0, 0));
    test_cond("Greedy plus", FIND("a+", "baaa", 2, 1, 4));
    test_cond("Lazy plus", FIND("a+?", "baaa", 2, 1, 2));
    test_cond("Greedy quest", FIND("ab?", "ab", 2, 0, 2));
    test_cond("Lazy quest", FIND("ab??", "ab", 2, 0, 1));
    test_cond("Exact repeat", FIND("x{3}", "xxxxx", 2, 0, 3));
    test_cond("Bounded repeat", FIND("x{2,3}", "xxxx", 2, 0, 3));
    test_cond("Lazy bounded repeat", FIND("x{2,3}?", "xxxx", 2, 0, 2));
    test_cond("Open repeat", FIND("x{2,}", "xxxxx", 2, 0, 5));
    test_cond("Lazy open repeat", FIND("x{2,}?", "xxxxx", 2, 0, 2));
    test_cond("Zero repeat", FIND("ax{0}b", "ab", 2, 0, 2));
    test_cond("A brace that is not a repeat is a literal", FIND("x{,2}", "x{,2}", 2, 0, 5));
    test_cond("A lone brace is a literal", FIND("a{", "a{", 2, 0, 2));
    test_cond("U flag swaps greedy and lazy", FIND("(?U)a*", "aaa", 2, 0, 0));
    test_cond("U flag swaps lazy to greedy", FIND("(?U)a*?", "aaa", 2, 0, 3));
    test_cond("Repeat of a group", FIND("(ab){2}", "ababab", 4, 0, 4, 2, 4));
    test_cond("Nested repeat in a group", FIND("(?:a{2}){3}", "aaaaaaa", 2, 0, 6));
    test_cond("Star of an empty-width group terminates", FIND("(?:)*a", "a", 2, 0, 1));
    test_cond("Star of a nullable group terminates", FIND("(a*)*b", "aab", 4, 0, 3, 0, 2));
    test_cond("Star of a nullable body keeps the priority order", FIND("(?:.?"
                                                                       "?)*",
                                                                       "aab", 2, 0, 0));
    test_cond("Star of a nullable body still runs the body", FIND("(\\b)*", "a", 4, 0, 0, 0, 0));
    test_cond("Lazy star of a nullable body", FIND("(?:a?)*?b", "aab", 2, 0, 3));

    /* ---------- groups ---------- */
    test_cond("Captures", FIND("(a)(b)", "ab", 6, 0, 2, 0, 1, 1, 2));
    test_cond("Optional group that does not take part",
              FIND("(a)(b)?", "ac", 6, 0, 1, 0, 1, -1, -1));
    test_cond("Non-capturing group", FIND("(?:a)(b)", "ab", 4, 0, 2, 1, 2));
    test_cond("Named group", FIND("(?P<y>a)", "a", 4, 0, 1, 0, 1));
    test_cond("Named group, angle form", FIND("(?<y>a)", "a", 4, 0, 1, 0, 1));
    test_cond("Last iteration wins in a repeated group", FIND("(a|b)+", "ab", 4, 0, 2, 1, 2));
    test_cond("Nested groups", FIND("((a)(b))", "ab", 8, 0, 2, 0, 2, 0, 1, 1, 2));
    {
        const char *pat = "(?P<year>\\d{4})-(\\d\\d)-(?P<day>\\d\\d)";
        ReRegex *p = kl_re_compile(pat, strlen(pat), 0, NULL);
        test_cond("Group count and names",
                  p && kl_re_ngroups(p) == 3 && !strcmp(kl_re_group_name(p, 1), "year") &&
                      kl_re_group_name(p, 2) == NULL && !strcmp(kl_re_group_name(p, 3), "day") &&
                      kl_re_group_name(p, 0) == NULL && kl_re_group_name(p, 4) == NULL);
        test_cond("Group index by name",
                  p && kl_re_group_index(p, "day", 3) == 3 &&
                      kl_re_group_index(p, "year", 4) == 1 &&
                      kl_re_group_index(p, "nope", 4) == -1 &&
                      kl_re_group_index(p, "da", 2) == -1);
        kl_re_free(p);
    }

    /* ---------- anchors and boundaries ---------- */
    test_cond("^ matches only at the text start", NOMATCH("^b", "ab"));
    test_cond("$ matches only at the text end", FIND("a$", "ba", 2, 1, 2));
    test_cond("$ does not match before a trailing newline", NOMATCH("a$", "a\n"));
    test_cond("(?m)^ matches after a newline", FIND("(?m)^b", "a\nb", 2, 2, 3));
    test_cond("(?m)$ matches before a newline", FIND("(?m)a$", "a\nb", 2, 0, 1));
    test_cond("\\A and \\z", FIND("\\Aab\\z", "ab", 2, 0, 2));
    test_cond("\\A is not a line start in (?m)", NOMATCH("(?m)\\Ab", "a\nb"));
    test_cond("\\b", FIND("\\bfoo\\b", "a foo b", 2, 2, 5));
    test_cond("\\b is not inside a word", NOMATCH("\\bfoo\\b", "afoob"));
    test_cond("\\B", FIND("\\Boo", "foo", 2, 1, 3));
    test_cond("\\b at the text edges", FIND("\\b", "ab", 2, 0, 0));
    test_cond("\\b is ASCII only", NOMATCH("\\b", "\xc3\xa9"));
    test_cond("^ at a start offset sees the text before it",
              expect("^b", 0, "ab", 1, RE_MODE_SEARCH, 0));
    test_cond("\\b at a start offset sees the text before it",
              expect("\\bb", 0, "ab", 1, RE_MODE_SEARCH, 0));
    test_cond("(?m)^ at a start offset sees a newline before it",
              expect("(?m)^b", 0, "a\nb", 2, RE_MODE_SEARCH, 2, 2, 3));

    /* ---------- dot, classes, escapes ---------- */
    test_cond(". does not match \\n", NOMATCH(".", "\n"));
    test_cond("(?s). matches \\n", FIND("(?s).", "\n", 2, 0, 1));
    test_cond("S flag", expect(".", RE_FLAG_S, "\n", 0, RE_MODE_SEARCH, 2, 0, 1));
    test_cond("Negated class matches \\n", FIND("[^a]", "a\n", 2, 1, 2));
    test_cond("Range", FIND("[a-c]+", "xabcd", 2, 1, 4));
    test_cond("Negated range", FIND("[^a-c]+", "abxyc", 2, 2, 4));
    test_cond("Leading ] is literal", FIND("[]a]+", "x]a]", 2, 1, 4));
    test_cond("Leading ^] is literal", FIND("[^]a]+", "]ab", 2, 2, 3));
    test_cond("Trailing - is literal", FIND("[a-]+", "x-a", 2, 1, 3));
    test_cond("Leading - is literal", FIND("[-a]+", "x-a", 2, 1, 3));
    test_cond("Perl classes", FIND("\\d+\\s\\w+", "ab 12 cd_1", 2, 3, 10));
    test_cond("ASCII \\s is Python's [ \\t\\n\\r\\f\\v]", FIND("\\s+", "a\t\n\v\f\r b", 2, 1, 7));
    test_cond("Negated Perl classes", FIND("\\D\\S\\W", "1a? ", 2, 1, 4));
    test_cond("\\d is ASCII only", NOMATCH("\\d", "\xd9\xa1"));
    test_cond("\\w is ASCII only", NOMATCH("\\w", "\xc3\xa9"));
    test_cond("Perl class inside a class", FIND("[\\d_]+", "ab12_3c", 2, 2, 6));
    test_cond("Negated Perl class inside a class", FIND("[\\D]+", "12ab3", 2, 2, 4));
    test_cond("POSIX class", FIND("[[:alpha:]]+", "12ab3", 2, 2, 4));
    test_cond("Negated POSIX class", FIND("[[:^alpha:]]+", "ab12c", 2, 2, 4));
    test_cond("POSIX class mixed with a range", FIND("[[:digit:]a-c]+", "xa1bz", 2, 1, 4));
    test_cond("[[:word:]]", FIND("[[:word:]]+", "-a_1-", 2, 1, 4));
    test_cond("Escapes", FIND("\\t\\n\\r\\f\\v\\a", "\t\n\r\f\v\a", 2, 0, 6));
    test_cond("Hex escape", FIND("\\x41\\x{42}", "AB", 2, 0, 2));
    test_cond("Wide hex escape", FIND("\\x{4F60}", "\xe4\xbd\xa0", 2, 0, 3));
    test_cond("Octal escape", FIND("\\101\\012", "A\n", 2, 0, 2));
    test_cond("Escaped punctuation",
              FIND("\\.\\*\\+\\?\\(\\)\\[\\]\\{\\}\\|\\^\\$\\\\", ".*+?()[]{}|^$\\", 2, 0, 14));
    test_cond("\\Q...\\E", FIND("\\Qa.b*\\E+", "a.b**", 2, 0, 5));
    test_cond("\\Q without \\E runs to the end", FIND("\\Qa.b", "xa.b", 2, 1, 4));
    test_cond("Escape inside a class", FIND("[\\]\\-\\\\]+", "a]-\\b", 2, 1, 4));
    test_cond("Class range with escapes", FIND("[\\x41-\\x43]+", "xABCD", 2, 1, 4));

    /* ---------- flags ---------- */
    test_cond("(?i)", FIND("(?i)abc", "xABC", 2, 1, 4));
    test_cond("I flag", expect("abc", RE_FLAG_I, "xAbC", 0, RE_MODE_SEARCH, 2, 1, 4));
    test_cond("(?i) class", FIND("(?i)[a-c]+", "xABCd", 2, 1, 4));
    test_cond("(?i) negated class", NOMATCH("(?i)[^a]", "A"));
    test_cond("(?i:a)b is scoped",
              expect("(?i:a)b", 0, "Ab", 0, RE_MODE_SEARCH, 2, 0, 2) && NOMATCH("(?i:a)b", "AB"));
    test_cond("(?i) lasts until the end of the group",
              FIND("(a(?i)b)c", "aBc", 4, 0, 3, 0, 2) && NOMATCH("(a(?i)b)c", "aBC"));
    test_cond("(?-i) turns it off",
              NOMATCH("(?i)a(?-i)b", "AB") && FIND("(?i)a(?-i)b", "Ab", 2, 0, 2));
    /* case folding follows Python: Unicode simple folding with RE_FLAG_UNICODE,
     * ASCII letters only without it (re.ASCII) */
    test_cond("(?i) folds non-ASCII letters", UFIND("(?i)\xc3\xa9", "\xc3\x89", 2, 0, 2));
    test_cond("(?i) folds through the whole orbit", UFIND("(?i)k+", "kK\xe2\x84\xaa", 2, 0, 5) &&
                                                        UFIND("(?i)s", "\xc5\xbf", 2, 0, 2) &&
                                                        UFIND("(?i)\xcf\x83", "\xcf\x82", 2, 0, 2));
    test_cond("(?i) class folds non-ASCII", UFIND("(?i)[\xce\xa3]+", "\xcf\x83\xcf\x82", 2, 0, 4));
    test_cond("ASCII (?i) folds ASCII letters only",
              FIND("(?i)k", "K", 2, 0, 1) && NOMATCH("(?i)k", "\xe2\x84\xaa") &&
                  NOMATCH("(?i)\xc3\xa9", "\xc3\x89") && NOMATCH("(?i)[a-z]", "\xe2\x84\xaa"));
    test_cond("Unicode \\w takes in the Kelvin sign, a letter",
              UFIND("\\w", "\xe2\x84\xaa", 2, 0, 3) && NOMATCH("(?i)\\w", "\xe2\x84\xaa"));
    test_cond("\\w is not folded by (?i): U+0345 stays out",
              expect("(?i)\\w", RE_FLAG_UNICODE, "\xcd\x85", 0, RE_MODE_SEARCH, 0));
    test_cond("\\w without (?i) does not", NOMATCH("\\w", "\xe2\x84\xaa"));
    test_cond("Combined flags", FIND("(?ims)^A.$", "x\na\n", 2, 2, 4));
    test_cond("(?s:.) scoped", FIND("(?s:.)a", "\na", 2, 0, 2));

    /* ---------- anchoring modes ---------- */
    test_cond("Match anchors at the start", expect("b", 0, "ab", 0, RE_MODE_MATCH, 0));
    test_cond("Match at an offset", expect("b", 0, "ab", 1, RE_MODE_MATCH, 2, 1, 2));
    test_cond("Fullmatch fails on a prefix", expect("a+", 0, "aab", 0, RE_MODE_FULLMATCH, 0));
    test_cond("Fullmatch succeeds", expect("a+", 0, "aa", 0, RE_MODE_FULLMATCH, 2, 0, 2));
    test_cond("Fullmatch backtracks into a longer alternative",
              expect("a|ab", 0, "ab", 0, RE_MODE_FULLMATCH, 2, 0, 2));
    test_cond("Fullmatch of the empty text", expect("a*", 0, "", 0, RE_MODE_FULLMATCH, 2, 0, 0));
    test_cond("Fullmatch prefers the higher-priority split",
              expect("(a*)(a*)", 0, "aa", 0, RE_MODE_FULLMATCH, 6, 0, 2, 0, 2, 2, 2));

    /* ---------- errors ---------- */
    test_cond("Nested repetition", fails("a**", RE_ERR_INVALID_REPEAT_OP));
    test_cond("Nested repetition with +", fails("a*+", RE_ERR_INVALID_REPEAT_OP));
    test_cond("Nested lazy repetition", fails("a*??", RE_ERR_INVALID_REPEAT_OP));
    test_cond("Nested repeat count", fails("a{2}{3}", RE_ERR_INVALID_REPEAT_OP));
    test_cond("Missing argument", fails("*a", RE_ERR_MISSING_REPEAT_ARG));
    test_cond("Missing argument after |", fails("a|*", RE_ERR_MISSING_REPEAT_ARG));
    test_cond("Missing argument after (", fails("(+)", RE_ERR_MISSING_REPEAT_ARG));
    test_cond("Missing argument after a flag group", fails("(?i)*", RE_ERR_MISSING_REPEAT_ARG));
    test_cond("Missing argument for a brace repeat", fails("{2}", RE_ERR_MISSING_REPEAT_ARG));
    test_cond("Missing )", fails("(a", RE_ERR_MISSING_PAREN));
    test_cond("Unexpected )", fails("a)", RE_ERR_UNEXPECTED_PAREN));
    test_cond("Missing ]", fails("[a", RE_ERR_MISSING_BRACKET));
    test_cond("Missing ] after a first ]", fails("[]", RE_ERR_MISSING_BRACKET));
    test_cond("Bad class range", fails("[z-a]", RE_ERR_INVALID_CLASS_RANGE));
    test_cond("Backreference is an invalid escape, as in Go",
              fails("(a)\\1", RE_ERR_INVALID_ESCAPE));
    test_cond("Lookahead", fails("(?=a)", RE_ERR_INVALID_PERL_OP));
    test_cond("Negative lookahead", fails("(?!a)", RE_ERR_INVALID_PERL_OP));
    test_cond("Lookbehind", fails("(?<=a)", RE_ERR_INVALID_NAMED_CAPTURE));
    test_cond("Negative lookbehind", fails("(?<!a)", RE_ERR_INVALID_NAMED_CAPTURE));
    test_cond("Atomic group", fails("(?>a)", RE_ERR_INVALID_PERL_OP));
    test_cond("Unicode class", FIND("\\pL+", "1\xc3\xa9\xe4\xbd\xa0x2", 2, 1, 7));
    test_cond("Unicode class in a set", FIND("[\\p{Greek}x]+", "a\xce\xb1x\xce\xb2", 2, 1, 6));
    test_cond("Unicode class names are canonicalized",
              FIND("\\p{greek}+\\p{Uppercase_Letter}", "\xce\xb1\xce\xb2X", 2, 0, 5) &&
                  FIND("\\p{ L-u }", "A", 2, 0, 1));
    test_cond("Unicode class errors", fails("\\p{Nope}", RE_ERR_INVALID_CLASS_RANGE) &&
                                          fails("\\p{L", RE_ERR_INVALID_CLASS_RANGE) &&
                                          fails("\\p", RE_ERR_INVALID_CLASS_RANGE) &&
                                          fails("[\\p", RE_ERR_INVALID_CLASS_RANGE) &&
                                          fails("\\p{Old_Italic}", RE_ERR_INVALID_CLASS_RANGE));
    test_cond("Python backreference by name", fails("(?P=n)", RE_ERR_INVALID_PERL_OP));
    test_cond("Repeat too large", fails("a{1001}", RE_ERR_INVALID_REPEAT_SIZE));
    test_cond("Repeat min above max", fails("a{2,1}", RE_ERR_INVALID_REPEAT_SIZE));
    test_cond("Nested repeat counts must not multiply past 1000",
              fails("(a{100}){100}", RE_ERR_INVALID_REPEAT_SIZE) &&
                  fails("(?:a{1000}){2}", RE_ERR_INVALID_REPEAT_SIZE) &&
                  !fails("(?:(?:a{10}){10}){10}", RE_ERR_INVALID_REPEAT_SIZE) &&
                  !fails("(a*){1000}", RE_ERR_INVALID_REPEAT_SIZE));
    test_cond("Duplicate group names are allowed, the first one wins", ({
                  ReRegex *q = kl_re_compile("(?P<n>a)(?P<n>b)", 16, 0, NULL);
                  int ok = q && kl_re_ngroups(q) == 2 && kl_re_group_index(q, "n", 1) == 1;
                  kl_re_free(q);
                  ok;
              }));
    test_cond("Empty group name", fails("(?P<>a)", RE_ERR_INVALID_NAMED_CAPTURE));
    test_cond("Bad group name", fails("(?P<a-b>a)", RE_ERR_INVALID_NAMED_CAPTURE));
    test_cond("Unterminated group name", fails("(?P<a", RE_ERR_INVALID_NAMED_CAPTURE));
    test_cond("Group names may start with a digit",
              !fails("(?P<1a>x)", RE_ERR_INVALID_NAMED_CAPTURE));
    test_cond("Trailing backslash", fails("a\\", RE_ERR_TRAILING_BACKSLASH));
    test_cond("Invalid escape", fails("\\y", RE_ERR_INVALID_ESCAPE));
    test_cond("\\Z is the end of the text, as in Python",
              FIND("a\\Z", "ba", 2, 1, 2) && NOMATCH("a\\Z", "a\n"));
    test_cond("Python's \\u and \\U escapes",
              FIND("\\u00e9", "\xc3\xa9", 2, 0, 2) && FIND("\\U0001F600", "\xf0\x9f\x98\x80", 2, 0, 4) &&
                  FIND("[\\u0061-\\u0063]+", "abcd", 2, 0, 3));
    test_cond("\\u needs 4 hex digits, \\U 8 and at most U+10FFFF",
              fails("\\u12", RE_ERR_INVALID_ESCAPE) && fails("\\U0001F60", RE_ERR_INVALID_ESCAPE) &&
                  fails("\\U00110000", RE_ERR_INVALID_ESCAPE));
    test_cond("Unicode \\w \\d \\s and \\b with RE_FLAG_UNICODE",
              expect("\\w+", RE_FLAG_UNICODE, "-\xc3\xa9\xe4\xbd\xa0_1-", 0, RE_MODE_SEARCH, 2, 1, 8) &&
                  expect("\\d", RE_FLAG_UNICODE, "\xd9\xa3", 0, RE_MODE_SEARCH, 2, 0, 2) &&
                  expect("\\s", RE_FLAG_UNICODE, "a\xe3\x80\x80", 0, RE_MODE_SEARCH, 2, 1, 4) &&
                  expect("\\b\\w", RE_FLAG_UNICODE, "\xc3\xa9", 0, RE_MODE_SEARCH, 2, 0, 2) &&
                  NOMATCH("\\w", "\xc3\xa9"));
    test_cond("(?a) and (?u) switch the classes",
              expect("(?a)\\w", RE_FLAG_UNICODE, "\xc3\xa9", 0, RE_MODE_SEARCH, 0) &&
                  FIND("(?u)\\w", "\xc3\xa9", 2, 0, 2) &&
                  fails("(?au)a", RE_ERR_INVALID_PERL_OP) && fails("(?-a:a)", RE_ERR_INVALID_PERL_OP));
    test_cond("Stray \\E", fails("\\E", RE_ERR_INVALID_ESCAPE));
    test_cond("Bad hex escape", fails("\\xZ", RE_ERR_INVALID_ESCAPE));
    test_cond("Hex escape above U+10FFFF", fails("\\x{110000}", RE_ERR_INVALID_ESCAPE));
    test_cond("Unknown POSIX class", fails("[[:foo:]]", RE_ERR_INVALID_CLASS_RANGE));
    test_cond("Unknown flag", fails("(?z)", RE_ERR_INVALID_PERL_OP));
    test_cond("Empty flag group is fine", FIND("(?)a", "a", 2, 0, 1));
    test_cond("Dangling -", fails("(?i-)", RE_ERR_INVALID_PERL_OP));
    test_cond("Invalid UTF-8 in the pattern", fails("a\xff", RE_ERR_INVALID_UTF8));
    {
        /* 3400 copies of a{1000} is 3.4 million instructions, past Go's 3355443 */
        size_t unit = 7, copies = 3400;
        char *big = malloc(unit * copies + 1);

        for (size_t i = 0; i < copies; i++) memcpy(big + i * unit, "a{1000}", unit);
        big[unit * copies] = '\0';
        test_cond("Expression too large", fails(big, RE_ERR_TOO_LARGE));
        big[unit * 3300] = '\0';
        test_cond("Just under the limit is fine", !fails(big, RE_ERR_TOO_LARGE));
        free(big);
    }

    {
        ReError err;
        ReRegex *p = kl_re_compile("ab(", 3, 0, &err);
        test_cond("Error position points at the open paren",
                  p == NULL && err.code == RE_ERR_MISSING_PAREN && err.pos == 2);

        char deep[RE_MAX_DEPTH * 2 + 8];
        size_t k = 0;
        for (int i = 0; i < RE_MAX_DEPTH + 1; i++) deep[k++] = '(';
        deep[k++] = 'a';
        for (int i = 0; i < RE_MAX_DEPTH + 1; i++) deep[k++] = ')';
        p = kl_re_compile(deep, k, 0, &err);
        test_cond("Nesting depth is limited", p == NULL && err.code == RE_ERR_NESTING_DEPTH);
    }
    test_cond("Every error has a message", strlen(kl_re_strerror(RE_ERR_TOO_LARGE)) > 0 &&
                                               strlen(kl_re_strerror(-RE_ERR_NOMEM)) > 0 &&
                                               strcmp(kl_re_strerror(9999), "unknown error") == 0);

    /* ---------- successive matches: Python's finditer (3.7+) ---------- */
    test_cond("find-all keeps an empty match right after a match",
              iter_spans("a*", "baaab", 4, 0, 0, 1, 4, 4, 4, 5, 5));
    test_cond("a non-empty match may start where an empty one was",
              iter_spans("a*?", "aa", 5, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2) &&
                  iter_spans("(?:)|b", "ab", 4, 0, 0, 1, 1, 1, 2, 2, 2) &&
                  iter_spans("a|", "aab", 4, 0, 1, 1, 2, 2, 2, 3, 3));
    {
        ReRegex *p = kl_re_compile("", 0, 0, NULL);
        ReIter it = RE_ITER_INIT;
        ptrdiff_t caps[2];
        int n = 0;

        while (kl_re_iter_next(p, "\xc3\xa9x", 3, &it, caps) == RE_MATCH) n++;
        test_cond("find-all steps over a multi-byte rune", n == 3);
        kl_re_free(p);

        p = kl_re_compile("\\b", 2, 0, NULL);
        it = (ReIter)RE_ITER_INIT;
        n = 0;
        while (kl_re_iter_next(p, "ab cd", 5, &it, caps) == RE_MATCH) n++;
        test_cond("find-all keeps the context before each search", n == 4);
        kl_re_free(p);
    }

    /* ---------- Go quirks ---------- */
    test_cond("A repeat after an empty \\Q\\E applies to the atom before",
              FIND("a\\Q\\E*", "aaa", 2, 0, 3));
    test_cond("A repeat after \\Q...\\E applies to its last rune",
              FIND("\\Qab\\E+", "abbb", 2, 0, 4));
    test_cond("A repeat after a flag group applies to the atom before",
              FIND("a(?i)*", "aaA", 2, 0, 2));

    /* ---------- linear time ---------- */
    /* These patterns take exponential time in a backtracking engine. */
    {
        char *as = repeat_char('a', 50000);
        char *xs = repeat_char('x', 30000);

        test_cond("(a*)*b on 50000 a's", NOMATCH("(a*)*b", as));
        test_cond("(a|aa)+$ on a's then b", NOMATCH("^(a|aa)+$b", as));
        test_cond("(x+x+)+y on 30000 x's", NOMATCH("(x+x+)+y", xs));
        test_cond("(a?){30}a{30} on 30 a's",
                  FIND("(a?){30}a{30}", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", 4, 0, 30, 0, 0));
        test_cond("Search over a long text", NOMATCH("b$", as));

        free(as);
        free(xs);
    }

    test_report();
    return 0;
}

int main(void) { return reCoreTest(); }
