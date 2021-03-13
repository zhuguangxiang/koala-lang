/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _KOALA_VECTOR_H_
#define _KOALA_VECTOR_H_

#include "common.h"

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
} Vector, *VectorRef;

/* Initialize an empty vector */
static inline void vector_init(VectorRef vec, int objsize)
{
    memset(vec, 0, sizeof(*vec));
    vec->objsize = objsize;
}

/* Finalize an vector */
static inline void vector_fini(VectorRef vec)
{
    if (!vec) return;
    free(vec->objs);
    vec->objs = NULL;
    vec->objsize = 0;
    vec->size = 0;
    vec->capacity = 0;
}

/* Clear a vector, no free memory */
static inline void vector_clear(VectorRef vec)
{
    memset(vec->objs, 0, vec->capacity * vec->objsize);
    vec->size = 0;
}

/* Create a vector */
static inline VectorRef vector_new(int objsize)
{
    VectorRef vec = malloc(sizeof(*vec));
    vector_init(vec, objsize);
    return vec;
}

/* Destroy a vector */
static inline void vector_destroy(VectorRef vec)
{
    vector_fini(vec);
    free(vec);
}

/* Get a vector size */
#define vector_size(vec) ((vec) ? (vec)->size : 0)

/* Test whether a vector is empty */
#define vector_empty(vec) (!vector_size(vec))

/* Get a vector capacity */
#define vector_capacity(vec) ((vec) ? (vec)->capacity : 0)

/*
 * Store an object at an index. The old will be erased.
 * Index bound is checked.
 */
int vector_set(VectorRef vec, int index, void *obj);

/* Append an object at the end of the vector. */
static inline void vector_push_back(VectorRef vec, void *obj)
{
    vector_set(vec, vec->size, obj);
}

/*
 * Insert an object into the in-bound of the vector.
 * This is relatively expensive operation.
 */
int vector_insert(VectorRef vec, int index, void *obj);

/*
 * Insert an object at the front of the vector.
 * This is relatively expensive operation.
 */
static inline void vector_push_front(VectorRef vec, void *obj)
{
    vector_insert(vec, 0, obj);
}

/*
 * Get an object stored at an index position.
 * Index bound is checked.
 */
int vector_get(VectorRef vec, int index, void *obj);

/* Get first object */
#define vector_get_first(vec, obj) vector_get(vec, 0, obj)

/* Get last object */
#define vector_get_last(vec, obj) vector_get(vec, vector_size(vec) - 1, obj)

/*
 * Get an object pointer stored at an index position.
 * Index bound is checked.
 */
void *vector_get_ptr(VectorRef vec, int index);

/* Get first object pointer */
#define vector_first_ptr(vec) vector_get_ptr(vec, 0)

/* Get last object pointer */
#define vector_last_ptr(vec) vector_get_ptr(vec, vector_size(vec) - 1)

/* Peek an object at the end of the vector, not remove it.*/
static inline void vector_top_back(VectorRef vec, void *obj)
{
    vector_get(vec, vec->size - 1, obj);
}

/*
 * Remove an object from the in-bound of the vector.
 * This is relatively expensive operation.
 */
int vector_remove(VectorRef vec, int index, void *obj);

/*
 * Remove an object at the end of the vector.
 * When used with 'vector_push_back', the vector can be as a `stack`.
 */
static inline void vector_pop_back(VectorRef vec, void *obj)
{
    vector_remove(vec, vec->size - 1, obj);
}

/*
 * Remove an object at the front of the vector.
 * This is relatively expensive operation.
 */
static inline void vector_pop_front(VectorRef vec, void *obj)
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
