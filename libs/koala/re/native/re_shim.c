/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * Koala bindings for the regular expression engine in re_core.c.
 *
 * `Pattern` is a native class: the instance holds the compiled ReRegex, and the
 * type's finalizer releases it when the object is collected. Only the
 * primitives live here (compile, group info, exec); everything built on them
 * is written in Koala.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dictobj.h"
#include "excobj.h"
#include "listobj.h"
#include "re_core.h"
#include "utf8.h"

/* The Koala fields come first, in declaration order: `groups`, `pattern`,
   `flags` and `groupindex`. */
typedef struct _PatternObject {
    OBJECT_HEAD
    TValue groups;
    TValue pattern;
    TValue flags;
    TValue groupindex;
    ReRegex *re;
} PatternObject;

static TypeObject pattern_type;

/* Python's flag values */
#define PY_IGNORECASE 2
#define PY_MULTILINE  8
#define PY_DOTALL     16
#define PY_ASCII      256
#define VALID_FLAGS   (PY_IGNORECASE | PY_MULTILINE | PY_DOTALL | PY_ASCII)

/* The engine's flags for Python's `flags`. */
static uint32_t _engine_flags(int64_t flags)
{
    /* \d \w \s \b are Unicode for a str pattern, as in Python, unless re.ASCII */
    uint32_t bits = (flags & PY_ASCII) ? 0 : RE_FLAG_UNICODE;

    if (flags & PY_IGNORECASE) bits |= RE_FLAG_I;
    if (flags & PY_MULTILINE) bits |= RE_FLAG_M;
    if (flags & PY_DOTALL) bits |= RE_FLAG_S;
    return bits;
}

/* Released by the collector; re is NULL if __init__ never succeeded. */
static void _pattern_free(Object *self)
{
    PatternObject *pobj = (PatternObject *)self;

    kl_re_free(pobj->re);
    pobj->re = NULL;
}

/* Group name -> number, the leftmost group for a name used twice. */
static Object *_new_groupindex(const ReRegex *re)
{
    Object *dict = kl_new_dict();

    for (int i = 1; i <= kl_re_ngroups(re); i++) {
        const char *name = kl_re_group_name(re, i);
        if (!name || kl_re_group_index(re, name, strlen(name)) != i) continue;

        TValue key = kl_val_str((char *)name);
        TValue val = int64_value(i);
        kl_dict_add(dict, &key, &val);
    }
    return dict;
}

// func __init__(text str, bits int) {}
static TValue _pattern_init(TValue *self, TValue *args, int nargs)
{
    PatternObject *pobj = SELF_AS(pattern_type);
    Object *pat;
    int64_t flags;
    ReError err;
    ReRegex *re;

    ASSERT(nargs == 2);
    pat = kl_arg_strobj(0);
    flags = kl_arg_int64(1);

    if (flags < 0 || (flags & ~(int64_t)VALID_FLAGS) != 0) {
        raise_exc_fmt("re: invalid flags %lld", (long long)flags);
        return error_value;
    }

    re = kl_re_compile(STR_BUF(pat), STR_LEN(pat), _engine_flags(flags), &err);
    if (!re) {
        raise_exc_fmt("re: invalid pattern: %s at %zu", kl_re_strerror(err.code), err.pos);
        return error_value;
    }

    /* a second __init__ call replaces the program */
    kl_re_free(pobj->re);
    pobj->re = re;
    pobj->groups = int64_value(kl_re_ngroups(re));
    pobj->pattern = args[0];
    pobj->flags = int64_value(flags);
    pobj->groupindex = obj_value(_new_groupindex(re));
    return no_value;
}

/* The program of `self`; a Pattern whose __init__ failed has none. */
static ReRegex *_re_of(TValue *self)
{
    PatternObject *pobj = SELF_AS(pattern_type);

    if (!pobj->re) raise_exc_str("re: Pattern is not initialized");
    return pobj->re;
}

// func group_name(i int) str? {}
static TValue _pattern_group_name(TValue *self, TValue *args, int nargs)
{
    ReRegex *re = _re_of(self);
    const char *name;

    ASSERT(nargs == 1);
    if (!re) return error_value;

    int64_t i = kl_arg_int64(0);
    if (i < 1 || i > kl_re_ngroups(re)) return nil_value;

    name = kl_re_group_name(re, (int)i);
    if (!name) return nil_value;
    return kl_val_str((char *)name);
}

