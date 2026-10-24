/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_DEQUE_OBJECT_H_
#define _KOALA_DEQUE_OBJECT_H_

#include "object.h"
#include "ring.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _DequeObject {
    OBJECT_HEAD
    /* Elements are `TValue`, stored by value in a circular buffer: the front
     * element is `ring.head` and `ring.size` is the number of live elements.
     * Both ends are strictly O(1), and draining never compacts the buffer.
     * Inserting or removing in the middle is O(n), the same as a list. */
    Ring ring;
    /* Structural modification counter. Iterators snapshot it and raise on
     * mismatch, so an insertion or removal during traversal is reported
     * instead of silently skipping or repeating elements. */
    uint32_t mod_count;
} DequeObject;

extern TypeObject deque_type;
#define IS_DEQUE(ob) IS_TYPE((ob), &deque_type)

extern TypeObject deque_iter_type;
extern TypeObject deque_rev_iter_type;

Object *kl_new_deque(void);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_DEQUE_OBJECT_H_ */