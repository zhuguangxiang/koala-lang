/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_QUEUE_H_
#define _KOALA_QUEUE_H_

#include "list.h"
#include "mm.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Queue is first-in first-out(FIFO) ADT.
 */
typedef struct _Queue {
    List head;
    int count;
} Queue;

typedef struct _QNode {
    List entry;
    void *data;
} QNode;

#define QUEUE(name) Queue name = { LIST_INIT((name).head), 0 }

static inline void init_queue(Queue *que)
{
    init_list(&que->head);
    que->count = 0;
}

static inline int queue_size(Queue *que) { return que->count; }
static inline int queue_empty(Queue *que) { return list_empty(&que->head); }

static inline void queue_push(Queue *que, void *data)
{
    QNode *node = mm_alloc_obj(node);
    init_list(&node->entry);
    node->data = data;
    list_push_back(&que->head, &node->entry);
    ++que->count;
}

static inline void *queue_pop(Queue *que)
{
    QNode *node = list_first(&que->head, QNode, entry);
    if (node == NULL) return NULL;
    list_remove(&node->entry);
    --que->count;
    void *data = node->data;
    mm_free(node);
    return data;
}

static inline void *queue_front(Queue *que)
{
    QNode *node = list_first(&que->head, QNode, entry);
    if (node == NULL) return NULL;
    return node->data;
}

static inline void *queue_back(Queue *que)
{
    QNode *node = list_last(&que->head, QNode, entry);
    if (node == NULL) return NULL;
    return node->data;
}

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_QUEUE_H_ */
