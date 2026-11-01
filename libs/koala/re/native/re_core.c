/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * Linear-time regular expression engine. Nothing here knows about TValue,
 * str objects or the exception machinery; re_shim.c owns that translation.
 *
 * Three stages:
 *
 *   1. parse   pattern text -> Node tree, following Go's regexp/syntax Perl
 *              mode (RE2 syntax);
 *   2. compile Node tree -> flat program for the Pike VM;
 *   3. exec    Pike VM, one pass over the text with a list of threads that
 *              is ordered by priority, so the first match found is the
 *              leftmost-first match, exactly as in Go.
 */

#include <stdlib.h>
#include <string.h>
#include "re_core.h"
#include "re_unicode.h"
#include "utf8.h"

#define MAX_RUNE 0x10FFFF

/* Budget for the thread lists: instructions * capture slots. */
#define MAX_THREAD_SLOTS ((size_t)1 << 24)

/* The parser is recursive; its depth is bounded separately from the tree
 * height that RE_MAX_DEPTH limits, so a flat (?:(?:...)) cannot exhaust the
 * stack. */
#define MAX_PARSE_RECURSION 4000

/*---------------------------------------------------------------------------+
 |   Errors                                                                  |
 +---------------------------------------------------------------------------*/

static const char *const _messages[] = {
    [RE_OK] = "no error",
    [RE_ERR_NOMEM] = "out of memory",
    [RE_ERR_INVALID_UTF8] = "invalid UTF-8",
    [RE_ERR_TRAILING_BACKSLASH] = "trailing backslash at end of expression",
    [RE_ERR_INVALID_ESCAPE] = "invalid escape sequence",
    [RE_ERR_MISSING_BRACKET] = "missing closing ]",
    [RE_ERR_MISSING_PAREN] = "missing closing )",
    [RE_ERR_UNEXPECTED_PAREN] = "unexpected )",
    [RE_ERR_MISSING_REPEAT_ARG] = "missing argument to repetition operator",
    [RE_ERR_INVALID_REPEAT_OP] = "invalid nested repetition operator",
    [RE_ERR_INVALID_REPEAT_SIZE] = "invalid repeat count",
    [RE_ERR_INVALID_CLASS_RANGE] = "invalid character class range",
    [RE_ERR_INVALID_NAMED_CAPTURE] = "invalid named capture",
    [RE_ERR_INVALID_PERL_OP] = "invalid or unsupported Perl syntax",
    [RE_ERR_NESTING_DEPTH] = "expression nests too deeply",
    [RE_ERR_TOO_LARGE] = "expression too large",
};

const char *kl_re_strerror(int code)
{
    if (code < 0) code = -code;
    if (code >= (int)(sizeof(_messages) / sizeof(_messages[0])) || !_messages[code]) {
        return "unknown error";
    }
    return _messages[code];
}

/*---------------------------------------------------------------------------+
 |   Rune sets                                                               |
 +---------------------------------------------------------------------------*/

typedef struct {
    uint32_t lo, hi;
} Range;

typedef struct {
    Range *r;
    size_t n, cap;
} RSet;

static int rs_add(RSet *s, uint32_t lo, uint32_t hi)
{
    if (s->n == s->cap) {
        size_t cap = s->cap ? s->cap * 2 : 8;
        Range *r = realloc(s->r, cap * sizeof(Range));

        if (!r) return -1;
        s->r = r;
        s->cap = cap;
    }
    s->r[s->n].lo = lo;
    s->r[s->n].hi = hi;
    s->n++;
    return 0;
}

static int _range_cmp(const void *a, const void *b)
{
    const Range *x = a, *y = b;

    if (x->lo != y->lo) return x->lo < y->lo ? -1 : 1;
    if (x->hi != y->hi) return x->hi < y->hi ? -1 : 1;
    return 0;
}

/* Sort and merge overlapping or adjacent ranges. */
static void rs_normalize(RSet *s)
{
    size_t i, o = 0;

    if (s->n < 2) return;
    qsort(s->r, s->n, sizeof(Range), _range_cmp);

    for (i = 1; i < s->n; i++) {
        if (s->r[i].lo <= s->r[o].hi + 1) {
            if (s->r[i].hi > s->r[o].hi) s->r[o].hi = s->r[i].hi;
        } else {
            s->r[++o] = s->r[i];
        }
    }
    s->n = o + 1;
}

/* Complement within [0, MAX_RUNE]. The set must be normalized. */
static int rs_negate(RSet *s)
{
    RSet out = { 0 };
    uint32_t next = 0;
    size_t i;
    int done = 0;

    for (i = 0; i < s->n && !done; i++) {
        if (s->r[i].lo > next && rs_add(&out, next, s->r[i].lo - 1) < 0) goto nomem;
        if (s->r[i].hi >= MAX_RUNE) {
            done = 1;
        } else {
            next = s->r[i].hi + 1;
        }
    }
    if (!done && rs_add(&out, next, MAX_RUNE) < 0) goto nomem;

    free(s->r);
    *s = out;
    return 0;

nomem:
    free(out.r);
    return -1;
}

/*
 * Case folding follows Go. Every rune has an orbit under unicode.SimpleFold
 * (k, K and the Kelvin sign U+212A, for instance) and folding a class adds
 * the orbit of each member. kl_re_unicode_fold lists the runes that have an
 * orbit, sorted, each with the next rune of its orbit.
 */

/* Index of the first fold entry with r >= lo. */
static int fold_lower_bound(uint32_t lo)
{
    int a = 0, b = kl_re_unicode_nfold;

    while (a < b) {
        int mid = a + (b - a) / 2;

        if (kl_re_unicode_fold[mid].r < lo) a = mid + 1;
        else b = mid;
    }
    return a;
}

static uint32_t simple_fold(uint32_t r)
{
    int i = fold_lower_bound(r);

    if (i < kl_re_unicode_nfold && kl_re_unicode_fold[i].r == r) return kl_re_unicode_fold[i].next;
    return r;
}

/*
 * How (?i) folds, as in Python: by Unicode simple folding for a Unicode
 * pattern, and only between the ASCII letters under re.ASCII.
 */
enum { FOLD_NONE, FOLD_ASCII, FOLD_UNICODE };

static int fold_mode(uint32_t flags)
{
    if (!(flags & RE_FLAG_I)) return FOLD_NONE;
    return (flags & RE_FLAG_UNICODE) ? FOLD_UNICODE : FOLD_ASCII;
}

/* The ASCII letters in lo..hi, with their case swapped. */
static int rs_add_ascii_folded(RSet *s, uint32_t lo, uint32_t hi)
{
    for (uint32_t a = 'A'; a <= 'Z'; a++) {
        if (a >= lo && a <= hi && rs_add(s, a + 32, a + 32) < 0) return -1;
        if (a + 32 >= lo && a + 32 <= hi && rs_add(s, a, a) < 0) return -1;
    }
    return 0;
}

/* Add lo..hi and, by `fold`, everything that folds onto a member. */
static int rs_add_range_folded(RSet *s, uint32_t lo, uint32_t hi, int fold)
{
    if (rs_add(s, lo, hi) < 0) return -1;
    if (fold == FOLD_NONE) return 0;
    if (fold == FOLD_ASCII) return rs_add_ascii_folded(s, lo, hi);

    for (int i = fold_lower_bound(lo); i < kl_re_unicode_nfold && kl_re_unicode_fold[i].r <= hi;
         i++) {
        uint32_t r = kl_re_unicode_fold[i].r;

        for (uint32_t f = simple_fold(r); f != r; f = simple_fold(f)) {
            if (rs_add(s, f, f) < 0) return -1;
        }
    }
    return 0;
}

/*
 * Add the normalized set `t` to `s`: folded first by `fold`, then negated.
 * `t` is consumed.
 */
static int rs_add_set(RSet *s, RSet *t, int negate, int fold)
{
    int rc = 0;

    if (fold != FOLD_NONE) {
        RSet f = { 0 };

        for (size_t i = 0; !rc && i < t->n; i++) {
            rc = rs_add_range_folded(&f, t->r[i].lo, t->r[i].hi, fold);
        }
        free(t->r);
        *t = f;
        if (!rc) rs_normalize(t);
    }
    if (!rc && negate) rc = rs_negate(t);
    for (size_t i = 0; !rc && i < t->n; i++) rc = rs_add(s, t->r[i].lo, t->r[i].hi);

    free(t->r);
    t->r = NULL;
    return rc;
}

static const ReUName *unicode_lookup(const char *name, size_t len);
static int rs_add_table(RSet *t, const ReURange *r, int n);

