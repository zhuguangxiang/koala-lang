/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * Unicode tables for koala/re: the classes behind \p{..} and the case
 * orbits behind case-insensitive matching. re_unicode.c is generated from
 * Go's unicode package by tools/gen_unicode.go, so the two agree.
 */

#ifndef _KOALA_RE_UNICODE_H_
#define _KOALA_RE_UNICODE_H_

#include <stddef.h>
#include <stdint.h>

/* Runes lo, lo + stride, lo + 2 * stride, ... up to hi, as in Go's
 * unicode.Range16/Range32. */
typedef struct {
    uint32_t lo, hi, stride;
} ReURange;

/* A class name as \p{name} reaches it after canonicalization. `fold` lists
 * the runes outside the table that fold to a rune inside it; with the I flag
 * the class is the table plus those, exactly as in Go. */
typedef struct {
    const char *name;
    const ReURange *ranges;
    int nranges;
    const ReURange *fold;
    int nfold;
    int negate; /* 1 if the class is the complement of the table (Assigned) */
} ReUName;

/* Sorted by name (strcmp order). */
extern const ReUName kl_re_unicode_names[];
extern const int kl_re_unicode_nnames;

/* r and the next rune of its simple case orbit, sorted by r. */
typedef struct {
    uint32_t r, next;
} ReUFold;

extern const ReUFold kl_re_unicode_fold[];
extern const int kl_re_unicode_nfold;

#endif /* _KOALA_RE_UNICODE_H_ */