// func group_index(name str) int {}
static TValue _pattern_group_index(TValue *self, TValue *args, int nargs)
{
    ReRegex *re = _re_of(self);
    Object *name;

    ASSERT(nargs == 1);
    if (!re) return error_value;

    name = kl_arg_strobj(0);
    return int64_value(kl_re_group_index(re, STR_BUF(name), STR_LEN(name)));
}

/* Check 0 <= end <= len(s): the text is s[0, end). */
static int _check_end(Object *so, int64_t end)
{
    if (end < 0 || (uint64_t)end > STR_LEN(so)) {
        raise_exc_fmt("re: end %lld out of range", (long long)end);
        return -1;
    }
    return 0;
}

// func exec(s str, start int, end int, mode int) list[int] {}
static TValue _pattern_exec(TValue *self, TValue *args, int nargs)
{
    ReRegex *re = _re_of(self);
    Object *so, *list;
    int64_t start, end, mode;
    ptrdiff_t *caps;
    int ncap, rc;

    ASSERT(nargs == 4);
    if (!re) return error_value;

    so = kl_arg_strobj(0);
    start = kl_arg_int64(1);
    end = kl_arg_int64(2);
    mode = kl_arg_int64(3);

    if (_check_end(so, end) < 0) return error_value;
    if (start < 0 || start > end) {
        raise_exc_fmt("re: start %lld out of range", (long long)start);
        return error_value;
    }
    if (mode < RE_MODE_SEARCH || mode > RE_MODE_FULLMATCH) {
        raise_exc_fmt("re: invalid mode %lld", (long long)mode);
        return error_value;
    }

    ncap = 2 * (kl_re_ngroups(re) + 1);
    caps = malloc((size_t)ncap * sizeof(ptrdiff_t));
    if (!caps) {
        raise_exc_str("re: out of memory");
        return error_value;
    }

    rc = kl_re_exec(re, STR_BUF(so), (size_t)end, (size_t)start, (int)mode, caps);
    if (rc < 0) {
        free(caps);
        raise_exc_str("re: out of memory");
        return error_value;
    }

    list = kl_new_list();
    if (rc == RE_MATCH) {
        for (int i = 0; i < ncap; i++) kl_list_append(list, int64_value(caps[i]));
    }
    free(caps);
    return obj_value(list);
}

static void _no_mem(void) { raise_exc_str("re: out of memory"); }

static void _push_caps(Object *list, const ptrdiff_t *caps, int ncap)
{
    for (int i = 0; i < ncap; i++) kl_list_append(list, int64_value(caps[i]));
}

// func all_matches(s str, pos int, end int, n int) list[int] {}
//
// Every match of Go's FindAll in s[0, end) from `pos` on, at most n of them
// when n >= 0, as one flat list: the offsets of group 0, 1, ... of the first
// match, then of the second, and so on.
static TValue _pattern_all_matches(TValue *self, TValue *args, int nargs)
{
    ReRegex *re = _re_of(self);
    Object *so, *list;
    int64_t pos, end, limit, count = 0;
    ptrdiff_t *caps;
    ReIter it = RE_ITER_INIT;
    int ncap, rc = RE_NOMATCH;

    ASSERT(nargs == 4);
    if (!re) return error_value;

    so = kl_arg_strobj(0);
    pos = kl_arg_int64(1);
    end = kl_arg_int64(2);
    limit = kl_arg_int64(3);

    if (_check_end(so, end) < 0) return error_value;
    if (pos < 0 || pos > end) {
        raise_exc_fmt("re: start %lld out of range", (long long)pos);
        return error_value;
    }
    it.pos = (size_t)pos;
    ncap = 2 * (kl_re_ngroups(re) + 1);

    caps = malloc((size_t)ncap * sizeof(ptrdiff_t));
    if (!caps) {
        _no_mem();
        return error_value;
    }

    list = kl_new_list();
    while ((limit < 0 || count < limit) &&
           (rc = kl_re_iter_next(re, STR_BUF(so), (size_t)end, &it, caps)) == RE_MATCH) {
        _push_caps(list, caps, ncap);
        count++;
    }
    free(caps);

    if (rc < 0 && (limit < 0 || count < limit)) {
        _no_mem();
        return error_value;
    }
    return obj_value(list);
}

