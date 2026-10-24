/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "ring.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RING_MINIMUM_CAPACITY 4

/*
 * Allocate a buffer of `capacity` slots and flatten the live range into it,
 * so the first object lands at slot zero.
 */
static int __ring_grow(Ring *ring, int capacity)
{
    char *objs = mm_alloc(capacity * ring->obj_size);

    if (ring->objs && ring->size > 0) {
        int nbytes = ring->obj_size;
        /* the live range may straddle the end of the buffer, copy two pieces */
        int first = MIN(ring->size, ring->capacity - ring->head);
        memcpy(objs, __ring_offset(ring, ring->head), first * nbytes);
        if (ring->size > first) {
            memcpy(objs + first * nbytes, ring->objs, (ring->size - first) * nbytes);
        }
    }

    mm_free(ring->objs);

    ring->objs = objs;
    ring->capacity = capacity;
    ring->head = 0;
    return 0;
}

int __ring_maybe_expand(Ring *ring, int extra)
{
    /* ring has enough room space */
    int size = ring->size + extra;
    if (size <= ring->capacity) return 0;

    int capacity = ring->capacity ? ring->capacity : RING_MINIMUM_CAPACITY;
    while (capacity < size) capacity <<= 1;

    return __ring_grow(ring, capacity);
}

/*
 * Move the live range to the beginning of the buffer, so the objects become
 * contiguous. Only the O(n) operations below need it.
 *
 * When the live range wraps, both pieces have to move and the target of each
 * one overlaps the source of the other, so a plain pair of memmoves would
 * clobber data. The smaller piece is parked in a temporary buffer instead.
 */
static void __ring_flatten(Ring *ring)
{
    if (ring->head == 0 || ring->size == 0) {
        ring->head = 0;
        return;
    }

    int nbytes = ring->obj_size;
    /* the piece in [head, capacity) */
    int first = MIN(ring->size, ring->capacity - ring->head);
    /* the wrapped piece in [0, second) */
    int second = ring->size - first;

    if (second == 0) {
        /* the live range is contiguous already, just slide it down */
        memmove(ring->objs, ring->objs + ring->head * nbytes, first * nbytes);
        ring->head = 0;
        return;
    }

    if (second > first) {
        /* park the front piece, the wrapped piece then lands behind it */
        char *tmp = mm_alloc(first * nbytes);
        memcpy(tmp, ring->objs + ring->head * nbytes, first * nbytes);
        memmove(ring->objs + first * nbytes, ring->objs, second * nbytes);
        memcpy(ring->objs, tmp, first * nbytes);
        mm_free(tmp);
    } else {
        /* park the wrapped piece, the front piece then slides down */
        char *tmp = mm_alloc(second * nbytes);
        memcpy(tmp, ring->objs, second * nbytes);
        memmove(ring->objs, ring->objs + ring->head * nbytes, first * nbytes);
        memcpy(ring->objs + first * nbytes, tmp, second * nbytes);
        mm_free(tmp);
    }

    ring->head = 0;
}

int ring_insert(Ring *ring, int index, void *obj)
{
    if (!ring || index < 0 || index > ring->size) return -1;

    /* may grow, which flattens the ring already */
    if (__ring_maybe_expand(ring, 1)) return -1;

    /* the objects must be contiguous, so one memmove shifts the tail */
    __ring_flatten(ring);

    int nbytes = ring->obj_size;
    char *offset = ring->objs + index * nbytes;

    if (index < ring->size) {
        memmove(offset + nbytes, offset, (ring->size - index) * nbytes);
    }

    memcpy(offset, obj, nbytes);
    ++ring->size;
    return 0;
}

int ring_remove(Ring *ring, int index, void *obj)
{
    if (!ring || index < 0 || index >= ring->size) return -1;

    __ring_flatten(ring);

    int nbytes = ring->obj_size;
    char *offset = ring->objs + index * nbytes;

    if (obj != NULL) memcpy(obj, offset, nbytes);

    if (index + 1 < ring->size) {
        memmove(offset, offset + nbytes, (ring->size - index - 1) * nbytes);
    }

    --ring->size;
    return 0;
}

#ifdef __cplusplus
}
#endif
