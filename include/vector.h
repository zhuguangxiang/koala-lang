/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangxiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_VECTOR_H_
#define _KOALA_VECTOR_H_

#include "mm.h"
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A dynamic array */
typedef struct Vector {
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
    memset(vec, 0, sizeof(*vec));
    vec->objsize = objsize;
}

/* Finalize an vector */
static inline void vector_fini(Vector *vec)
{
    if (!vec) return;
    mm_free(vec->objs);
    vec->objs = NULL;
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
static inline Vector *vector_new(int objsize)
{
    Vector *vec = mm_alloc(sizeof(*vec));
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
#define vector_size(vec) ((vec) ? (vec)->size : 0)

/* Check a vector is empty or not */
#define vector_empty(vec) (!vector_size(vec))

/* Get a vector capacity */
#define vector_capacity(vec) ((vec) ? (vec)->capacity : 0)

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
#define vector_get_first(vec, obj) vector_get(vec, 0, obj)

/* Get last object */
#define vector_get_last(vec, obj) vector_get(vec, vector_size(vec) - 1, obj)

/*
 * Get an object pointer stored at an index position.
 * Index bound is checked.
 */
void *vector_get_ptr(Vector *vec, int index);

/* Get first object pointer */
#define vector_get_ptr_first(vec) vector_get_ptr(vec, 0)

/* Get last object pointer */
#define vector_get_ptr_last(vec) vector_get_ptr(vec, vector_size(vec) - 1)

/* Peek an object at the end of the vector, not remove it.*/
static inline void vector_top_back(Vector *vec, void *obj)
{
    vector_get(vec, vec->size - 1, obj);
}

/*
 * Remove an object at the end of the vector.
 * When used with 'vector_push_back', the vector can be as a `stack`.
 */
static inline void vector_pop_back(Vector *vec, void *obj)
{
    vector_get(vec, vec->size - 1, obj);
    vec->size--;
}

/*
 * Remove an object from the in-bound of the vector.
 * This is relatively expensive operation.
 */
int vector_remove(Vector *vec, int index, void *obj);

/*
 * Remove an object at the front of the vector.
 * This is relatively expensive operation.
 */
static inline void vector_pop_front(Vector *vec, void *obj)
{
    vector_remove(vec, 0, obj);
}

/* iterate for vector by pointer */
#define vector_foreach(item, vec)                                            \
    if (vec)                                                                 \
        for (int i = 0, len = (vec)->size;                                   \
             i < len && (item = (void *)((vec)->objs + i * (vec)->objsize)); \
             i++)

/* iterate for vector reversely by pointer */
#define vector_foreach_reverse(item, vec)                                   \
    if (vec)                                                                \
        for (int i = (vec)->size - 1;                                       \
             i >= 0 && (item = (void *)((vec)->objs + i * (vec)->objsize)); \
             i--)

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_VECTOR_H_ */