// func next_match(s str, pos int, must_advance int, end int) list[int] {}
//
// One step of a find-all loop over s[0, end) started at `pos`; `must_advance`
// is 1 after an empty match. Return an empty list when the text is used up,
// else the offsets of the match followed by the `pos` and `must_advance` to
// continue with.
static TValue _pattern_next_match(TValue *self, TValue *args, int nargs)
{
    ReRegex *re = _re_of(self);
    Object *so, *list;
    int64_t pos, adv, end;
    ptrdiff_t *caps;
    ReIter it;
    int ncap, rc;

    ASSERT(nargs == 4);
    if (!re) return error_value;

    so = kl_arg_strobj(0);
    pos = kl_arg_int64(1);
    adv = kl_arg_int64(2);
    end = kl_arg_int64(3);

    if (_check_end(so, end) < 0) return error_value;
    if (pos < 0 || pos > end + 1) {
        raise_exc_fmt("re: start %lld out of range", (long long)pos);
        return error_value;
    }

    ncap = 2 * (kl_re_ngroups(re) + 1);
    caps = malloc((size_t)ncap * sizeof(ptrdiff_t));
    if (!caps) {
        _no_mem();
        return error_value;
    }

    it.pos = (size_t)pos;
    it.must_advance = adv != 0;
    rc = kl_re_iter_next(re, STR_BUF(so), (size_t)end, &it, caps);
    if (rc < 0) {
        free(caps);
        _no_mem();
        return error_value;
    }

    list = kl_new_list();
    if (rc == RE_MATCH) {
        _push_caps(list, caps, ncap);
        kl_list_append(list, int64_value(it.pos));
        kl_list_append(list, int64_value(it.must_advance));
    }
    free(caps);
    return obj_value(list);
}

/*
 * Python's replacement templates (sre_parse.parse_template): a tokenizer
 * that, like Python's, reads one token ahead, so a lone `\` at the end is
 * reported as soon as the token before it is taken. A token is one
 * character or a backslash and the character after it. Positions in the
 * messages are byte offsets into the template.
 */
typedef struct {
    const char *t;
    size_t n;
    size_t pos; /* start of the token ahead */
    size_t len; /* its length, 0 at the end */
} PyTok;

static int _tok_err(size_t pos, const char *msg)
{
    raise_exc_fmt("re: %s at position %zu", msg, pos);
    return -1;
}

/* Read the token at `at` as the token ahead. */
static int _tok_read(PyTok *k, size_t at)
{
    k->pos = at;
    if (at >= k->n) {
        k->len = 0;
        return 0;
    }
    if (k->t[at] == '\\') {
        if (at + 1 >= k->n) return _tok_err(k->n - 1, "bad escape (end of pattern)");
        k->len = 1 + (size_t)utf8_decode(k->t + at + 1, k->n - at - 1, NULL);
    } else {
        k->len = (size_t)utf8_decode(k->t + at, k->n - at, NULL);
    }
    return 0;
}

/* Take the token ahead: it is t[*at, *at + *len). */
static int _tok_take(PyTok *k, size_t *at, size_t *len)
{
    *at = k->pos;
    *len = k->len;
    return _tok_read(k, k->pos + k->len);
}

/* True if the token ahead is the single character `c`. */
static int _tok_is(const PyTok *k, char c) { return k->len == 1 && k->t[k->pos] == c; }

static int _is_digit(char c) { return c >= '0' && c <= '9'; }
static int _is_octal(char c) { return c >= '0' && c <= '7'; }

/* Append the character `r` as UTF-8. */
static int _put_rune(ReBuf *out, uint32_t r)
{
    char b[4];
    int w = utf8_encode(r, b);

    return out ? kl_re_buf_append(out, b, (size_t)w) : 0;
}

