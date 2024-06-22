/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_HLIST_H_
#define _KOALA_HLIST_H_

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Double linked lists with a single pointer of list head.
 * Mostly useful for hash tables where the two pointer list head is
 * too wasteful.
 * You lose the ability to access the tail in O(1).
 */
typedef struct _HListNode {
    struct _HListNode *next, **pprev;
} HListNode;

typedef struct _HListHead {
    struct _HListNode *first;
} HListHead;

/* clang-format off */

#define HLIST_HEAD_INIT { .first = NULL }
#define HLIST_HEAD(name) HListHead name = { .first = NULL }
#define init_hlist_head(ptr) ((ptr)->first = NULL)

/* clang-format on */

static inline void init_hlist_node(HListNode *n)
{
    n->next = NULL;
    n->pprev = NULL;
}

static inline int hlist_unhashed(const HListNode *node) { return !node->pprev; }

static inline int hlist_empty(const HListHead *head) { return !head->first; }

static inline void hlist_del(HListNode *n)
{
    if (!hlist_unhashed(n)) {
        HListNode *next = n->next;
        HListNode **pprev = n->pprev;
        *pprev = next;
        if (next) next->pprev = pprev;
        init_hlist_node(n);
    }
}

static inline void hlist_add(HListNode *n, HListHead *h)
{
    HListNode *first = h->first;
    n->next = first;
    if (first) first->pprev = &n->next;
    h->first = n;
    n->pprev = &h->first;
}

/* clang-format off */

#define hlist_for_each(pos, head) \
	for (pos = (head)->first; pos; pos = pos->next)

#define hlist_for_each_safe(pos, n, head) \
	for (pos = (head)->first; pos && ({n = pos->next; 1;}); pos = n)

/* clang-format on */

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_HLIST_H_ */
