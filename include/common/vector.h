/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_VECTOR_H_
#define _KOALA_VECTOR_H_

#include "common.h"
#include "mm.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _Vector {
    /* total slots */
    int capacity;
    /* used slots */
    int size;
    /* object memory */
    void **objs;
} Vector;

/* Initialize an empty vector */
static inline void vector_init(Vector *vec)
{
    vec->capacity = 0;
    vec->size = 0;
    vec->objs = NULL;
}

/* Finalize an vector */
static inline void vector_fini(Vector *vec)
{
    if (!vec) return;
    mm_free(vec->objs);
    vector_init(vec);
}

/* Clear a vector, no free memory */
static inline void vector_clear(Vector *vec) { vec->size = 0; }

/* Create a vector */
static inline Vector *vector_create(void)
{
    Vector *vec = mm_alloc_obj_fast(vec);
    vector_init(vec);
    return vec;
}

/* Destroy a vector */
static inline void vector_destroy(Vector *vec)
{
    vector_fini(vec);
    mm_free(vec);
}

/* Get a vector size */
#define vector_size(vec) ((NULL != (vec)) ? (vec)->size : 0)

/* Test whether a vector is empty */
#define vector_empty(vec) (!vector_size(vec))

/* Get a vector capacity */
#define vector_capacity(vec) ((NULL != (vec)) ? (vec)->capacity : 0)

/*
 * Store an object at an index. The old will be erased.
 * Index bound is checked.
 */
int vector_set(Vector *vec, int index, void *obj);

/* Append an object at the end of the vector. */
static inline void vector_push_back(Vector *vec, void *obj)
{
    vector_set(vec, vector_size(vec), obj);
}

/*
 * Insert an object into the in-bound of the vector.
 * This is relatively expensive operation.
 */
int vector_insert(Vector *vec, int index, void *obj);

/*
 * Insert an object at the front of the vector.
 * This is relatively expensive operation.
 */
static inline void vector_push_front(Vector *vec, void *obj)
{
    vector_insert(vec, 0, obj);
}

/*
 * Get an object pointer(the index position as object pointer)
 * Index bound is checked.
 */
void *vector_get(Vector *vec, int index);

/* Get first object pointer */
#define vector_first(vec) vector_get(vec, 0)

/* Get last object pointer */
#define vector_last(vec) vector_get(vec, vector_size(vec) - 1)

/* Peek an object at the end of the vector, not remove it.*/
static inline void *vector_top_back(Vector *vec)
{
    return vector_get(vec, vector_size(vec) - 1);
}

/*
 * Remove an object from the in-bound of the vector.
 * This is relatively expensive operation.
 */
void *vector_remove(Vector *vec, int index);

/*
 * Remove an object at the end of the vector.
 * When used with 'vector_push_back', the vector can be as a `stack`.
 */
static inline void *vector_pop_back(Vector *vec)
{
    return vector_remove(vec, vector_size(vec) - 1);
}

/*
 * Remove an object at the front of the vector.
 * This is relatively expensive operation.
 */
static inline void *vector_pop_front(Vector *vec)
{
    return vector_remove(vec, 0);
}

/* clang-format off */

/* iterate vector(pointer saved), deletion is unsafe */
#define vector_foreach(obj, vec) \
    for (int i__ = 0; (obj = vector_get(vec, i__)); ++i__)

/* iterate vector(pointer saved) in reverse order, deletion is unsafe */
#define vector_foreach_reverse(obj, vec) \
    for (int i__ = vector_size(vec) - 1; \
        (obj = vector_get(vec, i__)); --i__)

/* clang-format on */

/* concat two vector(pointer saved) */
int vector_concat(Vector *to, Vector *from);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_VECTOR_H_ */