/* Group `index` of the match, or an error if there is no such group. */
static int _put_group(const ReRegex *re, ReBuf *out, const char *src, const ptrdiff_t *caps,
                      long index, size_t pos)
{
    char msg[64];

    if (index < 0 || index > kl_re_ngroups(re)) {
        snprintf(msg, sizeof(msg), "invalid group reference %ld", index);
        return _tok_err(pos, msg);
    }
    if (!out || caps[2 * index] < 0) return 0;
    return kl_re_buf_append(out, src + caps[2 * index], (size_t)(caps[2 * index + 1] - caps[2 * index]));
}

/* An identifier as a group name. Python asks for str.isidentifier(); here
 * any non-ASCII character counts as a letter. */
static int _is_ident(const char *s, size_t n)
{
    if (n == 0 || _is_digit(s[0])) return 0;
    for (size_t i = 0; i < n; i++) {
        unsigned char c = (unsigned char)s[i];

        if (c >= 0x80 || c == '_' || _is_digit((char)c) || (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z')) {
            continue;
        }
        return 0;
    }
    return 1;
}

/* \g<...>, after the `g`: a group number or name. */
static int _template_group(PyTok *k, const ReRegex *re, ReBuf *out, const char *src,
                           const ptrdiff_t *caps)
{
    size_t at, len, name_at, name_len = 0;
    char msg[160];
    long index = 0;
    int digits = 1;

    if (!_tok_is(k, '<')) return _tok_err(k->pos, "missing <");
    if (_tok_take(k, &at, &len) < 0) return -1;

    name_at = k->pos;
    for (;;) {
        if (k->len == 0) {
            if (name_len == 0) return _tok_err(k->pos, "missing group name");
            return _tok_err(k->pos - name_len, "missing >, unterminated name");
        }
        if (_tok_take(k, &at, &len) < 0) return -1;
        if (len == 1 && k->t[at] == '>') {
            if (name_len == 0) return _tok_err(k->pos - 1, "missing group name");
            break;
        }
        name_len += len;
    }

    for (size_t i = 0; i < name_len; i++) {
        if (!_is_digit(k->t[name_at + i])) digits = 0;
    }
    if (digits) {
        for (size_t i = 0; i < name_len; i++) {
            index = index * 10 + (k->t[name_at + i] - '0');
            if (index > 1000000000L) {
                snprintf(msg, sizeof(msg), "invalid group reference %.*s", (int)name_len,
                         k->t + name_at);
                return _tok_err(name_at, msg);
            }
        }
    } else {
        if (!_is_ident(k->t + name_at, name_len)) {
            snprintf(msg, sizeof(msg), "bad character in group name '%.*s'", (int)name_len,
                     k->t + name_at);
            return _tok_err(name_at, msg);
        }
        index = kl_re_group_index(re, k->t + name_at, name_len);
        if (index < 0) {
            raise_exc_fmt("re: unknown group name '%.*s'", (int)name_len, k->t + name_at);
            return -1;
        }
    }
    return _put_group(re, out, src, caps, index, name_at);
}

/* Expand the template t[0, tn) with the match `caps` of `src` into `out`;
 * with `out` NULL only check it. Returns 0, or -1 with an error raised. */
static int _expand_template(const ReRegex *re, const char *t, size_t tn, const char *src,
                            const ptrdiff_t *caps, ReBuf *out)
{
    static const char escapes[] = "a\ab\bf\fn\nr\rt\tv\v\\\\";
    PyTok k = { t, tn, 0, 0 };
    size_t at, len, d_at, d_len;
    char msg[96];

    if (_tok_read(&k, 0) < 0) return -1;

    while (k.len > 0) {
        if (_tok_take(&k, &at, &len) < 0) return -1;

        if (t[at] != '\\') {
            if (out && kl_re_buf_append(out, t + at, len) < 0) goto nomem;
            continue;
        }

        char c = t[at + 1];
        if (c == 'g') {
            if (_template_group(&k, re, out, src, caps) < 0) return -1;
        } else if (c == '0') {
            /* \0 and up to two more octal digits: that character */
            uint32_t v = 0;
            for (int i = 0; i < 2 && k.len == 1 && _is_octal(t[k.pos]); i++) {
                v = v * 8 + (uint32_t)(t[k.pos] - '0');
                if (_tok_take(&k, &d_at, &d_len) < 0) return -1;
            }
            if (_put_rune(out, v & 0xff) < 0) goto nomem;
        } else if (_is_digit(c)) {
            long index = c - '0';
            if (k.len == 1 && _is_digit(t[k.pos])) {
                char d2 = t[k.pos];
                if (_tok_take(&k, &d_at, &d_len) < 0) return -1;
                if (_is_octal(c) && _is_octal(d2) && k.len == 1 && _is_octal(t[k.pos])) {
                    /* three octal digits: that character */
                    char d3 = t[k.pos];
                    uint32_t v = (uint32_t)((c - '0') * 64 + (d2 - '0') * 8 + (d3 - '0'));
                    if (_tok_take(&k, &d_at, &d_len) < 0) return -1;
                    if (v > 0377) {
                        snprintf(msg, sizeof(msg),
                                 "octal escape value \\%c%c%c outside of range 0-0o377", c, d2, d3);
                        return _tok_err(at, msg);
                    }
                    if (_put_rune(out, v) < 0) goto nomem;
                    continue;
                }
                index = index * 10 + (d2 - '0');
            }
            if (_put_group(re, out, src, caps, index, at + 1) < 0) return -1;
        } else {
            const char *e = len == 2 && c ? strchr(escapes, c) : NULL;
            /* the table is pairs: the letter, then what it stands for */
            if (e && ((e - escapes) % 2) == 0) {
                if (out && kl_re_buf_append(out, e + 1, 1) < 0) goto nomem;
            } else if (len == 2 && ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))) {
                snprintf(msg, sizeof(msg), "bad escape \\%c", c);
                return _tok_err(at, msg);
            } else {
                /* any other escape stays as it is */
                if (out && kl_re_buf_append(out, t + at, len) < 0) goto nomem;
            }
        }
    }
    return 0;

