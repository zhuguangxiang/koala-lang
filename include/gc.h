/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangXiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_GC_H_
#define _KOALA_GC_H_

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 *           |------|    |------|    |------|
 * gcroot -> |  2   |    |  3   |    |  1   |
 *           | prev | -> | prev | -> | NULL |
 *           | &obj |    | &obj |    | &obj |
 *           | &obj |    | &obj |    |------|
 *           |------|    | &obj |
 *                       |------|
 *
 * NOTE: it is the caller's responsibility to make sure arguments are
 * rooted. foo(f(), g()) will not work, and foo can't do anything about it,
 * so the caller must do it below:
 *
 * Object *x = NULL, *y = NULL;
 * gc_push(2, &x, &y);
 * x = f();
 * y = g();
 * foo(x, y);
 * gc_pop();
 *
 */
typedef struct GcTrace {
    uintptr_t nroots;
    struct GcTrace *prev;
    /* actual roots go here */
} GcTrace;

extern GcTrace *gcroots;

/* count of __VA_ARGS__ */
#define VA_NARGS(type, ...) \
    ((type)(sizeof((type[]) { __VA_ARGS__ }) / sizeof(type)))

/* save traced objects in func frame */
#define gc_push(...)                   \
    void *__gc_stkf[] = {              \
        VA_NARGS(void *, __VA_ARGS__), \
        gcroots,                       \
        __VA_ARGS__,                   \
    };                                 \
    gcroots = (GcTrace *)__gc_stkf;

/* remove traced objects from func frame */
#define gc_pop() (gcroots = gcroots->prev)

#define ROOT_TRACE(name, ...)          \
    static void *__##name##__[] = {    \
        VA_NARGS(void *, __VA_ARGS__), \
        NULL,                          \
        __VA_ARGS__,                   \
    };

/* add root */
#define gc_add_root(name)                          \
    do {                                           \
        ((GcTrace *)__##name##__)->prev = gcroots; \
        gcroots = (GcTrace *)__##name##__;         \
    } while (0)

/* objmap is used by Object or raw struct */
typedef struct objmap {
    int num;
    int offset[0];
} objmap_t;

#define OBJECT_MAP(name, ...)       \
    static int __##name##__[] = {   \
        VA_NARGS(int, __VA_ARGS__), \
        __VA_ARGS__,                \
    }

/* object finalized func for close resource */
typedef void (*gc_fini_func)(void *);

/* allocated object(struct) layout memory */
void *gc_alloc_object(int size, void *objmap, gc_fini_func fini);

/* allocate raw memory(no object layout) */
void *gc_alloc(int size);

/* start to gc */
void gc(void);

/* initialize gc */
void gc_init(void);

/* finalize gc */
void gc_fini(void);

/* gc traced array */
typedef struct GcArray {
    /* elem ptr */
    void *ptr;
    /* elem count */
    int len;
    /* elem is obj */
    int isobj;
    /* nested elems */
    char data[0];
} GcArray;

/* allocate array in gc */
GcArray *gc_alloc_array(int len, int own_buf, int isobj);

/* expand exist array in gc */
GcArray *gc_expand_array(GcArray *arr, int newlen);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_GC_H_ */