/* Python's \s for a str pattern: str.isspace(). */
static const uint32_t _py_space[] = {
    0x1c,   0x1d,   0x1e,   0x1f,   0x85,   0xa0,   0x1680, 0x2000, 0x2001, 0x2002,
    0x2003, 0x2004, 0x2005, 0x2006, 0x2007, 0x2008, 0x2009, 0x200a, 0x2028, 0x2029,
    0x202f, 0x205f, 0x3000,
};

/* \d \w \s by Unicode, as Python's re does for a str pattern: \d is Nd, \w is
 * L, N and '_', \s is \t-\r, space and _py_space. */
static int rs_add_unicode_perl(RSet *t, int c)
{
    const ReUName *u;

    switch (c) {
        case 'd':
            u = unicode_lookup("Nd", 2);
            return u ? rs_add_table(t, u->ranges, u->nranges) : -1;
        case 's':
            if (rs_add(t, '\t', '\r') < 0 || rs_add(t, ' ', ' ') < 0) return -1;
            for (size_t i = 0; i < sizeof(_py_space) / sizeof(_py_space[0]); i++) {
                if (rs_add(t, _py_space[i], _py_space[i]) < 0) return -1;
            }
            return 0;
        default: /* w */
            u = unicode_lookup("L", 1);
            if (!u || rs_add_table(t, u->ranges, u->nranges) < 0) return -1;
            u = unicode_lookup("N", 1);
            if (!u || rs_add_table(t, u->ranges, u->nranges) < 0) return -1;
            return rs_add(t, '_', '_');
    }
}

/* Perl classes: ASCII only unless `unicode`. As in Python, (?i) does not
 * change them. */
static int rs_add_perl(RSet *s, int c, int negate, int unicode)
{
    RSet t = { 0 };
    int rc = 0;

    if (unicode) {
        rc = rs_add_unicode_perl(&t, c);
        if (rc) {
            free(t.r);
            return -1;
        }
        rs_normalize(&t);
        return rs_add_set(s, &t, negate, FOLD_NONE);
    }

    switch (c) {
        case 'd':
            rc |= rs_add(&t, '0', '9');
            break;
        case 's':
            /* Python's [ \t\n\r\f\v]; Go leaves out \v */
            rc |= rs_add(&t, '\t', '\r');
            rc |= rs_add(&t, ' ', ' ');
            break;
        default: /* w */
            rc |= rs_add(&t, '0', '9');
            rc |= rs_add(&t, 'A', 'Z');
            rc |= rs_add(&t, '_', '_');
            rc |= rs_add(&t, 'a', 'z');
            break;
    }
    if (rc) {
        free(t.r);
        return -1;
    }

    rs_normalize(&t);
    return rs_add_set(s, &t, negate, FOLD_NONE);
}

typedef struct {
    const char *name;
    int n;
    unsigned char r[10]; /* lo, hi pairs */
} Posix;

static const Posix _posix[] = {
    { "alnum", 3, { '0', '9', 'A', 'Z', 'a', 'z' } },
    { "alpha", 2, { 'A', 'Z', 'a', 'z' } },
    { "ascii", 1, { 0, 0x7F } },
    { "blank", 2, { '\t', '\t', ' ', ' ' } },
    { "cntrl", 2, { 0, 0x1F, 0x7F, 0x7F } },
    { "digit", 1, { '0', '9' } },
    { "graph", 1, { '!', '~' } },
    { "lower", 1, { 'a', 'z' } },
    { "print", 1, { ' ', '~' } },
    { "punct", 4, { '!', '/', ':', '@', '[', '`', '{', '~' } },
    { "space", 2, { '\t', '\r', ' ', ' ' } },
    { "upper", 1, { 'A', 'Z' } },
    { "word", 4, { '0', '9', 'A', 'Z', '_', '_', 'a', 'z' } },
    { "xdigit", 3, { '0', '9', 'A', 'F', 'a', 'f' } },
};

static int rs_add_posix(RSet *s, const Posix *px, int negate, int fold)
{
    RSet t = { 0 };

    for (int i = 0; i < px->n; i++) {
        if (rs_add(&t, px->r[2 * i], px->r[2 * i + 1]) < 0) {
            free(t.r);
            return -1;
        }
    }

    rs_normalize(&t);
    return rs_add_set(s, &t, negate, fold);
}

/*
 * \p{name}: the Unicode tables come from re_unicode.c. Name lookup follows
 * regexp/syntax, including its canonicalName: '_', '-' and ' ' are dropped,
 * the first letter is upper-cased and the others lower-cased, so \p{greek},
 * \p{GREEK} and \p{Uppercase_Letter} all work.
 */
static int canonical_name(const char *s, size_t n, char *out, size_t cap)
{
    size_t o = 0;
    int first = 1;

    for (size_t i = 0; i < n; i++) {
        int c = (unsigned char)s[i];

        if (c == '_' || c == '-' || c == ' ') continue;
        if (first) {
            if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
            first = 0;
        } else if (c >= 'A' && c <= 'Z') {
            c += 'a' - 'A';
        }
        if (o + 1 >= cap) return -1;
        out[o++] = (char)c;
    }
    out[o] = '\0';
    return 0;
}

static const ReUName *unicode_lookup(const char *name, size_t len)
{
    char key[64];
    int a = 0, b = kl_re_unicode_nnames;

    if (canonical_name(name, len, key, sizeof(key)) < 0) return NULL;
    while (a < b) {
        int mid = a + (b - a) / 2, c = strcmp(kl_re_unicode_names[mid].name, key);

        if (c == 0) return &kl_re_unicode_names[mid];
        if (c < 0) a = mid + 1;
        else b = mid;
    }
    return NULL;
}

static int rs_add_table(RSet *t, const ReURange *r, int n)
{
    for (int i = 0; i < n; i++) {
        if (r[i].stride <= 1) {
            if (rs_add(t, r[i].lo, r[i].hi) < 0) return -1;
        } else {
            for (uint32_t c = r[i].lo; c <= r[i].hi; c += r[i].stride) {
                if (rs_add(t, c, c) < 0) return -1;
            }
        }
    }
    return 0;
}

/* The class of `u`, with the runes that fold into it by `fold`. */
static int rs_add_unicode(RSet *s, const ReUName *u, int negate, int fold)
{
    RSet t = { 0 };

    if (rs_add_table(&t, u->ranges, u->nranges) < 0 ||
        (fold == FOLD_UNICODE && u->fold && rs_add_table(&t, u->fold, u->nfold) < 0)) {
        free(t.r);
        return -1;
    }

    rs_normalize(&t);
    return rs_add_set(s, &t, negate ^ u->negate, fold == FOLD_ASCII ? FOLD_ASCII : FOLD_NONE);
}

/*---------------------------------------------------------------------------+
 |   Program                                                                 |
 +---------------------------------------------------------------------------*/

enum {
    I_MATCH,
    I_RUNE,      /* x = rune */
    I_CLASS,     /* x = class index */
    I_ANY,       /* any rune but \n */
    I_ANYNL,     /* any rune */
    I_SPLIT,     /* try x first, then y */
    I_JMP,       /* x */
    I_SAVE,      /* capture slot x */
    I_ASSERT,    /* empty-width assertion, x = A_* */
};

enum {
    A_BOL,    /* \A, ^ */
    A_EOL,    /* \z, $ */
    A_BOL_M,  /* ^ in multi-line mode */
    A_EOL_M,  /* $ in multi-line mode */
    A_WORD_B, /* \b */
    A_NWORD_B, /* \B */
    A_WORD_B_U, /* \b by Unicode \w */
    A_NWORD_B_U /* \B by Unicode \w */
};

typedef struct {
    int op;
    int x, y;
} Inst;

typedef struct {
    Range *r;
    int n;
} Class;

struct ReRegex {
    Inst *inst;
    int ninst, cap_inst;
    Class *cls;
    int ncls, cap_cls;
    int ngroups;
    char **names; /* [0 .. ngroups], NULL for unnamed groups */
    int cap_names;
};

void kl_re_free(ReRegex *re)
{
    int i;

    if (!re) return;
    for (i = 0; i < re->ncls; i++) free(re->cls[i].r);
    for (i = 0; i <= re->ngroups && i < re->cap_names; i++) free(re->names[i]);
    free(re->cls);
    free(re->names);
    free(re->inst);
    free(re);
}

int kl_re_ngroups(const ReRegex *re) { return re->ngroups; }

