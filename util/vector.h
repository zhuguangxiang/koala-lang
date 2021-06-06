/*
 * This file is part of the koala-lang project, under the MIT License.
 *
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
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
    /* object size */
    int objsize;
    /* object memory */
    char *objs;
} Vector;

/* Initialize an empty vector */
static inline void vector_init(Vector *vec, int objsize)
{
    memset(vec, 0, OBJ_SIZE(vec));
    vec->objsize = objsize;
}

#define vector_init_ptr(vec) vector_init(vec, PTR_SIZE)

/* Finalize an vector */
static inline void vector_fini(Vector *vec)
{
    if (!vec) return;
    mm_free(vec->objs);
    vec->objs = nil;
    vec->objsize = 0;
    vec->size = 0;
    vec->capacity = 0;
}

/* Clear a vector, no free memory */
static inline void vector_clear(Vector *vec)
{
    memset(vec->objs, 0, vec->capacity * vec->objsize);
    vec->size = 0;
}

/* Create a vector */
static inline Vector *vector_create(int objsize)
{
    Vector *vec = mm_alloc_obj(vec);
    vector_init(vec, objsize);
    return vec;
}

/* Destroy a vector */
static inline void vector_destroy(Vector *vec)
{
    vector_fini(vec);
    mm_free(vec);
}

/* Get a vector size */
#define vector_size(vec) ((nil != (vec)) ? (vec)->size : 0)

/* Test whether a vector is empty */
#define vector_empty(vec) (!vector_size(vec))

/* Get a vector capacity */
#define vector_capacity(vec) ((nil != (vec)) ? (vec)->capacity : 0)

/*
 * Store an object at an index. The old will be erased.
 * Index bound is checked.
 */
int vector_set(Vector *vec, int index, void *obj);

/* Append an object at the end of the vector. */
static inline void vector_push_back(Vector *vec, void *obj)
{
    vector_set(vec, vec->size, obj);
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
 * Get an object stored at an index position.
 * Index bound is checked.
 */
int vector_get(Vector *vec, int index, void *obj);

/* Get first object */
#define vector_first(vec, obj) vector_get(vec, 0, obj)

/* Get last object */
#define vector_last(vec, obj) vector_get(vec, vector_size(vec) - 1, obj)

/*
 * Get an object pointer stored at an index position.
 * Index bound is checked.
 */
void *vector_get_ptr(Vector *vec, int index);

/* Get first object pointer */
#define vector_first_ptr(vec) vector_get_ptr(vec, 0)

/* Get last object pointer */
#define vector_last_ptr(vec) vector_get_ptr(vec, vector_size(vec) - 1)

/* Peek an object at the end of the vector, not remove it.*/
static inline void vector_top_back(Vector *vec, void *obj)
{
    vector_get(vec, vec->size - 1, obj);
}

/*
 * Remove an object from the in-bound of the vector.
 * This is relatively expensive operation.
 */
int vector_remove(Vector *vec, int index, void *obj);

/*
 * Remove an object at the end of the vector.
 * When used with 'vector_push_back', the vector can be as a `stack`.
 */
static inline void vector_pop_back(Vector *vec, void *obj)
{
    vector_remove(vec, vec->size - 1, obj);
}

/*
 * Remove an object at the front of the vector.
 * This is relatively expensive operation.
 */
static inline void vector_pop_front(Vector *vec, void *obj)
{
    vector_remove(vec, 0, obj);
}

// clang-format off

/* iterate vector by pointer */
#define vector_foreach(item, vec, closure) \
    for (int i = 0, len = vector_size(vec); \
         i < len && (item = vector_get_ptr(vec, i)); i++) closure;

/* iterate vector by pointer reversely */
#define vector_foreach_reverse(item, vec, closure) \
    for (int i = vector_size(vec) - 1; \
         i >= 0 && (item = vector_get_ptr(vec, i)); i--) closure;

// clang-format on

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_VECTOR_H_ */
