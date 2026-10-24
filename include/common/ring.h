/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_RING_H_
#define _KOALA_RING_H_

#include "common.h"
#include "mm.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Ring is a double-ended circular buffer of fixed-size objects.
 *
 * The `index`-th live object sits in slot `(head + index) & (capacity - 1)`,
 * so `capacity` is always zero or a power of two. Both ends are strictly
 * O(1): pushing and popping only touch one slot and never move the other
 * objects. Growth flattens the live range back to the beginning of a new,
 * larger buffer. Inserting or removing in the middle is O(n), it flattens the
 * ring first so the tail can be shifted with a single memmove.
 */
typedef struct _Ring {
    /* object memory */
    char *objs;
    /* total slots, zero or a power of two */
    int capacity;
    /* used slots */
    int size;
    /* index of the first object */
    int head;
    /* object size */
    int obj_size;
    /* padding */
    int _padding;
} Ring;

/* Initialize an empty ring */
static inline void ring_init(Ring *ring, int obj_size)
{
    ring->objs = NULL;
    ring->capacity = 0;
    ring->size = 0;
    ring->head = 0;
    ring->obj_size = obj_size;
    ring->_padding = 0;
}

#define RING_INIT_PTR { NULL, 0, 0, 0, PTR_SIZE, 0 }

/* Finalize a ring */
static inline void ring_fini(Ring *ring)
{
    if (!ring) return;
    ring->capacity = 0;
    ring->size = 0;
    ring->head = 0;
    ring->obj_size = 0;
    ring->_padding = 0;
    mm_free(ring->objs);
    ring->objs = NULL;
}

/* Clear a ring, no free memory */
static inline void ring_clear(Ring *ring)
{
    ring->size = 0;
    ring->head = 0;
}

/* Create a ring */
static inline Ring *ring_create(int obj_size)
{
    Ring *ring = mm_alloc_obj_fast(ring);
    ring_init(ring, obj_size);
    return ring;
}

/* Destroy a ring */
static inline void ring_destroy(Ring *ring)
{
    if (!ring) return;
    ring_fini(ring);
    mm_free(ring);
}

/* Get a ring size */
#define ring_size(ring) ((NULL != (ring)) ? (ring)->size : 0)

/* Test whether a ring is empty */
#define ring_empty(ring) (!ring_size(ring))

/* Get a ring capacity */
#define ring_capacity(ring) ((NULL != (ring)) ? (ring)->capacity : 0)

/* internal: slot mask, capacity must be a power of two */
static inline int __ring_mask(Ring *ring) { return ring->capacity - 1; }

/* internal: memory offset of slot `index` */
static inline char *__ring_offset(Ring *ring, int index)
{
    return ring->objs + ring->obj_size * index;
}

/* internal: real slot index of the `index`-th live object */
static inline int __ring_index(Ring *ring, int index)
{
    return (ring->head + index) & __ring_mask(ring);
}

/* expand the ring if needed */
int __ring_maybe_expand(Ring *ring, int extra);

/* Reserve room for `size` more objects, the ring size is unchanged. */
static inline void ring_reserve(Ring *ring, int size) { __ring_maybe_expand(ring, size); }

/*
 * Get an object pointer(the slot index as object pointer).
 * `index` is relative to the front, index bound is checked.
 */
static inline void *ring_get_ptr(Ring *ring, int index)
{
    /* not set any object */
    if (!ring || !ring->objs) return NULL;

    /* valid range is (0 ..< size) */
    if (index < 0 || index >= ring->size) return NULL;

    return __ring_offset(ring, __ring_index(ring, index));
}

/*
 * Insert `obj` at `index`, which is relative to the front and may equal the
 * ring size, in which case the object is appended. O(n): the ring is flattened
 * first, so the tail can be shifted with a single memmove.
 * Return 0 on success, -1 if `index` is out of range.
 */
int ring_insert(Ring *ring, int index, void *obj);

/*
 * Remove the object at `index` into `obj`, which may be NULL. O(n), for the
 * same reason as `ring_insert`. Return 0 on success, -1 if out of range.
 */
int ring_remove(Ring *ring, int index, void *obj);

/* Get first object pointer */
static inline void *ring_first(Ring *ring) { return ring_get_ptr(ring, 0); }

/* Get last object pointer */
static inline void *ring_last(Ring *ring) { return ring_get_ptr(ring, ring_size(ring) - 1); }

/* Append an object at the back. O(1) amortized. */
static inline void *ring_push_back(Ring *ring, void *obj)
{
    /* try to expand the ring */
    if (__ring_maybe_expand(ring, 1)) return NULL;

    char *offset = __ring_offset(ring, __ring_index(ring, ring->size));
    memcpy(offset, obj, ring->obj_size);
    ++ring->size;
    return offset;
}

/* Prepend an object at the front. O(1) amortized. */
static inline void *ring_push_front(Ring *ring, void *obj)
{
    /* try to expand the ring */
    if (__ring_maybe_expand(ring, 1)) return NULL;

    /* `+ capacity - 1` instead of `- 1`, so the operand stays positive */
    ring->head = (ring->head + ring->capacity - 1) & __ring_mask(ring);

    char *offset = __ring_offset(ring, ring->head);
    memcpy(offset, obj, ring->obj_size);
    ++ring->size;
    return offset;
}

/*
 * Remove an object at the front into `obj`, which may be NULL.
 * Return 0 on success, -1 if the ring is empty.
 */
static inline int ring_pop_front(Ring *ring, void *obj)
{
    if (!ring || ring->size == 0) return -1;

    char *offset = __ring_offset(ring, ring->head);
    if (obj != NULL) memcpy(obj, offset, ring->obj_size);

    ring->head = (ring->head + 1) & __ring_mask(ring);
    --ring->size;
    return 0;
}

/*
 * Remove an object at the back into `obj`, which may be NULL.
 * Return 0 on success, -1 if the ring is empty.
 */
static inline int ring_pop_back(Ring *ring, void *obj)
{
    if (!ring || ring->size == 0) return -1;

    char *offset = __ring_offset(ring, __ring_index(ring, ring->size - 1));
    if (obj != NULL) memcpy(obj, offset, ring->obj_size);

    --ring->size;
    return 0;
}

/* Peek the first object, not remove it. NULL if empty. */
static inline void *ring_peek_front(Ring *ring)
{
    if (!ring || ring->size == 0) return NULL;
    return __ring_offset(ring, ring->head);
}

/* Peek the last object, not remove it. NULL if empty. */
static inline void *ring_peek_back(Ring *ring)
{
    if (!ring || ring->size == 0) return NULL;
    return __ring_offset(ring, __ring_index(ring, ring->size - 1));
}

/* clang-format off */

/* iterate ring, deletion is unsafe
return item's pointer
*/
#define ring_foreach_ptr(obj_ptr, ring) \
    for (int i__ = 0; (obj_ptr = ring_get_ptr(ring, i__)); ++i__)

/* clang-format on */

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_RING_H_ */
