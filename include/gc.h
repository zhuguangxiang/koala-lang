/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_GC_H_
#define _KOALA_GC_H_

#include "lldq.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*GcMarkFunc)(void **);

#define GC_COLOR_WHITE 1
#define GC_COLOR_GRAY  2
#define GC_COLOR_BLACK 3

/* clang-format off */
#define GC_HEAD LLDqNode gc_link; int gc_size; short gc_color; short gc_age;
/* clang-format on */

typedef struct _GcHdr {
    GC_HEAD
} GcHdr;

/* clang-format off */
#define GC_HEAD_INIT(_size, _color, _age) \
    .gc_link = { NULL }, .gc_size = (_size), .gc_color = (_color), .gc_age = (_age)

#define INIT_GC_HEAD(hdr, _size, _color, _age) \
    (hdr).gc_size = (_size); (hdr).gc_color = (_color); (hdr).gc_age = (_age)
/* clang-format on */

#define GC_ARRAY_INT8    1
#define GC_ARRAY_INT16   2
#define GC_ARRAY_INT32   3
#define GC_ARRAY_INT64   4
#define GC_ARRAY_FLOAT32 5
#define GC_ARRAY_FLOAT64 6
#define GC_ARRAY_OBJECT  7
#define GC_ARRAY_VALUE   8

typedef struct _GcArray {
    GC_HEAD
    /* one of GC_ARRAY_XXX */
    int kind;
    /* real data */
    char data[0];
} GcArray;

extern volatile char *__gc_check_ptr;

/* This will trigger segment fault, if gc has no memory. */
static inline void gc_check_stw(void)
{
    char v = *__gc_check_ptr;
    UNUSED(v);
}

typedef enum _GcState {
    GC_DONE,
    GC_MARK_ROOTS,
    GC_CO_MARK,
    GC_REMARK,
    GC_CO_SWEEP,
    GC_FULL,
} GcState;

void init_gc_system(size_t max_mem_size, double factor);
void *_gc_alloc(int size, int perm);
static inline void *gc_alloc(int size) { return _gc_alloc(size, 0); }
static inline void *gc_alloc_perm(int size) { return _gc_alloc(size, 1); }
#define gc_mark(obj, _color) ((GcHdr *)(obj))->gc_color = (_color)

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_GC_H_ */