const char *kl_re_group_name(const ReRegex *re, int i)
{
    if (i < 1 || i > re->ngroups) return NULL;
    return re->names[i];
}

int kl_re_group_index(const ReRegex *re, const char *name, size_t len)
{
    for (int i = 1; i <= re->ngroups; i++) {
        const char *nm = re->names[i];

        if (nm && strlen(nm) == len && memcmp(nm, name, len) == 0) return i;
    }
    return -1;
}

/*---------------------------------------------------------------------------+
 |   Parser                                                                  |
 +---------------------------------------------------------------------------*/

enum {
    N_EMPTY,
    N_LIT,    /* r, fold */
    N_ANY,    /* . without s */
    N_ANYNL,  /* . with s */
    N_CLASS,  /* arg = class index */
    N_CAT,    /* children: sub -> next -> ... */
    N_ALT,    /* children: sub -> next -> ... */
    N_REPEAT, /* sub, min, max (-1: unbounded), greedy */
    N_CAP,    /* sub, arg = group index */
    N_ASSERT, /* arg = A_* */
};

typedef struct Node Node;
struct Node {
    int op;
    int arg;
    int min, max;
    uint8_t greedy;
    uint8_t counted; /* N_REPEAT written as {n,m}; Go limits these when nested */
    uint8_t lit;     /* a literal rune, or the class a folded literal becomes */
    int64_t size;    /* Go's estimate of the program size, 0 until computed */
    uint32_t r;
    Node *sub;
    Node *next;
    Node *all; /* every node, for freeing */
};

typedef struct {
    const char *s;
    size_t n, i;
    int depth;
    ReRegex *re;
    Node *all;
    ReError err;
} Parser;

#define FAIL(p, c, at)               \
    do {                             \
        if (!(p)->err.code) {        \
            (p)->err.code = (c);     \
            (p)->err.pos = (at);     \
        }                            \
        return NULL;                 \
    } while (0)

static Node *node_new(Parser *p, int op)
{
    Node *nd = calloc(1, sizeof(Node));

    if (!nd) FAIL(p, RE_ERR_NOMEM, p->i);
    nd->op = op;
    nd->all = p->all;
    p->all = nd;
    return nd;
}

static void nodes_free(Node *all)
{
    while (all) {
        Node *next = all->all;

        free(all);
        all = next;
    }
}

