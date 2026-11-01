/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * Linear-time regular expression engine for koala/re.
 *
 * The syntax and the matching rules follow Go's regexp (RE2): leftmost-first
 * matching, UTF-8 text, no backreferences and no lookaround. Matching is a
 * Pike VM, so one search costs O(len(program) * len(text)) and never
 * backtracks.
 *
 * Nothing here knows about TValue, str objects or the exception machinery;
 * re_shim.c owns that translation, the way hex_shim.c does for hex_core.c.
 * The only dependency is the rune API of utf8.h.
 *
 * Functions are prefixed `kl_re_`: glibc already exports re_exec, re_compile and
 * friends (the old GNU regex API), and a plain re_* name in a native package
 * resolves to libc's function instead of ours.
 *
 * Text is a byte range [s, s + n). Offsets are byte offsets. Invalid UTF-8
 * bytes are matched as U+FFFD, one byte at a time, as in Go.
 *
 * Unicode support is Go's: \p{..} classes and the case orbits of the I flag
 * come from tables that tools/gen_unicode.go generates out of Go's unicode
 * package (re_unicode.c), so both agree on the Unicode version and on the
 * rules for class names. \d, \s, \w and \b stay ASCII only, as in Go.
 */

#ifndef _KOALA_RE_CORE_H_
#define _KOALA_RE_CORE_H_

#include <stddef.h>
#include <stdint.h>

/*---------------------------------------------------------------------------+
 |   Flags and errors                                                        |
 +---------------------------------------------------------------------------*/

/* Initial flags for kl_re_compile; the same letters work inline as (?ims-U). */
#define RE_FLAG_I 0x01 /* case-insensitive (Unicode simple folding) */
#define RE_FLAG_M 0x02 /* ^ and $ also match at line boundaries */
#define RE_FLAG_S 0x04 /* . matches \n */
#define RE_FLAG_U 0x08 /* swap greedy and lazy quantifiers */
#define RE_FLAG_UNICODE 0x10 /* \d \w \s \b by Unicode, as in Python's str patterns */

/*
 * Limits, with Go's values where Go has one:
 *
 * - RE_MAX_REPEAT: a {n,m} count; the counts of nested {..} repeats may not
 *   multiply to more than this either, so (a{100}){100} is rejected.
 * - RE_MAX_DEPTH: height of the parse tree, as Go counts it.
 * - RE_MAX_INST: program size, Go's limit of 128 MB at 40 bytes per
 *   instruction. Go's own estimate of the size rejects an expression first;
 *   this bound only catches what the estimate misses. A search keeps one
 *   thread slot per instruction and capture, so a huge program that also has
 *   many groups can still be refused for its memory.
 */
#define RE_MAX_REPEAT 1000
#define RE_MAX_DEPTH  1000
#define RE_MAX_INST   ((128 << 20) / 40 + 1024)

/* The error codes are the ones of Go's regexp/syntax.ErrorCode. */
enum {
    RE_OK = 0,
    RE_ERR_NOMEM,
    RE_ERR_INVALID_UTF8,
    RE_ERR_TRAILING_BACKSLASH,
    RE_ERR_INVALID_ESCAPE, /* also \1, a backreference */
    RE_ERR_MISSING_BRACKET,
    RE_ERR_MISSING_PAREN,
    RE_ERR_UNEXPECTED_PAREN,
    RE_ERR_MISSING_REPEAT_ARG,
    RE_ERR_INVALID_REPEAT_OP,
    RE_ERR_INVALID_REPEAT_SIZE,
    RE_ERR_INVALID_CLASS_RANGE, /* also an unknown class name */
    RE_ERR_INVALID_NAMED_CAPTURE,
    RE_ERR_INVALID_PERL_OP, /* (?=, (?!, (?>, (?#, a bad flag group */
    RE_ERR_NESTING_DEPTH,
    RE_ERR_TOO_LARGE,
};

