/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VECTOR_MINIMUM_CAPACITY 4

static inline int __maybe_expand(Vector *vec, int extra)
{
    /* vector has enough room space */
    int size = vec->size + extra;
    if (size <= vec->capacity) return 0;

    int cap;
    if (vec->capacity) {
        cap = vec->capacity << 1;
    } else {
        cap = VECTOR_MINIMUM_CAPACITY;
    }

    void *objs = mm_alloc(cap * vec->obj_size);
    if (vec->objs) {
        memcpy(objs, vec->objs, vec->size * vec->obj_size);
        mm_free(vec->objs);
    }

    vec->objs = (char *)objs;
    vec->capacity = cap;
    return 0;
}

static inline char *__offset(Vector *vec, int index)
{
    return vec->objs + vec->obj_size * index;
}

int vector_set(Vector *vec, int index, void *obj)
{
    /*
     * valid range is (0 ... size)
     * if index equals vector size, it's 'append' operation.
     */
    if (index < 0 || index > vec->size) return -1;

    /* try to expand the vector */
    if (__maybe_expand(vec, 1)) return -1;

    char *offset = __offset(vec, index);
    memcpy(offset, obj, vec->obj_size);
    if (index == vec->size) vec->size++;
    return 0;
}

void *vector_get(Vector *vec, int index)
{
    /* not set any object */
    if (!vec || !vec->objs) return NULL;

    /* valid range is (0 ..< size) */
    if (index < 0 || index >= vec->size) return NULL;

    return __offset(vec, index);
}

/* move one object to right */
static void __move_to_right(Vector *vec, int index)
{
    /* the location to start to move */
    char *from = __offset(vec, index);

    /* the destination to move(one object) */
    char *to = from + vec->obj_size;

    /* how many bytes to be moved to the right */
    int nbytes = (vec->size - index) * vec->obj_size;

    /* NOTES: use memmove instead of memcpy */
    memmove(to, from, nbytes);
}

int vector_insert(Vector *vec, int index, void *obj)
{
    /*
     * valid range is (0 ... size)
     * if index equals vector size, it's 'append' operation.
     */
    if (index < 0 || index > vec->size) return -1;

    /* try to expand the vector */
    if (__maybe_expand(vec, 1)) return -1;

    char *offset = __offset(vec, index);
    __move_to_right(vec, index);
    memcpy(offset, obj, vec->obj_size);
    vec->size++;
    return 0;
}

static void __move_to_left(Vector *vec, int index)
{
    /* the destination to move(one object) */
    char *to = __offset(vec, index);

    /* the location to start to move */
    char *from = to + vec->obj_size;

    /* how many bytes to be moved to the left */
    int nbytes = (vec->size - index - 1) * vec->obj_size;

    /* NOTES: use memmove instead of memcpy */
    memmove(to, from, nbytes);
}

int vector_remove(Vector *vec, int index, void *obj)
{
    /* valid range is (0 ..< size) */
    if (index < 0 || index >= vec->size) return -1;

    char *offset = __offset(vec, index);
    if (obj != NULL) memcpy(obj, offset, vec->obj_size);
    __move_to_left(vec, index);
    vec->size--;
    return 0;
}

int vector_concat(Vector *to, Vector *from)
{
    void *obj;
    vector_foreach(obj, from) {
        vector_push_back(to, &obj);
    }
    return 0;
}

#ifdef __cplusplus
}
#endif
