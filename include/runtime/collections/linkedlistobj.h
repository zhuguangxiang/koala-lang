/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_LINKEDLIST_OBJECT_H_
#define _KOALA_LINKEDLIST_OBJECT_H_

#include "list.h"
#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _LinkedListObject {
    OBJECT_HEAD
    /* Sentinel: `head.next` is the first node, `head.prev` is the last one.
     * An empty list has `head.next == head.prev == &head`. */
    List head;
    /* Cached length, so len()/empty()/index normalization are O(1). */
    size_t len;
    /* Structural modification counter. Iterators snapshot it and raise on
     * mismatch, turning "use after free" into a clean panic. */
    uint32_t mod_count;
} LinkedListObject;

extern TypeObject linkedlist_type;
#define IS_LINKEDLIST(ob) IS_TYPE((ob), &linkedlist_type)

extern TypeObject linkedlist_iter_type;
extern TypeObject linkedlist_rev_iter_type;

Object *kl_new_linkedlist(void);

/* Append `item` to the tail. O(1). */
void kl_linkedlist_append(Object *ob, TValue item);

int kl_linkedlist_len(Object *ob);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_LINKEDLIST_OBJECT_H_ */