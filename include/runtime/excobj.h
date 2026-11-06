/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_EXCEPT_H_
#define _KOALA_EXCEPT_H_

#include "vm.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline Object *pop_exc()
{
    KoalaState *ks = __ks();
    Object *exc = ks->exc;
    ks->exc = NULL;
    return exc;
}

void print_exc_and_free(Object *obj);

void _raise_exc_fmt(KoalaState *ks, char *fmt, ...);

#define raise_exc_fmt(fmt, args...) \
    do { \
        KoalaState *ks = __ks(); \
        _raise_exc_fmt(ks, fmt, args); \
    } while (0)

void _raise_exc_str(KoalaState *ks, char *str);

#define raise_exc_str(str) \
    do { \
        KoalaState *ks = __ks(); \
        _raise_exc_str(ks, str); \
    } while (0)

#define raise_exc(str) raise_exc_str(str)

/* A shift count must be in [0, 64): integers are promoted to 64 bits before they are shifted.
 * Returns 0 with an exception raised otherwise. */
static inline int kl_check_shift_count(TValue *count)
{
    uint64_t bits = (uint64_t)count->ival;
    if (bits < 64) return 1;

    if (!is_uint(count) && (int64_t)bits < 0) {
        raise_exc_str("negative shift count");
    } else {
        raise_exc_str("shift count out of range");
    }
    return 0;
}

char *kl_exc_get_msg(Object *exc);
void kl_free_exc(Object *exc);

void trace_here(CallFrame *cf);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_EXCEPT_H_ */
