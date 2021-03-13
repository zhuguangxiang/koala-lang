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

#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VECTOR_MINIMUM_CAPACITY 8

static inline int __maybe_expand(VectorRef vec, int extra)
{
    /* vector has enough room space */
    int size = vec->size + extra;
    if (size <= vec->capacity) return 0;

    int cap;
    if (vec->capacity)
        cap = vec->capacity << 1;
    else
        cap = VECTOR_MINIMUM_CAPACITY;

    char *objs = (char *)mm_alloc(cap * vec->objsize);
    if (vec->objs) {
        memcpy(objs, vec->objs, vec->size * vec->objsize);
        mm_free(vec->objs);
    }

    vec->objs = objs;
    vec->capacity = cap;
    return 0;
}

static inline char *__offset(VectorRef vec, int index)
{
    return vec->objs + vec->objsize * index;
}

int vector_set(VectorRef vec, int index, void *obj)
{
    /*
     * valid range is (0 ... size)
     * if index equals vector size, it's 'append' operation.
     */
    if (index < 0 || index > vec->size) return -1;

    /* try to expand the vector */
    if (__maybe_expand(vec, 1)) return -1;

    void *offset = __offset(vec, index);
    memcpy(offset, obj, vec->objsize);
    if (index == vec->size) vec->size++;
    return 0;
}

void *vector_get_ptr(VectorRef vec, int index)
{
    /* not set any object */
    if (!vec->objs) return NULL;

    /* valid range is (0 ..< size) */
    if (index < 0 || index >= vec->size) return NULL;

    return __offset(vec, index);
}

int vector_get(VectorRef vec, int index, void *obj)
{
    /* not set any object */
    if (!vec->objs) return -1;

    /* valid range is (0 ..< size) */
    if (index < 0 || index >= vec->size) return -1;

    void *offset = __offset(vec, index);
    memcpy(obj, offset, vec->objsize);
    return 0;
}

/* move one object to right */
static void __move_to_right(VectorRef vec, int index)
{
    /* the location to start to move */
    char *from = __offset(vec, index);

    /* the destination to move(one object) */
    char *to = from + vec->objsize;

    /* how many bytes to be moved to the right */
    int nbytes = (vec->size - index) * vec->objsize;

    /* NOTES: use memmove instead of memcpy */
    memmove(to, from, nbytes);
}

int vector_insert(VectorRef vec, int index, void *obj)
{
    /*
     * valid range is (0 ... size)
     * if index equals vector size, it's 'append' operation.
     */
    if (index < 0 || index > vec->size) return -1;

    /* try to expand the vector */
    if (__maybe_expand(vec, 1)) return -1;

    void *offset = __offset(vec, index);
    __move_to_right(vec, index);
    memcpy(offset, obj, vec->objsize);
    vec->size++;
    return 0;
}

static void __move_to_left(VectorRef vec, int index)
{
    /* the destination to move(one object) */
    char *to = __offset(vec, index);

    /* the location to start to move */
    char *from = to + vec->objsize;

    /* how many bytes to be moved to the left */
    int nbytes = (vec->size - index - 1) * vec->objsize;

    /* NOTES: use memmove instead of memcpy */
    memmove(to, from, nbytes);
}

int vector_remove(VectorRef vec, int index, void *obj)
{
    /* valid range is (0 ..< size) */
    if (index < 0 || index >= vec->size) return -1;

    void *offset = __offset(vec, index);
    memcpy(obj, offset, vec->objsize);
    __move_to_left(vec, index);
    vec->size--;
    return 0;
}

#ifdef __cplusplus
}
#endif
