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

#define GC_KIND_ARRAY_INT8   1
#define GC_KIND_ARRAY_INT64  2
#define GC_KIND_ARRAY_FLT64  3
#define GC_KIND_ARRAY_OBJECT 4
#define GC_KIND_ARRAY_VALUE  5
#define GC_KIND_OBJECT       6

/* clang-format off */
#define GC_OBJECT_HEAD LLDqNode gc_link; int gc_size; short gc_age; char gc_color; char gc_kind;
/* clang-format on */

typedef struct _GcObject {
    GC_OBJECT_HEAD
} GcObject;

typedef struct _GcArrayObject {
    GC_OBJECT_HEAD
    int gc_num_objs;
} GcArrayObject;

/* clang-format off */
#define GC_OBJECT_INIT(_size, _age, _color) \
    .gc_link = { NULL }, .gc_size = (_size), .gc_age = (_age), .gc_color = (_color)

#define INIT_GC_OBJECT(_obj, _size, _age, _color) \
    lldq_node_init(&(_obj)->gc_link); (_obj)->gc_size = (_size); \
    (_obj)->gc_age = (_age); (_obj)->gc_color = (_color)
/* clang-format on */

extern volatile char *_gc_check_ptr;

/* This will trigger segment fault, if gc has no memory. */
static inline void gc_check_stw(void)
{
    char v = *_gc_check_ptr;
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
void fini_gc_system(void);

void *_gc_alloc(int size, int perm);

#define gc_alloc(size)   _gc_alloc(size, 0)
#define gc_alloc_p(size) _gc_alloc(size, 1)

#define gc_alloc_obj(ptr)   gc_alloc(OBJ_SIZE(ptr))
#define gc_alloc_obj_p(ptr) gc_alloc_p(OBJ_SIZE(ptr))

#define _gc_mark(obj, _color) ((GcObject *)(obj))->gc_color = (_color)

static inline void gc_mark_obj(GcObject *obj, Queue *que)
{
    ASSERT(obj);

    if (obj->gc_age != -1) {
        _gc_mark(obj, GC_COLOR_GRAY);
        queue_push(que, obj);
    }
}

void *gc_alloc_array(char kind, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_GC_H_ */