typedef struct {
    int code;   /* RE_OK or RE_ERR_* */
    size_t pos; /* byte offset into the pattern where the problem was seen */
} ReError;

/* A fixed message for an error code. Never NULL. */
const char *kl_re_strerror(int code);

/*---------------------------------------------------------------------------+
 |   Compile                                                                 |
 +---------------------------------------------------------------------------*/

typedef struct ReRegex ReRegex;

/* Compile pat[0, len) with the initial `flags`. Returns NULL on failure and
 * fills *err when err is not NULL. The result is immutable: any number of
 * searches may use it at the same time. */
ReRegex *kl_re_compile(const char *pat, size_t len, uint32_t flags, ReError *err);

void kl_re_free(ReRegex *re);

/* Number of capture groups, not counting group 0 (the whole match). */
int kl_re_ngroups(const ReRegex *re);

/* Name of group i (1..ngroups), or NULL if it is unnamed or out of range. */
const char *kl_re_group_name(const ReRegex *re, int i);

/* Index of the group called name[0, len), or -1. */
int kl_re_group_index(const ReRegex *re, const char *name, size_t len);

/*---------------------------------------------------------------------------+
 |   Match                                                                   |
 +---------------------------------------------------------------------------*/

enum {
    RE_MODE_SEARCH = 0, /* leftmost match starting at or after `start` */
    RE_MODE_MATCH,      /* the match must start at `start` */
    RE_MODE_FULLMATCH,  /* the match must start at `start` and end at n */
};

/* Or-ed into a mode: no empty match at `start` itself (Python's
 * must_advance); a non-empty one there, or any match later on, is fine. */
#define RE_MODE_ADVANCE 0x10

#define RE_NOMATCH 0
#define RE_MATCH   1

/* Search s[0, n) beginning at byte offset `start`.
 *
 * ^, \A and \b look at the text before `start`, so continuing a find-all
 * loop with a larger `start` behaves as if the text had been scanned in one
 * go.
 *
 * caps must hold 2 * (kl_re_ngroups + 1) entries (it may be NULL). On a match
 * caps[2 * i] and caps[2 * i + 1] are the begin and end offsets of group i,
 * or -1 for a group that did not take part.
 *
 * Returns RE_MATCH, RE_NOMATCH, or a negative RE_ERR_* on allocation
 * failure. */
int kl_re_exec(const ReRegex *re, const char *s, size_t n, size_t start, int mode, ptrdiff_t *caps);

/*---------------------------------------------------------------------------+
 |   Successive matches                                                      |
 +---------------------------------------------------------------------------*/

/* State of a find-all loop; start it with RE_ITER_INIT. */
typedef struct {
    size_t pos;       /* where the next search starts */
    int must_advance; /* the last match was empty and ended at pos */
} ReIter;

#define RE_ITER_INIT { 0, 0 }

/* The next match of a find-all loop, with the rules of Python's finditer
 * (3.7+): matches do not overlap; the next search starts where the last
 * match ended, and after an empty match it may not find another empty match
 * at the same place (a non-empty one there is fine). So "a*" on "baaab"
 * gives "", "aaa", "", "".
 *
 * caps is filled as in kl_re_exec and must not be NULL. Returns RE_MATCH,
 * RE_NOMATCH once the text is exhausted, or a negative RE_ERR_*. */
int kl_re_iter_next(const ReRegex *re, const char *s, size_t n, ReIter *it, ptrdiff_t *caps);

/*---------------------------------------------------------------------------+
 |   Byte buffer                                                             |
 +---------------------------------------------------------------------------*/

/* A growable byte buffer. Start from {0}; free with kl_re_buf_free. */
typedef struct {
    char *p;
    size_t n, cap;
} ReBuf;

/* Append b[0, n). Returns 0, or -1 when out of memory. */
int kl_re_buf_append(ReBuf *buf, const char *b, size_t n);
void kl_re_buf_free(ReBuf *buf);

#endif /* _KOALA_RE_CORE_H_ */