static inline int _is_alnum(int c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static inline int _is_digit(int c) { return c >= '0' && c <= '9'; }

static int _hex_value(int c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static Node *parse_alt(Parser *p, uint32_t *flags);
static int check_size(Parser *p, Node *nd);

static Node *class_node(Parser *p, RSet *set, int negate);

/* The literal for rune r; under the I flag a rune with a case orbit becomes
 * a class of the whole orbit. */
static Node *make_lit(Parser *p, uint32_t r, uint32_t flags)
{
    Node *nd;

    int fold = fold_mode(flags);
    int ascii_letter = (r | 0x20) >= 'a' && (r | 0x20) <= 'z';

    if ((fold == FOLD_UNICODE && simple_fold(r) != r) || (fold == FOLD_ASCII && ascii_letter)) {
        RSet set = { 0 };
        uint32_t f = r;

        do {
            if (rs_add(&set, f, f) < 0) {
                free(set.r);
                FAIL(p, RE_ERR_NOMEM, p->i);
            }
            f = fold == FOLD_ASCII ? (f ^ 0x20) : simple_fold(f);
        } while (f != r);
        nd = class_node(p, &set, 0);
        if (nd) nd->lit = 1;
        return nd;
    }

    nd = node_new(p, N_LIT);
    if (!nd) return NULL;
    nd->r = r;
    nd->lit = 1;
    return nd;
}

/* Decode one pattern rune at p->i; an invalid byte is an error. */
static int next_rune(Parser *p, uint32_t *r)
{
    int w = utf8_decode(p->s + p->i, p->n - p->i, r);

    if (w == 1 && *r == UTF8_RUNE_ERROR) {
        if (!p->err.code) {
            p->err.code = RE_ERR_INVALID_UTF8;
            p->err.pos = p->i;
        }
        return -1;
    }
    p->i += (size_t)w;
    return 0;
}

/*
 * A single-character escape, with p->i on the backslash. Used for literals
 * outside and inside classes. Returns -1 on error.
 */
static int parse_escape_rune(Parser *p, uint32_t *out)
{
    size_t at = p->i;
    const char *s = p->s;
    size_t n = p->n;
    int c;

    p->i++; /* the backslash */
    if (p->i >= n) {
        p->err.code = RE_ERR_TRAILING_BACKSLASH;
        p->err.pos = at;
        return -1;
    }

    c = (unsigned char)s[p->i];
    switch (c) {
        case '1': case '2': case '3': case '4': case '5': case '6': case '7':
            /* a single non-zero digit would be a backreference: not supported */
            if (p->i + 1 >= n || s[p->i + 1] < '0' || s[p->i + 1] > '7') goto bad;
            /* fall through */
        case '0': {
            uint32_t v = (uint32_t)(c - '0');

            p->i++;
            for (int k = 0; k < 2 && p->i < n && s[p->i] >= '0' && s[p->i] <= '7'; k++) {
                v = v * 8 + (uint32_t)(s[p->i] - '0');
                p->i++;
            }
            *out = v;
            return 0;
        }
        case 'x': {
            uint32_t v = 0;

            p->i++;
            if (p->i < n && s[p->i] == '{') {
                int digits = 0;

                p->i++;
                while (p->i < n && _hex_value((unsigned char)s[p->i]) >= 0) {
                    v = v * 16 + (uint32_t)_hex_value((unsigned char)s[p->i]);
                    if (v > MAX_RUNE) goto bad;
                    digits++;
                    p->i++;
                }
                if (digits == 0 || p->i >= n || s[p->i] != '}') goto bad;
                p->i++;
            } else {
                if (p->i + 1 >= n || _hex_value((unsigned char)s[p->i]) < 0 ||
                    _hex_value((unsigned char)s[p->i + 1]) < 0) {
                    goto bad;
                }
                v = (uint32_t)(_hex_value((unsigned char)s[p->i]) * 16 +
                               _hex_value((unsigned char)s[p->i + 1]));
                p->i += 2;
            }
            *out = v;
            return 0;
        }
        case 'u':
        case 'U': {
            /* Python's \uXXXX and \UXXXXXXXX: exactly 4 or 8 hex digits */
            int want = c == 'u' ? 4 : 8;
            uint32_t v = 0;

            p->i++;
            for (int k = 0; k < want; k++) {
                if (p->i >= n || _hex_value((unsigned char)s[p->i]) < 0) goto bad;
                v = v * 16 + (uint32_t)_hex_value((unsigned char)s[p->i]);
                p->i++;
            }
            if (v > MAX_RUNE) goto bad;
            *out = v;
            return 0;
        }
        case 'a': *out = 7; p->i++; return 0;
        case 'f': *out = 12; p->i++; return 0;
        case 'n': *out = 10; p->i++; return 0;
        case 'r': *out = 13; p->i++; return 0;
        case 't': *out = 9; p->i++; return 0;
        case 'v': *out = 11; p->i++; return 0;
        default:
            if (c < 0x80 && !_is_alnum(c)) {
                *out = (uint32_t)c;
                p->i++;
                return 0;
            }
    }

bad:
    p->err.code = RE_ERR_INVALID_ESCAPE;
    p->err.pos = at;
    return -1;
}

/* A single character inside [...]: a literal rune or an escape. */
static int parse_class_char(Parser *p, uint32_t *out)
{
    if (p->s[p->i] == '\\') return parse_escape_rune(p, out);
    return next_rune(p, out);
}

/* Register a finished set as a class of the program; returns its index. */
static int class_register(Parser *p, RSet *set)
{
    ReRegex *re = p->re;
    Class *c;

    if (re->ncls == re->cap_cls) {
        int cap = re->cap_cls ? re->cap_cls * 2 : 8;
        Class *nc = realloc(re->cls, (size_t)cap * sizeof(Class));

        if (!nc) return -1;
        re->cls = nc;
        re->cap_cls = cap;
    }

    c = &re->cls[re->ncls];
    c->r = set->r;
    c->n = (int)set->n;
    set->r = NULL;
    return re->ncls++;
}

static Node *class_node(Parser *p, RSet *set, int negate)
{
    Node *nd;
    int idx;

    rs_normalize(set);
    if (negate && rs_negate(set) < 0) goto nomem;

    nd = node_new(p, N_CLASS);
    if (!nd) goto fail;
    idx = class_register(p, set);
    if (idx < 0) goto nomem;
    nd->arg = idx;
    free(set->r);
    return nd;

nomem:
    if (!p->err.code) p->err.code = RE_ERR_NOMEM;
fail:
    free(set->r);
    return NULL;
}

static int parse_unicode_class(Parser *p, RSet *set, uint32_t flags);

/* [...] with p->i on the '['. */
static Node *parse_class(Parser *p, uint32_t flags)
{
    size_t at = p->i;
    const char *s = p->s;
    RSet set = { 0 };
    int negate = 0, first = 1;

    p->i++;
    if (p->i < p->n && s[p->i] == '^') {
        negate = 1;
        p->i++;
    }

    for (;;) {
        uint32_t lo, hi;

        if (p->i >= p->n) {
            p->err.code = RE_ERR_MISSING_BRACKET;
            p->err.pos = at;
            goto fail;
        }
        if (s[p->i] == ']' && !first) {
            p->i++;
            break;
        }
        first = 0;

        /* [:alpha:] and [:^alpha:] */
        if (s[p->i] == '[' && p->i + 1 < p->n && s[p->i + 1] == ':') {
            size_t j = p->i + 2;

            while (j + 1 < p->n && !(s[j] == ':' && s[j + 1] == ']')) j++;
            if (j + 1 < p->n) {
                size_t b = p->i + 2;
                int neg = 0;
                const Posix *px = NULL;

                if (s[b] == '^') {
                    neg = 1;
                    b++;
                }
                for (size_t k = 0; k < sizeof(_posix) / sizeof(_posix[0]); k++) {
                    if (strlen(_posix[k].name) == j - b && memcmp(_posix[k].name, s + b, j - b) == 0) {
                        px = &_posix[k];
                        break;
                    }
                }
                if (!px) {
                    p->err.code = RE_ERR_INVALID_CLASS_RANGE;
                    p->err.pos = p->i;
                    goto fail;
                }
                if (rs_add_posix(&set, px, neg, fold_mode(flags)) < 0) goto nomem;
                p->i = j + 2;
                continue;
            }
            /* no closing ":]": the '[' is an ordinary character */
        }

        if (s[p->i] == '\\' && p->i + 1 < p->n && (s[p->i + 1] == 'p' || s[p->i + 1] == 'P')) {
            if (parse_unicode_class(p, &set, flags) < 0) goto fail;
            continue;
        }

        if (s[p->i] == '\\' && p->i + 1 < p->n) {
            int e = (unsigned char)s[p->i + 1];

            if (e == 'd' || e == 's' || e == 'w' || e == 'D' || e == 'S' || e == 'W') {
                if (rs_add_perl(&set, e | 0x20, e < 'a', (flags & RE_FLAG_UNICODE) != 0) < 0) {
                    goto nomem;
                }
                p->i += 2;
                continue;
            }
        }

        if (parse_class_char(p, &lo) < 0) goto fail;
        hi = lo;
        if (p->i + 1 < p->n && s[p->i] == '-' && s[p->i + 1] != ']') {
            size_t rpos = p->i;

            p->i++;
            if (parse_class_char(p, &hi) < 0) goto fail;
            if (hi < lo) {
                p->err.code = RE_ERR_INVALID_CLASS_RANGE;
                p->err.pos = rpos;
                goto fail;
            }
        }
        if (rs_add_range_folded(&set, lo, hi, fold_mode(flags)) < 0) goto nomem;
    }

    return class_node(p, &set, negate);

nomem:
    if (!p->err.code) p->err.code = RE_ERR_NOMEM;
fail:
    free(set.r);
    return NULL;
}

/* \d \D \s \S \w \W with p->i on the backslash. */
static Node *parse_perl_class(Parser *p, int e, uint32_t flags)
{
    RSet set = { 0 };

    if (rs_add_perl(&set, e | 0x20, e < 'a', (flags & RE_FLAG_UNICODE) != 0) < 0) {
        free(set.r);
        FAIL(p, RE_ERR_NOMEM, p->i);
    }
    p->i += 2;
    return class_node(p, &set, 0);
}

/*
 * \pN, \p{Name}, \PN, \P{Name} and \p{^Name} with p->i on the backslash:
 * add the class to `set`. The rules are Go's: a name that is not found is an
 * invalid range, and the I flag adds the runes that fold into the class.
 */
static int parse_unicode_class(Parser *p, RSet *set, uint32_t flags)
{
    const char *s = p->s;
    size_t at = p->i, j = p->i + 2, name_at, name_len;
    int negate = s[at + 1] == 'P';
    const ReUName *u;
    uint32_t c = UTF8_RUNE_ERROR;
    int w = 0;

    if (j < p->n) {
        w = utf8_decode(s + j, p->n - j, &c);
        if (w == 1 && c == UTF8_RUNE_ERROR) {
            p->err.code = RE_ERR_INVALID_UTF8;
            p->err.pos = j;
            return -1;
        }
    }

    if (j >= p->n || c != '{') {
        /* a one-letter name; at the end of the pattern the name is empty */
        name_at = j;
        name_len = (size_t)w;
        p->i = j + (size_t)w;
    } else {
        size_t end = j;

        while (end < p->n && s[end] != '}') end++;
        if (end >= p->n) {
            p->err.code = utf8_valid(s + at, p->n - at) ? RE_ERR_INVALID_CLASS_RANGE
                                                          : RE_ERR_INVALID_UTF8;
            p->err.pos = at;
            return -1;
        }
        name_at = j + 1;
        name_len = end - name_at;
        if (!utf8_valid(s + name_at, name_len)) {
            p->err.code = RE_ERR_INVALID_UTF8;
            p->err.pos = name_at;
            return -1;
        }
        p->i = end + 1;
    }

    /* \p{^Han} is \P{Han} */
    if (name_len > 0 && s[name_at] == '^') {
        negate = !negate;
        name_at++;
        name_len--;
    }

    u = unicode_lookup(s + name_at, name_len);
    if (!u) {
        p->err.code = RE_ERR_INVALID_CLASS_RANGE;
        p->err.pos = at;
        return -1;
    }

    if (rs_add_unicode(set, u, negate, fold_mode(flags)) < 0) {
        p->err.code = RE_ERR_NOMEM;
        p->err.pos = at;
        return -1;
    }
    return 0;
}

/* The \p class as an atom of its own. */
static Node *parse_unicode_atom(Parser *p, uint32_t flags)
{
    RSet set = { 0 };

    if (parse_unicode_class(p, &set, flags) < 0) {
        free(set.r);
        return NULL;
    }
    return class_node(p, &set, 0);
}

static Node *make_assert(Parser *p, int kind)
{
    Node *nd = node_new(p, N_ASSERT);

    if (nd) nd->arg = kind;
    return nd;
}

/* Chain `b` after `a` in a CAT/ALT child list; returns the new tail. */
static Node *list_append(Node *tail, Node *nd)
{
    tail->next = nd;
    return nd;
}

/* \Q...\E with p->i on the backslash. */
static Node *parse_quoted(Parser *p, uint32_t flags)
{
    Node *cat = node_new(p, N_CAT), *tail = NULL;

    if (!cat) return NULL;
    p->i += 2;

    while (p->i < p->n) {
        uint32_t r;
        Node *lit;

        if (p->s[p->i] == '\\' && p->i + 1 < p->n && p->s[p->i + 1] == 'E') {
            p->i += 2;
            break;
        }
        if (next_rune(p, &r) < 0) return NULL;
        lit = make_lit(p, r, flags);
        if (!lit) return NULL;
        if (!tail) {
            cat->sub = lit;
        } else {
            list_append(tail, lit);
        }
        tail = lit;
    }
    return cat;
}

/*
 * {n}, {n,} or {n,m} with p->i on the '{'. Returns 1 and advances when it
 * is a well-formed repeat, 0 (nothing consumed) when the brace is an
 * ordinary character, -1 on a bad count.
 */
static int parse_braces(Parser *p, int *min, int *max)
{
    const char *s = p->s;
    size_t i = p->i + 1, at = p->i;
    long lo = 0, hi = -1;
    int big = 0, ndig = 0;

    while (i < p->n && _is_digit(s[i])) {
        lo = lo * 10 + (s[i] - '0');
        if (lo > 100000) big = lo = 100000;
        ndig++;
        i++;
    }
    if (ndig == 0) return 0;

    if (i < p->n && s[i] == ',') {
        i++;
        if (i < p->n && s[i] == '}') {
            hi = -1;
        } else {
            long v = 0;
            int nd = 0;

            while (i < p->n && _is_digit(s[i])) {
                v = v * 10 + (s[i] - '0');
                if (v > 100000) big = v = 100000;
                nd++;
                i++;
            }
            if (nd == 0) return 0;
            hi = v;
        }
    } else {
        hi = lo;
    }
    if (i >= p->n || s[i] != '}') return 0;

    if (big || lo > RE_MAX_REPEAT || hi > RE_MAX_REPEAT || (hi >= 0 && lo > hi)) {
        p->err.code = RE_ERR_INVALID_REPEAT_SIZE;
        p->err.pos = at;
        return -1;
    }

    *min = (int)lo;
    *max = (int)hi;
    p->i = i + 1;
    return 1;
}

static int _valid_name(const char *s, size_t n)
{
    if (n == 0) return 0;
    for (size_t i = 0; i < n; i++) {
        if (!_is_alnum((unsigned char)s[i]) && s[i] != '_') return 0;
    }
    return 1;
}

/* The group names are stored as they are opened. */
static int groups_add(Parser *p, const char *name, size_t len, int *index)
{
    ReRegex *re = p->re;
    char *copy = NULL;

    if (name) {
        copy = malloc(len + 1);
        if (!copy) return -1;
        memcpy(copy, name, len);
        copy[len] = '\0';
    }

    if (re->ngroups + 1 >= re->cap_names) {
        int cap = re->cap_names ? re->cap_names * 2 : 8;
        char **nn = realloc(re->names, (size_t)cap * sizeof(char *));

        if (!nn) {
            free(copy);
            return -1;
        }
        re->names = nn;
        memset(nn + re->cap_names, 0, (size_t)(cap - re->cap_names) * sizeof(char *));
        re->cap_names = cap;
    }

    re->ngroups++;
    re->names[re->ngroups] = copy;
    *index = re->ngroups;
    return 0;
}

/* After a group is parsed: expect the closing paren. */
static int close_paren(Parser *p, size_t open)
{
    if (p->i >= p->n || p->s[p->i] != ')') {
        p->err.code = RE_ERR_MISSING_PAREN;
        p->err.pos = open;
        return -1;
    }
    p->i++;
    return 0;
}

/*
 * A group with p->i on the '('. Returns NULL without an error for a bare
 * flag group such as (?i), which only changes *flags.
 */
static Node *parse_group(Parser *p, uint32_t *flags)
{
    size_t open = p->i;
    uint32_t local = *flags;
    const char *s = p->s;
    int capture = 1, index = 0;
    const char *name = NULL;
    size_t name_len = 0;
    Node *body, *nd;

    p->i++;
    if (p->i < p->n && s[p->i] == '?') {
        size_t q = p->i + 1;

        size_t rest = p->n - open; /* Go's len(t), with t starting at the paren */
        int starts_p = rest > 4 && s[q] == 'P' && s[q + 1] == '<';
        int starts_name = rest > 3 && s[q] == '<';

        if (starts_p || starts_name) {
            /* (?P<name>re) or (?<name>re); a (?<= or (?<! lookbehind ends up
             * here too and fails on its name, as in Go */
            size_t b = open + (starts_name ? 3 : 4), e = open;

            while (e < p->n && s[e] != '>') e++;
            if (e >= p->n) {
                p->err.code = utf8_valid(s + open, p->n - open) ? RE_ERR_INVALID_NAMED_CAPTURE
                                                                : RE_ERR_INVALID_UTF8;
                p->err.pos = open;
                return NULL;
            }
            if (!utf8_valid(s + b, e - b)) FAIL(p, RE_ERR_INVALID_UTF8, b);
            if (!_valid_name(s + b, e - b)) FAIL(p, RE_ERR_INVALID_NAMED_CAPTURE, open);
            name = s + b;
            name_len = e - b;
            p->i = e + 1;
        } else {
            /* (?:re), (?flags) or (?flags:re); lookahead, atomic groups and
             * comments are not flags and fail here */
            int neg = 0, seen = 0, charset = 0;

            p->i = q;
            for (;; p->i++) {
                int c = p->i < p->n ? (unsigned char)s[p->i] : -1;
                uint32_t bit = 0;

                if (c == 'i') bit = RE_FLAG_I;
                else if (c == 'm') bit = RE_FLAG_M;
                else if (c == 's') bit = RE_FLAG_S;
                else if (c == 'U') bit = RE_FLAG_U;

                if (c == 'a' || c == 'u') {
                    /* Python's (?a) and (?u): ASCII or Unicode classes; only one
                     * of them, and neither can be turned off */
                    if (neg || charset) FAIL(p, RE_ERR_INVALID_PERL_OP, open);
                    charset = 1;
                    if (c == 'a') local &= ~(uint32_t)RE_FLAG_UNICODE;
                    else local |= RE_FLAG_UNICODE;
                    seen = 1;
                } else if (bit) {
                    if (neg) local &= ~bit;
                    else local |= bit;
                    seen = 1;
                } else if (c == '-') {
                    if (neg) FAIL(p, RE_ERR_INVALID_PERL_OP, open);
                    neg = 1;
                    seen = 0;
                } else if (c == ':' || c == ')') {
                    if (neg && !seen) FAIL(p, RE_ERR_INVALID_PERL_OP, open);
                    p->i++;
                    if (c == ')') {
                        *flags = local;
                        return NULL;
                    }
                    capture = 0;
                    break;
                } else {
                    FAIL(p, RE_ERR_INVALID_PERL_OP, open);
                }
            }
        }
    }

    if (++p->depth > MAX_PARSE_RECURSION) FAIL(p, RE_ERR_NESTING_DEPTH, open);
    if (capture && groups_add(p, name, name_len, &index) < 0) {
        if (!p->err.code) p->err.code = RE_ERR_NOMEM;
        return NULL;
    }

    body = parse_alt(p, &local);
    if (!body) return NULL;
    if (close_paren(p, open) < 0) return NULL;
    p->depth--;

    if (!capture) return body;
    nd = node_new(p, N_CAP);
    if (!nd) return NULL;
    nd->sub = body;
    nd->arg = index;
    if (check_size(p, nd) < 0) return NULL;
    return nd;
}

static Node *parse_atom(Parser *p, uint32_t *flags)
{
    const char *s = p->s;
    int c = (unsigned char)s[p->i];
    uint32_t r;
    int dummy_min, dummy_max;

    switch (c) {
        case '(':
            return parse_group(p, flags);
        case '[':
            return parse_class(p, *flags);
        case '.': {
            Node *nd = node_new(p, (*flags & RE_FLAG_S) ? N_ANYNL : N_ANY);

            p->i++;
            return nd;
        }
        case '^':
            p->i++;
            return make_assert(p, (*flags & RE_FLAG_M) ? A_BOL_M : A_BOL);
        case '$':
            p->i++;
            return make_assert(p, (*flags & RE_FLAG_M) ? A_EOL_M : A_EOL);
        case '*':
        case '+':
        case '?':
            FAIL(p, RE_ERR_MISSING_REPEAT_ARG, p->i);
        case '{': {
            int rc = parse_braces(p, &dummy_min, &dummy_max);

            if (rc < 0) return NULL;
            if (rc > 0) FAIL(p, RE_ERR_MISSING_REPEAT_ARG, p->i);
            p->i++;
            return make_lit(p, '{', *flags);
        }
        case '\\': {
            int e = p->i + 1 < p->n ? (unsigned char)s[p->i + 1] : -1;

            switch (e) {
                case 'd': case 'D': case 's': case 'S': case 'w': case 'W':
                    return parse_perl_class(p, e, *flags);
                case 'A':
                    p->i += 2;
                    return make_assert(p, A_BOL);
                case 'z':
                case 'Z': /* Python's \Z: the end of the text, as \z */
                    p->i += 2;
                    return make_assert(p, A_EOL);
                case 'b':
                    p->i += 2;
                    return make_assert(p, (*flags & RE_FLAG_UNICODE) ? A_WORD_B_U : A_WORD_B);
                case 'B':
                    p->i += 2;
                    return make_assert(p, (*flags & RE_FLAG_UNICODE) ? A_NWORD_B_U : A_NWORD_B);
                case 'p':
                case 'P':
                    return parse_unicode_atom(p, *flags);
                default:
                    break;
            }
            if (parse_escape_rune(p, &r) < 0) return NULL;
            return make_lit(p, r, *flags);
        }
        default:
            if (next_rune(p, &r) < 0) return NULL;
            return make_lit(p, r, *flags);
    }
}

/*
 * Go's estimate of the size of the program for a tree (calcSize in
 * regexp/syntax), used to reject expressions that are too large at the same
 * point Go does. A repeat {n,m} is counted as m copies of its operand.
 */
#define MAX_SIZE ((int64_t)(128 << 20) / 40)

static int64_t node_size(Node *nd)
{
    int64_t size = 0;

    if (nd->size) return nd->size;

    switch (nd->op) {
        case N_CAP:
            size = 2 + node_size(nd->sub);
            break;
        case N_CAT:
            for (Node *k = nd->sub; k; k = k->next) size += node_size(k);
            break;
        case N_ALT: {
            /* Go merges a run of single-character branches into one class
             * before it measures the alternation */
            int outs = 0, run = 0;

            for (Node *k = nd->sub; k; k = k->next) {
                Node *e = (k->op == N_CAT && k->sub && !k->sub->next) ? k->sub : k;

                if (e->op == N_LIT || e->op == N_CLASS || e->op == N_ANY || e->op == N_ANYNL) {
                    run++;
                    continue;
                }
                if (run) {
                    size += 1;
                    outs++;
                    run = 0;
                }
                size += node_size(k);
                outs++;
            }
            if (run) {
                size += 1;
                outs++;
            }
            if (outs > 1) size += outs - 1;
            break;
        }
        case N_REPEAT: {
            int64_t sub = node_size(nd->sub);

            if (nd->max < 0) {
                size = nd->min == 0 ? 2 + sub : 1 + (int64_t)nd->min * sub;
            } else if (!nd->counted && nd->min == 0 && nd->max == 1) {
                size = 1 + sub; /* ? */
            } else {
                size = (int64_t)nd->max * sub + (nd->max - nd->min);
            }
            break;
        }
        default:
            break;
    }

    if (size < 1) size = 1;
    nd->size = size;
    return size;
}

static int check_size(Parser *p, Node *nd)
{
    if (node_size(nd) > MAX_SIZE) {
        p->err.code = RE_ERR_TOO_LARGE;
        p->err.pos = 0;
        return -1;
    }
    return 0;
}

static Node *make_repeat(Parser *p, Node *sub, int min, int max, int greedy, int counted)
{
    Node *nd = node_new(p, N_REPEAT);

    if (!nd) return NULL;
    nd->sub = sub;
    nd->min = min;
    nd->max = max;
    nd->greedy = (uint8_t)greedy;
    nd->counted = (uint8_t)counted;
    return nd;
}

/*
 * Go's repeatIsValid: the counts of {n,m} repeats nested in each other must
 * not multiply to more than n (1000), so (a{100}){100} is rejected while
 * ((a{10}){10}){10} is not. * + ? do not count.
 */
static int repeat_valid(const Node *nd, int n)
{
    if (nd->op == N_REPEAT && nd->counted) {
        int m = nd->max;

        if (m == 0) return 1;
        if (m < 0) m = nd->min;
        if (m > n) return 0;
        if (m > 0) n /= m;
    }

    switch (nd->op) {
        case N_CAT:
        case N_ALT:
            for (const Node *k = nd->sub; k; k = k->next) {
                if (!repeat_valid(k, n)) return 0;
            }
            return 1;
        case N_REPEAT:
        case N_CAP:
            return repeat_valid(nd->sub, n);
        default:
            return 1;
    }
}

/* Is there a repetition operator at p->i? Consumes it when asked to. */
static int parse_quantifier(Parser *p, int *min, int *max, int *counted)
{
    int c = p->i < p->n ? (unsigned char)p->s[p->i] : -1;

    *counted = 0;
    switch (c) {
        case '*': *min = 0; *max = -1; p->i++; return 1;
        case '+': *min = 1; *max = -1; p->i++; return 1;
        case '?': *min = 0; *max = 1; p->i++; return 1;
        case '{': *counted = 1; return parse_braces(p, min, max);
        default: return 0;
    }
}

static Node *parse_cat(Parser *p, uint32_t *flags)
{
    Node *cat = node_new(p, N_CAT), *prev = NULL, *tail = NULL;
    int last_repeat = 0;

    if (!cat) return NULL;

#define CAT_APPEND(nd)                    \
    do {                                  \
        if (tail) tail->next = (nd);      \
        else cat->sub = (nd);             \
        prev = tail;                      \
        tail = (nd);                      \
    } while (0)

    while (p->i < p->n && p->s[p->i] != '|' && p->s[p->i] != ')') {
        size_t at = p->i;
        int min, max, rc, counted;
        Node *atom;

        /*
         * Like Go, a repetition operator applies to the last atom, even when
         * an empty \Q\E or a flag group such as (?i) sits in between; only
         * an operator right after another operator is nested.
         */
        rc = parse_quantifier(p, &min, &max, &counted);
        if (rc < 0) return NULL;
        if (rc > 0) {
            int lazy = 0;
            Node *rep;

            if (p->i < p->n && p->s[p->i] == '?') {
                lazy = 1;
                p->i++;
            }
            if (last_repeat) FAIL(p, RE_ERR_INVALID_REPEAT_OP, at);
            if (!tail) FAIL(p, RE_ERR_MISSING_REPEAT_ARG, at);
            rep = make_repeat(p, tail, min, max, (*flags & RE_FLAG_U) ? lazy : !lazy, counted);
            if (!rep) return NULL;
            if (check_size(p, rep) < 0) return NULL;
            if (counted && (min >= 2 || max >= 2) && !repeat_valid(rep, RE_MAX_REPEAT)) {
                FAIL(p, RE_ERR_INVALID_REPEAT_SIZE, at);
            }
            if (prev) prev->next = rep;
            else cat->sub = rep;
            tail = rep;
            last_repeat = 1;
            continue;
        }
        last_repeat = 0;

        if (p->s[p->i] == '\\' && p->i + 1 < p->n && p->s[p->i + 1] == 'Q') {
            Node *quoted = parse_quoted(p, *flags), *k, *next;

            if (!quoted) return NULL;
            for (k = quoted->sub; k; k = next) {
                next = k->next;
                k->next = NULL;
                CAT_APPEND(k);
            }
            continue;
        }

        atom = parse_atom(p, flags);
        if (!atom) {
            if (p->err.code) return NULL;
            continue; /* a bare flag group */
        }
        CAT_APPEND(atom);
    }

#undef CAT_APPEND

    if (!cat->sub) cat->op = N_EMPTY;
    if (check_size(p, cat) < 0) return NULL;
    return cat;
}

static Node *parse_alt(Parser *p, uint32_t *flags)
{
    Node *first, *alt, *tail;

    first = parse_cat(p, flags);
    if (!first) return NULL;
    if (p->i >= p->n || p->s[p->i] != '|') return first;

    alt = node_new(p, N_ALT);
    if (!alt) return NULL;
    alt->sub = first;
    tail = first;

    while (p->i < p->n && p->s[p->i] == '|') {
        Node *branch;

        p->i++;
        branch = parse_cat(p, flags);
        if (!branch) return NULL;
        tail = list_append(tail, branch);
    }
    if (check_size(p, alt) < 0) return NULL;
    return alt;
}

/*---------------------------------------------------------------------------+
 |   Compiler                                                                |
 +---------------------------------------------------------------------------*/

/*
 * Height of the tree the way Go measures it for ErrNestingDepth: a
 * non-capturing group adds nothing, a run of literals is one leaf, and a
 * concatenation of a single item is that item.
 */
static int tree_height(const Node *nd)
{
    int h = 0, items = 0, in_run = 0;

    switch (nd->op) {
        case N_CAP:
        case N_REPEAT:
            return 1 + tree_height(nd->sub);
        case N_CAT:
            for (const Node *k = nd->sub; k; k = k->next) {
                int kh = 1;

                if (k->lit && in_run) continue;
                if (!k->lit) kh = tree_height(k);
                in_run = k->lit;
                items++;
                if (kh > h) h = kh;
            }
            return items == 1 ? h : 1 + h;
        case N_ALT:
            for (const Node *k = nd->sub; k; k = k->next) {
                int kh = tree_height(k);

                if (kh > h) h = kh;
            }
            return 1 + h;
        default:
            return 1;
    }
}

typedef struct {
    ReRegex *re;
    int err;
    long steps;
} Compiler;

static int emit(Compiler *c, int op, int x, int y)
{
    ReRegex *re = c->re;

    if (c->err) return 0;
    if (re->ninst >= RE_MAX_INST) {
        c->err = RE_ERR_TOO_LARGE;
        return 0;
    }
    if (re->ninst == re->cap_inst) {
        int cap = re->cap_inst ? re->cap_inst * 2 : 64;
        Inst *ni = realloc(re->inst, (size_t)cap * sizeof(Inst));

        if (!ni) {
            c->err = RE_ERR_NOMEM;
            return 0;
        }
        re->inst = ni;
        re->cap_inst = cap;
    }

    re->inst[re->ninst].op = op;
    re->inst[re->ninst].x = x;
    re->inst[re->ninst].y = y;
    return re->ninst++;
}

static void compile_node(Compiler *c, const Node *nd);

/* Emit nd `n` times. */
static void compile_times(Compiler *c, const Node *nd, int n)
{
    for (int i = 0; i < n && !c->err; i++) compile_node(c, nd);
}

/* Patch a split at `at` that tries `body` first when greedy. */
static void set_split(Compiler *c, int at, int body, int exit, int greedy)
{
    Inst *in = &c->re->inst[at];

    in->x = greedy ? body : exit;
    in->y = greedy ? exit : body;
}

static void compile_plus(Compiler *c, const Node *sub, int greedy)
{
    int l1 = c->re->ninst, s;

    compile_node(c, sub);
    s = emit(c, I_SPLIT, 0, 0);
    set_split(c, s, l1, s + 1, greedy);
}

static int nullable(const Node *nd)
{
    switch (nd->op) {
        case N_EMPTY:
        case N_ASSERT:
            return 1;
        case N_CAT:
            for (const Node *k = nd->sub; k; k = k->next) {
                if (!nullable(k)) return 0;
            }
            return 1;
        case N_ALT:
            for (const Node *k = nd->sub; k; k = k->next) {
                if (nullable(k)) return 1;
            }
            return 0;
        case N_REPEAT:
            return nd->min == 0 || nullable(nd->sub);
        case N_CAP:
            return nullable(nd->sub);
        default:
            return 0;
    }
}

static void compile_star(Compiler *c, const Node *sub, int greedy)
{
    int l1;

    if (nullable(sub)) {
        /* (x+)? keeps the priority order right when x can match the empty
         * string, the same trick as Go (golang.org/issue/46123) */
        int q = emit(c, I_SPLIT, 0, 0);

        compile_plus(c, sub, greedy);
        set_split(c, q, q + 1, c->re->ninst, greedy);
        return;
    }

    l1 = emit(c, I_SPLIT, 0, 0);
    compile_node(c, sub);
    emit(c, I_JMP, l1, 0);
    set_split(c, l1, l1 + 1, c->re->ninst, greedy);
}

static void compile_repeat(Compiler *c, const Node *nd)
{
    const Node *sub = nd->sub;
    int min = nd->min, max = nd->max, greedy = nd->greedy;

    if (max < 0) {
        if (min == 0) {
            compile_star(c, sub, greedy);
        } else {
            compile_times(c, sub, min - 1);
            compile_plus(c, sub, greedy);
        }
        return;
    }

    compile_times(c, sub, min);
    /* x{n,m}: (x(x(x)?)?)? flattened, every split exits to the same end */
    int *splits = NULL, nsplit = max - min;

    if (nsplit <= 0) return;
    splits = malloc((size_t)nsplit * sizeof(int));
    if (!splits) {
        c->err = RE_ERR_NOMEM;
        return;
    }
    for (int i = 0; i < nsplit && !c->err; i++) {
        splits[i] = emit(c, I_SPLIT, 0, 0);
        compile_node(c, sub);
    }
    if (!c->err) {
        for (int i = 0; i < nsplit; i++) set_split(c, splits[i], splits[i] + 1, c->re->ninst, greedy);
    }
    free(splits);
}

static void compile_node(Compiler *c, const Node *nd)
{
    if (c->err) return;
    if (++c->steps > (long)RE_MAX_INST * 8) {
        c->err = RE_ERR_TOO_LARGE;
        return;
    }

    switch (nd->op) {
        case N_EMPTY:
            break;
        case N_LIT:
            emit(c, I_RUNE, (int)nd->r, 0);
            break;
        case N_ANY:
            emit(c, I_ANY, 0, 0);
            break;
        case N_ANYNL:
            emit(c, I_ANYNL, 0, 0);
            break;
        case N_CLASS:
            emit(c, I_CLASS, nd->arg, 0);
            break;
        case N_ASSERT:
            emit(c, I_ASSERT, nd->arg, 0);
            break;
        case N_CAT:
            for (const Node *k = nd->sub; k && !c->err; k = k->next) compile_node(c, k);
            break;
        case N_ALT: {
            /* split L1, L2; L1: a; jmp end; L2: split ...; chain of jmps */
            int jmp_head = -1;

            for (const Node *k = nd->sub; k && !c->err; k = k->next) {
                if (k->next) {
                    int sp = emit(c, I_SPLIT, 0, 0);

                    c->re->inst[sp].x = sp + 1;
                    compile_node(c, k);
                    int j = emit(c, I_JMP, jmp_head, 0);

                    jmp_head = j;
                    c->re->inst[sp].y = c->re->ninst;
                } else {
                    compile_node(c, k);
                }
            }
            for (int j = jmp_head; j >= 0 && !c->err;) {
                int prev = c->re->inst[j].x;

                c->re->inst[j].x = c->re->ninst;
                j = prev;
            }
            break;
        }
        case N_REPEAT:
            compile_repeat(c, nd);
            break;
        case N_CAP:
            emit(c, I_SAVE, 2 * nd->arg, 0);
            compile_node(c, nd->sub);
            emit(c, I_SAVE, 2 * nd->arg + 1, 0);
            break;
    }
}

ReRegex *kl_re_compile(const char *pat, size_t len, uint32_t flags, ReError *err)
{
    ReRegex *re = calloc(1, sizeof(ReRegex));
    Parser p = { .s = pat, .n = len };
    Compiler c = { 0 };
    Node *root;
    ReError none = { 0, 0 };

    if (err) *err = none;
    if (!re) {
        if (err) err->code = RE_ERR_NOMEM;
        return NULL;
    }
    p.re = re;

    root = parse_alt(&p, &flags);
    if (root && !p.err.code && p.i < p.n) {
        /* parse_alt only stops early at a ')' with no open group */
        p.err.code = RE_ERR_UNEXPECTED_PAREN;
        p.err.pos = p.i;
        root = NULL;
    }
    if (!root || p.err.code) goto fail;

    if (tree_height(root) > RE_MAX_DEPTH) {
        p.err.code = RE_ERR_NESTING_DEPTH;
        p.err.pos = 0;
        goto fail;
    }
    c.re = re;
    emit(&c, I_SAVE, 0, 0);
    compile_node(&c, root);
    emit(&c, I_SAVE, 1, 0);
    emit(&c, I_MATCH, 0, 0);

    if (!c.err && (size_t)re->ninst * (size_t)(2 * (re->ngroups + 1)) > MAX_THREAD_SLOTS) {
        c.err = RE_ERR_TOO_LARGE;
    }
    if (c.err) {
        p.err.code = c.err;
        p.err.pos = 0;
        goto fail;
    }

    nodes_free(p.all);
    return re;

fail:
    if (err) *err = p.err;
    if (err && !err->code) err->code = RE_ERR_INVALID_PERL_OP;
    nodes_free(p.all);
    kl_re_free(re);
    return NULL;
}

/*---------------------------------------------------------------------------+
 |   Pike VM                                                                 |
 +---------------------------------------------------------------------------*/

typedef struct {
    int pc;
    int slot; /* >= 0: restore caps[slot] = old */
    ptrdiff_t old;
} Frame;

typedef struct {
    int *dense;
    int *sparse;
    int n;
    ptrdiff_t *caps; /* ncap entries per dense slot */
} TList;

static inline int _is_word(int c) { return _is_alnum(c) || c == '_'; }

static int table_has(const ReUName *u, uint32_t r)
{
    int a = 0, b = u->nranges - 1;

    while (a <= b) {
        int mid = a + (b - a) / 2;
        const ReURange *g = &u->ranges[mid];

        if (r < g->lo) b = mid - 1;
        else if (r > g->hi) a = mid + 1;
        else return g->stride <= 1 || (r - g->lo) % g->stride == 0;
    }
    return 0;
}

/* Unicode \w: L, N or '_'. The tables are looked up once. */
static int _is_word_rune(uint32_t r)
{
    static const ReUName *letter, *number;

    if (r < 0x80) return _is_word((int)r);
    if (!letter) {
        letter = unicode_lookup("L", 1);
        number = unicode_lookup("N", 1);
    }
    return table_has(letter, r) || table_has(number, r);
}

static int assert_holds(int kind, const char *s, size_t n, size_t pos)
{
    switch (kind) {
        case A_BOL:
            return pos == 0;
        case A_EOL:
            return pos == n;
        case A_BOL_M:
            return pos == 0 || s[pos - 1] == '\n';
        case A_EOL_M:
            return pos == n || s[pos] == '\n';
        case A_WORD_B_U:
        case A_NWORD_B_U: {
            uint32_t r;
            int a = 0, b = 0;

            if (pos > 0 && utf8_decode_last(s, pos, &r) > 0) a = _is_word_rune(r);
            if (pos < n && utf8_decode(s + pos, n - pos, &r) > 0) b = _is_word_rune(r);
            return kind == A_WORD_B_U ? a != b : a == b;
        }
        default: {
            int a = pos > 0 && _is_word((unsigned char)s[pos - 1]);
            int b = pos < n && _is_word((unsigned char)s[pos]);

            return kind == A_WORD_B ? a != b : a == b;
        }
    }
}

static int class_has(const Class *cl, uint32_t r)
{
    int lo = 0, hi = cl->n - 1;

    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;

        if (r < cl->r[mid].lo) hi = mid - 1;
        else if (r > cl->r[mid].hi) lo = mid + 1;
        else return 1;
    }
    return 0;
}

/*
 * Follow the empty transitions from pc0 at text offset pos and put the
 * threads that stop on a consuming instruction (or MATCH) into `list`, in
 * priority order. `scratch` holds the capture slots of the thread being
 * extended; SAVEs change it and a Frame undoes the change on the way back.
 */
static void add_thread(const ReRegex *re, TList *list, Frame *stack, int pc0, const char *s,
                       size_t n, size_t pos, ptrdiff_t *scratch, int ncap)
{
    int top = 0;

    stack[top++] = (Frame){ pc0, -1, 0 };

    while (top > 0) {
        Frame f = stack[--top];
        const Inst *in;

        if (f.slot >= 0) {
            scratch[f.slot] = f.old;
            continue;
        }

        int pc = f.pc;

        if (list->sparse[pc] < list->n && list->dense[list->sparse[pc]] == pc) continue;
        list->sparse[pc] = list->n;
        list->dense[list->n] = pc;

        in = &re->inst[pc];
        switch (in->op) {
            case I_JMP:
                stack[top++] = (Frame){ in->x, -1, 0 };
                break;
            case I_SPLIT:
                stack[top++] = (Frame){ in->y, -1, 0 };
                stack[top++] = (Frame){ in->x, -1, 0 };
                break;
            case I_SAVE:
                if (in->x < ncap) {
                    stack[top++] = (Frame){ 0, in->x, scratch[in->x] };
                    scratch[in->x] = (ptrdiff_t)pos;
                }
                stack[top++] = (Frame){ pc + 1, -1, 0 };
                break;
            case I_ASSERT:
                if (assert_holds(in->x, s, n, pos)) stack[top++] = (Frame){ pc + 1, -1, 0 };
                break;
            default:
                memcpy(list->caps + (size_t)list->n * (size_t)ncap, scratch,
                       (size_t)ncap * sizeof(ptrdiff_t));
                break;
        }
        list->n++;
    }
}

int kl_re_exec(const ReRegex *re, const char *s, size_t n, size_t start, int mode, ptrdiff_t *caps)
{
    int ncap = 2 * (re->ngroups + 1), ninst = re->ninst;
    TList lists[2] = { { 0 }, { 0 } };
    Frame *stack = NULL;
    ptrdiff_t *scratch = NULL, *best = NULL;
    int cur = 0, matched = 0, result = RE_NOMATCH;
    int advance = (mode & RE_MODE_ADVANCE) != 0;
    size_t pos = start;

    mode &= ~RE_MODE_ADVANCE;
    if (start > n) return RE_NOMATCH;

    for (int k = 0; k < 2; k++) {
        lists[k].dense = malloc((size_t)ninst * sizeof(int));
        lists[k].sparse = calloc((size_t)ninst, sizeof(int));
        lists[k].caps = malloc((size_t)ninst * (size_t)ncap * sizeof(ptrdiff_t));
        if (!lists[k].dense || !lists[k].sparse || !lists[k].caps) goto nomem;
    }
    stack = malloc(((size_t)ninst * 2 + 2) * sizeof(Frame));
    scratch = malloc((size_t)ncap * sizeof(ptrdiff_t));
    best = malloc((size_t)ncap * sizeof(ptrdiff_t));
    if (!stack || !scratch || !best) goto nomem;

    for (;;) {
        TList *clist = &lists[cur], *nlist = &lists[cur ^ 1];
        uint32_t rune = 0;
        int w = 0;

        /* a new thread may start here until some match has been found */
        if (!matched && (mode == RE_MODE_SEARCH || pos == start)) {
            for (int k = 0; k < ncap; k++) scratch[k] = -1;
            add_thread(re, clist, stack, 0, s, n, pos, scratch, ncap);
        }
        if (clist->n == 0 && (matched || mode != RE_MODE_SEARCH)) break;

        if (pos < n) w = utf8_decode(s + pos, n - pos, &rune);
        nlist->n = 0;

        for (int i = 0; i < clist->n; i++) {
            int pc = clist->dense[i];
            const Inst *in = &re->inst[pc];
            ptrdiff_t *cp = clist->caps + (size_t)i * (size_t)ncap;
            int ok = 0;

            switch (in->op) {
                case I_MATCH:
                    if (mode == RE_MODE_FULLMATCH && pos != n) continue;
                    /* an empty match at `start` is refused, the next thread may do better */
                    if (advance && pos == start && cp[0] == (ptrdiff_t)start) continue;
                    memcpy(best, cp, (size_t)ncap * sizeof(ptrdiff_t));
                    matched = 1;
                    /* lower-priority threads cannot beat this match */
                    i = clist->n;
                    continue;
                case I_RUNE:
                    ok = w && rune == (uint32_t)in->x;
                    break;
                case I_CLASS:
                    ok = w && class_has(&re->cls[in->x], rune);
                    break;
                case I_ANY:
                    ok = w && rune != '\n';
                    break;
                case I_ANYNL:
                    ok = w != 0;
                    break;
                default:
                    break; /* JMP, SPLIT, SAVE, ASSERT: already followed */
            }

            if (ok) {
                memcpy(scratch, cp, (size_t)ncap * sizeof(ptrdiff_t));
                add_thread(re, nlist, stack, pc + 1, s, n, pos + (size_t)w, scratch, ncap);
            }
        }

        if (pos >= n) break;
        pos += (size_t)w;
        cur ^= 1;
        lists[cur ^ 1].n = 0;
    }

    if (matched) {
        if (caps) memcpy(caps, best, (size_t)ncap * sizeof(ptrdiff_t));
        result = RE_MATCH;
    }
    goto done;

nomem:
    result = -RE_ERR_NOMEM;
done:
    for (int k = 0; k < 2; k++) {
        free(lists[k].dense);
        free(lists[k].sparse);
        free(lists[k].caps);
    }
    free(stack);
    free(scratch);
    free(best);
    return result;
}

/*---------------------------------------------------------------------------+
 |   Successive matches                                                      |
 +---------------------------------------------------------------------------*/

int kl_re_iter_next(const ReRegex *re, const char *s, size_t n, ReIter *it, ptrdiff_t *caps)
{
    int mode = RE_MODE_SEARCH | (it->must_advance ? RE_MODE_ADVANCE : 0);
    int rc;

    if (it->pos > n) return RE_NOMATCH;
    rc = kl_re_exec(re, s, n, it->pos, mode, caps);
    if (rc != RE_MATCH) return rc;

    it->must_advance = caps[0] == caps[1];
    it->pos = (size_t)caps[1];
    return RE_MATCH;
}

/*---------------------------------------------------------------------------+
 |   Byte buffer                                                             |
 +---------------------------------------------------------------------------*/

int kl_re_buf_append(ReBuf *buf, const char *b, size_t n)
{
    if (buf->n + n > buf->cap) {
        size_t cap = buf->cap ? buf->cap : 64;
        char *p;

        while (cap < buf->n + n) cap *= 2;
        p = realloc(buf->p, cap);
        if (!p) return -1;
        buf->p = p;
        buf->cap = cap;
    }
    if (n) memcpy(buf->p + buf->n, b, n);
    buf->n += n;
    return 0;
}

void kl_re_buf_free(ReBuf *buf)
{
    free(buf->p);
    buf->p = NULL;
    buf->n = buf->cap = 0;
}
