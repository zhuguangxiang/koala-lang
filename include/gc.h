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
#define GC_OBJECT_HEAD LLDqNode link; GcMarkFunc mark; int size; \
    short age; short color;
/* clang-format on */

typedef struct _GcHdr {
    GC_OBJECT_HEAD
} GcHdr;

/* clang-format off */
#define GC_HEAD_INIT(hdr, _mark, _size, _age, _color) \
    .hdr = { .link = { NULL }, .mark = (_mark), .size = (_size), .age = (_age), .color = (_color), }
/* clang-format on */

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

void init_gc_system(size_t max_mem_size, double load_factor);
void *gc_alloc(int size, GcMarkFunc mark);
#define gc_mark(obj, _color) ((GcHdr *)(obj))->color = (_color)

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_GC_H_ */