/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_GC_H_
#define _KOALA_GC_H_

#include "lldq.h"
#include "queue.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GC_COLOR_WHITE 1
#define GC_COLOR_GRAY  2
#define GC_COLOR_BLACK 3

#define GC_KIND_INT8   1
#define GC_KIND_INT16  2
#define GC_KIND_INT32  3
#define GC_KIND_INT64  4
#define GC_KIND_FLT32  5
#define GC_KIND_FLT64  6
#define GC_KIND_OBJECT 7
#define GC_KIND_VALUE  8
#define GC_KIND_RAW    9

/* clang-format off */
#define GC_HEAD LLDqNode gc_link; int gc_size; short gc_age; char gc_color; char gc_kind;
/* clang-format on */

typedef struct _GcHdr {
    GC_HEAD
} GcHdr;

/* clang-format off */
#define GC_HEAD_INIT(_size, _age, _color) \
    .gc_link = { NULL }, .gc_size = (_size), .gc_age = (_age), .gc_color = (_color)

#define INIT_GC_HEAD(hdr, _size, _age, _color) \
    (hdr)->gc_size = (_size); (hdr)->gc_age = (_age); (hdr)->gc_color = (_color)
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

void init_gc_system(size_t max_mem_size, double factor);
void *_gc_alloc(int size, int perm);

#define gc_alloc(size)   _gc_alloc(size, 0)
#define gc_alloc_p(size) _gc_alloc(size, 1)

#define gc_alloc_obj(ptr)   gc_alloc(OBJ_SIZE(ptr))
#define gc_alloc_obj_p(ptr) gc_alloc_p(OBJ_SIZE(ptr))

#define _gc_mark(obj, _color) ((GcHdr *)(obj))->gc_color = (_color)

static inline void gc_mark_obj(GcHdr *hdr, Queue *que)
{
    if (hdr->gc_age != -1) {
        _gc_mark(hdr, GC_COLOR_GRAY);
        queue_push(que, hdr);
    }
}

void *gc_alloc_array(char kind, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_GC_H_ */
