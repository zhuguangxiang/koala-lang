/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2023 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "lldq.h"
#include <pthread.h>
#include "common.h"
#include "mm.h"

int lldq_init(LLDeque *deque)
{
    lldq_node_init(&deque->sentinel);
    deque->tail = &deque->sentinel;
    deque->head = deque->tail;
    deque->count = 0;
    pthread_spin_init(&deque->lock, 0);
    return 0;
}

int lldq_empty(LLDeque *deque)
{
    int ret = 0;
    pthread_spin_lock(&deque->lock);
    ret = (deque->count == 0);
    pthread_spin_unlock(&deque->lock);
    return ret;
}

LLDqNode *lldq_pop_head(LLDeque *deque)
{
    pthread_spin_lock(&deque->lock);
    LLDqNode *head = deque->head;
    LLDqNode *first = head->next;
    if (first) {
        head->next = first->next;
        first->next = NULL;
        deque->count--;
    }
    if (first == deque->tail) {
        deque->tail = deque->head;
    }
    pthread_spin_unlock(&deque->lock);
    return first;
}

LLDqNode *lldq_pop_tail(LLDeque *deque) { NYI(); }

void lldq_push_head(LLDeque *deque, void *node) { NYI(); }

void lldq_push_tail(LLDeque *deque, void *node)
{
    pthread_spin_lock(&deque->lock);
    LLDqNode *tail = deque->tail;
    tail->next = node;
    deque->tail = node;
    deque->count++;
    pthread_spin_unlock(&deque->lock);
}

void lldq_fini(LLDeque *deque, void (*fini)(void *, void *), void *arg) {}
