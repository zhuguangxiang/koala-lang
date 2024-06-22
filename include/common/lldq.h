/**
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
static inline void lldq_init_node(LLDqNode *node) { node->next = NULL; }
LLDqNode *lldq_pop_head(LLDeque *deque);
LLDqNode *lldq_pop_tail(LLDeque *deque);
void lldq_push_head(LLDeque *deque, void *node);
void lldq_push_tail(LLDeque *deque, void *node);
void lldq_fini(LLDeque *deque, void (*fini)(void *, void *), void *arg);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_LOCKED_LINKED_DEQUE_H_ */
