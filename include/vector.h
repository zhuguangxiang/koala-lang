/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2020-2021 James <zhuguangxiang@gmail.com>
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
} Vector, *VectorRef;

/* Initialize an empty vector */
static inline void VectorInit(VectorRef vec, int objsize)
{
    memset(vec, 0, OBJ_SIZE(vec));
    vec->objsize = objsize;
}

#define VectorInitPtr(vec) VectorInit(vec, PTR_SIZE)

/* Finalize an vector */
static inline void VectorFini(VectorRef vec)
{
    if (!vec) return;
    MemFree(vec->objs);
    vec->objs = NULL;
    vec->objsize = 0;
    vec->size = 0;
    vec->capacity = 0;
}

/* Clear a vector, no free memory */
static inline void VectorClear(VectorRef vec)
{
    memset(vec->objs, 0, vec->capacity * vec->objsize);
    vec->size = 0;
}

/* Create a vector */
static inline VectorRef VectorCreate(int objsize)
{
    VectorRef vec = MemAllocWithPtr(vec);
    VectorInit(vec, objsize);
    return vec;
}

/* Destroy a vector */
static inline void VectorDestroy(VectorRef vec)
{
    VectorFini(vec);
    MemFree(vec);
}

/* Get a vector size */
#define VectorSize(vec) ((NULL != (vec)) ? (vec)->size : 0)

/* Test whether a vector is empty */
#define VectorEmpty(vec) (!VectorSize(vec))

/* Get a vector capacity */
#define VectorCapacity(vec) ((NULL != (vec)) ? (vec)->capacity : 0)

/*
 * Store an object at an index. The old will be erased.
 * Index bound is checked.
 */
int VectorSet(VectorRef vec, int index, void *obj);

/* Append an object at the end of the vector. */
static inline void VectorPushBack(VectorRef vec, void *obj)
{
    VectorSet(vec, vec->size, obj);
}

/*
 * Insert an object into the in-bound of the vector.
 * This is relatively expensive operation.
 */
int VectorInsert(VectorRef vec, int index, void *obj);

/*
 * Insert an object at the front of the vector.
 * This is relatively expensive operation.
 */
static inline void VectorPushFront(VectorRef vec, void *obj)
{
    VectorInsert(vec, 0, obj);
}

/*
 * Get an object stored at an index position.
 * Index bound is checked.
 */
int VectorGet(VectorRef vec, int index, void *obj);

/* Get first object */
#define VectorGetFirst(vec, obj) VectorGet(vec, 0, obj)

/* Get last object */
#define VectorGetLast(vec, obj) VectorGet(vec, VectorSize(vec) - 1, obj)

/*
 * Get an object pointer stored at an index position.
 * Index bound is checked.
 */
void *VectorGetPtr(VectorRef vec, int index);

/* Get first object pointer */
#define VectorFirstPtr(vec) VectorGetPtr(vec, 0)

/* Get last object pointer */
#define VectorLastPtr(vec) VectorGetPtr(vec, VectorSize(vec) - 1)

/* Peek an object at the end of the vector, not remove it.*/
static inline void VectorTopBack(VectorRef vec, void *obj)
{
    VectorGet(vec, vec->size - 1, obj);
}

/*
 * Remove an object from the in-bound of the vector.
 * This is relatively expensive operation.
 */
int VectorRemove(VectorRef vec, int index, void *obj);

/*
 * Remove an object at the end of the vector.
 * When used with 'VectorPushBack', the vector can be as a `stack`.
 */
static inline void VectorPopBack(VectorRef vec, void *obj)
{
    VectorRemove(vec, vec->size - 1, obj);
}

/*
 * Remove an object at the front of the vector.
 * This is relatively expensive operation.
 */
static inline void VectorPopFront(VectorRef vec, void *obj)
{
    VectorRemove(vec, 0, obj);
}

// clang-format off

/* iterate vector by pointer */
#define VectorForEach(item, vec, closure) \
    for (int i = 0, len = VectorSize(vec); \
         i < len && (item = VectorGetPtr(vec, i)); i++) closure;

/* iterate vector by pointer reversely */
#define VectorForEachReverse(item, vec, closure) \
    for (int i = VectorSize(vec) - 1; \
         i >= 0 && (item = VectorGetPtr(vec, i)); i--) closure;

// clang-format on

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_VECTOR_H_ */
