/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2023 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_LOCKED_LINKED_DEQUE_H_
#define _KOALA_LOCKED_LINKED_DEQUE_H_

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LLDQ_NODE_HEAD struct _LLDqNode *next;

typedef struct _LLDqNode {
    LLDQ_NODE_HEAD
} LLDqNode;

typedef struct _LLDeque {
    LLDqNode *head;
    LLDqNode *tail;
    int count;
    LLDqNode sentinel;
    pthread_spinlock_t lock;
} LLDeque;

int lldq_init(LLDeque *deque);
int lldq_empty(LLDeque *deque);
static inline void lldq_node_init(LLDqNode *node) { node->next = NULL; }
LLDqNode *lldq_pop_head(LLDeque *deque);
LLDqNode *lldq_pop_tail(LLDeque *deque);
void lldq_push_head(LLDeque *deque, void *node);
void lldq_push_tail(LLDeque *deque, void *node);
void lldq_fini(LLDeque *deque, void (*fini)(void *, void *), void *arg);

/* clang-format off */

#define lldq_entry(ptr, type, member) CONTAINER_OF(ptr, type, member)

#define lldq_first(list, type, member) ({ \
    LLDeque * head__ = (list); \
    LLDqNode * pos__ = head__->head->next; \
    pos__ ? lldq_entry(pos__, type, member) : NULL; \
})

#define lldq_next(pos, member, list) ({ \
    LLDqNode * nxt__ = (pos)->member.next; \
    nxt__ ? lldq_entry(nxt__, typeof(*(pos)), member) : NULL; \
})

#define lldq_foreach(v__, member, list) \
    for (v__ = lldq_first(list, typeof(*(v__)), member); \
         v__; v__ = lldq_next(v__, member, list))

/* clang-format on */

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_LOCKED_LINKED_DEQUE_H_ */
