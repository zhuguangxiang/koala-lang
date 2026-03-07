/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
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
    int obj_size;
    /* object memory */
    char *objs;
} Vector;

/* Initialize an empty vector */
static inline void vector_init(Vector *vec, int obj_size)
{
    vec->capacity = 0;
    vec->size = 0;
    vec->obj_size = obj_size;
    vec->objs = NULL;
}

#define VECTOR_INIT_PTR { 0, 0, PTR_SIZE, NULL }

#define vector_init_ptr(vec) vector_init(vec, PTR_SIZE)

/* Finalize an vector */
static inline void vector_fini(Vector *vec)
{
    if (!vec) return;
    mm_free(vec->objs);
}

/* Clear a vector, no free memory */
static inline void vector_clear(Vector *vec) { vec->size = 0; }

static inline void vector_clear_to_end(Vector *vec, int index)
{
    if (index < 0 || index >= vec->size) return;
    vec->size = index;
}

/* Create a vector */
static inline Vector *vector_create(int obj_size)
{
    Vector *vec = mm_alloc_obj_fast(vec);
    vector_init(vec, obj_size);
    return vec;
}

/* Create a pointer vector */
#define vector_create_ptr() vector_create(PTR_SIZE)

/* Destroy a vector */
static inline void vector_destroy(Vector *vec)
{
    if (!vec) return;
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

/* internal use */
static inline char *__vector_offset(Vector *vec, int index)
{
    return vec->objs + vec->obj_size * index;
}

/*
 * Get an object pointer(the index position as object pointer)
 * Index bound is checked.
 */
static inline void *vector_get_ptr(Vector *vec, int index)
{
    /* not set any object */
    if (!vec || !vec->objs) return NULL;

    /* valid range is (0 ..< size) */
    if (index < 0 || index >= vec->size) return NULL;

    return __vector_offset(vec, index);
}

/* Get first object pointer */
#define vector_first(vec) vector_get_ptr(vec, 0)

/* Get last object pointer */
#define vector_last(vec) vector_get_ptr(vec, vector_size(vec) - 1)

/* Peek an object at the end of the vector, not remove it.*/
static inline void *vector_top_back(Vector *vec)
{
    return vector_get_ptr(vec, vector_size(vec) - 1);
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
    vector_remove(vec, vector_size(vec) - 1, obj);
}

/*
 * Remove an object at the front of the vector.
 * This is relatively expensive operation.
 */
static inline void vector_pop_front(Vector *vec, void *obj)
{
    vector_remove(vec, 0, obj);
}

/* clang-format off */

/* iterate vector, deletion is unsafe
return item's self value
*/
#define vector_foreach(obj, vec) \
    for (int i__ = 0, n__ = vector_size(vec); i__ < n__; i__++) \
        if (({ typeof(obj) *p__ = vector_get_ptr(vec, i__); (obj = *p__); 1; }))

/* iterate vector, deletion is unsafe
return item's pointer
*/
#define vector_foreach_ptr(obj_ptr, vec) \
    for (int i__ = 0; (obj_ptr = vector_get_ptr(vec, i__)); ++i__)

/* iterate vector(pointer saved) in reverse order, deletion is unsafe */
#define vector_foreach_reverse(obj, vec) \
    for (int i__ = vector_size(vec) - 1; \
        (obj = vector_get_ptr(vec, i__)); --i__)

/* clang-format on */

/* concat two vector(pointer saved) */
static inline int vector_concat(Vector *to, Vector *from)
{
    void **obj_p;
    vector_foreach_ptr(obj_p, from) {
        vector_push_back(to, obj_p);
    }
    return 0;
}

/* get item self value from vector */
static inline void *vector_get(Vector *vec, int index)
{
    void **obj_p = (void **)vector_get_ptr(vec, index);
    return obj_p ? *obj_p : NULL;
}

static inline void vector_sort(Vector *vec, int (*cmp)(const void *, const void *))
{
    qsort(vec->objs, vec->size, vec->obj_size, cmp);
}

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_VECTOR_H_ */