nomem:
    _no_mem();
    return -1;
}

// func expand(template str, s str, caps list[int]) str {}
static TValue _pattern_expand(TValue *self, TValue *args, int nargs)
{
    ReRegex *re = _re_of(self);
    Object *tmpl, *src, *list;
    ptrdiff_t *caps;
    ReBuf out = { 0 };
    TValue res;
    int ncap;

    ASSERT(nargs == 3);
    if (!re) return error_value;

    tmpl = kl_arg_strobj(0);
    src = kl_arg_strobj(1);
    list = kl_arg_obj_as(2, list_type);
    ncap = 2 * (kl_re_ngroups(re) + 1);

    /* no offsets: only check the template */
    if (kl_list_len(list) == 0) {
        if (_expand_template(re, STR_BUF(tmpl), STR_LEN(tmpl), NULL, NULL, NULL) < 0) {
            return error_value;
        }
        return kl_val_nstr("", 0);
    }

    if (kl_list_len(list) != ncap) {
        raise_exc_fmt("re: expected %d offsets, got %d", ncap, kl_list_len(list));
        return error_value;
    }

    caps = malloc((size_t)ncap * sizeof(ptrdiff_t));
    if (!caps) {
        _no_mem();
        return error_value;
    }
    for (int i = 0; i < ncap; i++) {
        int64_t v = kl_list_get(list, i)->ival;

        /* an offset outside the text would make the copy read past it */
        if (v < -1 || v > (int64_t)STR_LEN(src)) {
            free(caps);
            raise_exc_fmt("re: offset %lld out of range", (long long)v);
            return error_value;
        }
        caps[i] = (ptrdiff_t)v;
    }
    for (int g = 0; g < ncap; g += 2) {
        if ((caps[g] < 0) != (caps[g + 1] < 0) || caps[g] > caps[g + 1]) {
            free(caps);
            raise_exc_str("re: invalid offsets");
            return error_value;
        }
    }

    if (_expand_template(re, STR_BUF(tmpl), STR_LEN(tmpl), STR_BUF(src), caps, &out) < 0) {
        free(caps);
        kl_re_buf_free(&out);
        return error_value;
    }
    free(caps);

    res = kl_val_nstr(out.p ? out.p : "", out.n);
    kl_re_buf_free(&out);
    return res;
}

