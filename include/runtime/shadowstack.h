/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_SHADOW_STACK_H_
#define _KOALA_SHADOW_STACK_H_

#include "vm.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Native temporary roots (plan §8.3). A native that holds an Object* only
 * in a C local across an allocation or a park protects it with:
 *
 *   Object *r = kl_new_raw_reader(...);
 *   Object *w = NULL;
 *   kl_gc_protect(&r, &w);
 *   w = kl_new_raw_writer(...);   // may allocate or park
 *   return obj_value(kl_new_tuple2(r, w));
 *
 * Slots store the ADDRESS of the local, so later assignments are seen by
 * the GC. The frame lives in the C scope and pops automatically on exit,
 * including error returns. Each frame belongs to the current ks, so parked
 * coroutines keep their own roots (gc_mark_roots scans ks->shadow).
 */
typedef struct _ShadowStack {
    struct _ShadowStack *back;
    uint32_t count;
    /* addresses of Object* locals */
    void **slots;
} ShadowStack;

static inline void _kl_gc_ss_pop(ShadowStack **pptr)
{
    ShadowStack *ss = *pptr;
    KoalaState *ks = __ks();
    ASSERT(ks->shadow == ss);
    ks->shadow = ss->back;
}

/* clang-format off */

#define kl_gc_protect(...) \
    void *_ss_slots_##__LINE__[] = { __VA_ARGS__ }; \
    ShadowStack _ss_##__LINE__ = { \
        .back = __ks()->shadow, \
        .count = sizeof(_ss_slots_##__LINE__) / sizeof(void *), \
        .slots = _ss_slots_##__LINE__, \
    }; \
    __attribute__((cleanup(_kl_gc_ss_pop))) \
    ShadowStack *_ss_ptr_##__LINE__ = &_ss_##__LINE__; \
    __ks()->shadow = &_ss_##__LINE__;

/* clang-format on */

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_SHADOW_STACK_H_ */