// func escape(s str) str {}
static TValue _escape(TValue *self, TValue *args, int nargs)
{
    Object *so;
    ReBuf out = { 0 };
    TValue res;
    size_t n;

    ASSERT(nargs == 1);
    so = kl_arg_strobj(0);
    n = STR_LEN(so);

    for (size_t i = 0; i < n; i++) {
        char c = STR_BUF(so)[i];

        /* the characters Python's re.escape escapes */
        if (c && strchr("()[]{}?*+-|^$\\.&~# \t\n\r\v\f", c) &&
            kl_re_buf_append(&out, "\\", 1) < 0)
            goto nomem;
        if (kl_re_buf_append(&out, &c, 1) < 0) goto nomem;
    }

    res = kl_val_nstr(out.p ? out.p : "", out.n);
    kl_re_buf_free(&out);
    return res;

nomem:
    kl_re_buf_free(&out);
    _no_mem();
    return error_value;
}

// func _repr(s str, limit int) str {}
//
// Python's repr() of a str, cut to `limit` characters as `%.<limit>R` does
// (the closing quote goes too): quoted with ' unless only " is free, with \\,
// the quote, \t \n \r and the other ASCII controls (\xNN) escaped. Other
// characters are kept as they are; Python also escapes the non-printable ones
// outside ASCII.
static TValue _repr(TValue *self, TValue *args, int nargs)
{
    Object *so;
    ReBuf out = { 0 };
    TValue res;
    const char *p;
    size_t n, cut;
    int64_t limit, count;
    char q;

    ASSERT(nargs == 2);
    so = kl_arg_strobj(0);
    limit = kl_arg_int64(1);
    p = STR_BUF(so);
    n = STR_LEN(so);
    q = (memchr(p, '\'', n) && !memchr(p, '"', n)) ? '"' : '\'';

    if (kl_re_buf_append(&out, &q, 1) < 0) goto nomem;
    for (size_t i = 0; i < n; i++) {
        unsigned char c = (unsigned char)p[i];
        char esc[5];
        int len;

        if (c == '\\' || c == (unsigned char)q) {
            esc[0] = '\\';
            esc[1] = (char)c;
            len = 2;
        } else if (c == '\t' || c == '\n' || c == '\r') {
            esc[0] = '\\';
            esc[1] = c == '\t' ? 't' : (c == '\n' ? 'n' : 'r');
            len = 2;
        } else if (c < 0x20 || c == 0x7f) {
            snprintf(esc, sizeof(esc), "\\x%02x", c);
            len = 4;
        } else {
            esc[0] = (char)c;
            len = 1;
        }
        if (kl_re_buf_append(&out, esc, (size_t)len) < 0) goto nomem;
    }
    if (kl_re_buf_append(&out, &q, 1) < 0) goto nomem;

    /* cut after `limit` characters */
    cut = 0;
    for (count = 0; count < limit && cut < out.n; count++) {
        cut += (size_t)utf8_decode(out.p + cut, out.n - cut, NULL);
    }

    res = kl_val_nstr(out.p, cut);
    kl_re_buf_free(&out);
    return res;

nomem:
    kl_re_buf_free(&out);
    _no_mem();
    return error_value;
}

static MethodDef pattern_methods[] = {
    { "__init__", _pattern_init },
    { "group_name", _pattern_group_name },
    { "group_index", _pattern_group_index },
    { "exec", _pattern_exec },
    { "all_matches", _pattern_all_matches },
    { "next_match", _pattern_next_match },
    { "expand", _pattern_expand },
    { NULL, NULL },
};

static TypeObject pattern_type = {
    ._type = &type_type,
    ._nfields = 4,
    .name = "Pattern",
    .flags = TP_FLAGS_CLASS,
    .objsize = sizeof(PatternObject),
    .methdefs = pattern_methods,
    ._free = _pattern_free,
};

/// Entry point called by the runtime after `dlopen("libkoala_re.so")`.
///
/// The name must be `<last segment of the package path>_native_lib_init`,
/// so `koala/re` becomes `re_native_lib_init`.
void re_native_lib_init(NativeLib *lib)
{
    kl_reg_type(lib, &pattern_type);
    kl_reg_func(lib, "escape", _escape);
    kl_reg_func(lib, "_repr", _repr);
}
