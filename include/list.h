/*----------------------------------------------------------------------------*\
|* This file is part of the koala project, under the MIT License.             *|
|* Copyright (c) 2021-2021 James <zhuguangxiang@gmail.com>                    *|
\*----------------------------------------------------------------------------*/

#ifndef _KOALA_LIST_H_
#define _KOALA_LIST_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* List node */
typedef struct list_node {
    /* Previous list node */
    struct list_node *prev;
    /* Next list node */
    struct list_node *next;
} list_node_t;

/* List structure */
typedef list_node_t list_t;

/*
 * List initialization
 *
 * A list may be initialized by calling list_init():
 *
 * list_t list;
 * list_init(&list);
 *
 * or with an initializer using LIST_INITIALIZER:
 *
 * list_t list = LIST_INITIALIZER(list);
 *
 */

// clang-format off
#define LIST_NODE_INITIALIZER(name) { &(name), &(name) }
#define LIST_INITIALIZER(name) LIST_NODE_INITIALIZER(name)
// clang-format on

static inline void list_init(list_t *list)
{
    list->prev = list;
    list->next = list;
}

static inline void list_node_init(list_node_t *node)
{
    node->prev = node;
    node->next = node;
}

/* Get the entry in which node is embedded */
#define list_entry(ptr, type, member) container_of(ptr, type, member)

/* Test a list is empty or not */
#define list_is_empty(list) ((list)->next == (list))

/* Get the first element from a list */
#define list_first(list) (list_is_empty(list) ? NULL : (list)->next)

/* Get the last element from a list */
#define list_last(list) (list_is_empty(list) ? NULL : (list)->prev)

/* Get the next element from a list */
#define list_next(list, pos)             \
    ({                                   \
        list_node_t *next = (pos)->next; \
        (next == (list)) ? NULL : next;  \
    })

/* Get the previous element from a list */
#define list_prev(list, pos)             \
    ({                                   \
        list_node_t *prev = (pos)->prev; \
        (prev == (list)) ? NULL : prev;  \
    })

/* Iterate a list */
#define list_foreach(list, pos) \
    for (pos = (list)->next; pos != (list); pos = pos->next)

/* Iterate a list in reverse order */
#define list_foreach_reverse(list, pos) \
    for (pos = (list)->prev; pos != (list); pos = pos->prev)

/* Get the length of a list */
static inline int list_length(list_t *list)
{
    int count = 0;
    list_node_t *pos;
    list_foreach(list, pos) count++;
    return count;
}

/* Insert a node between two known consecutive nodes */
static inline void list_add_between(
    list_node_t *entry, list_node_t *prev, list_node_t *next)
{
    entry->next = next;
    entry->prev = prev;
    prev->next = entry;
    next->prev = entry;
}

/* Insert a 'pos' node after 'prev' node */
static inline void list_add(list_node_t *prev, list_node_t *pos)
{
    list_add_between(pos, prev, prev->next);
}

/* Insert a 'pos' node before 'next' node */
static inline void list_add_before(list_node_t *next, list_node_t *pos)
{
    list_add_between(pos, next->prev, next);
}

/* Insert a 'pos' node at front */
static inline void list_push_front(list_t *list, list_node_t *pos)
{
    list_add_between(pos, list, list->next);
}

/* Insert a 'pos' node at tail */
static inline void list_push_back(list_t *list, list_node_t *pos)
{
    list_add_between(pos, list->prev, list);
}

/* Remove a node by making the prev/next nodes pointer to each other. */
#define list_remove_between(_prev, _next) \
    ({                                    \
        _next->prev = _prev;              \
        _prev->next = _next;              \
    })

/* Remove a node from list */
#define list_remove(pos)                               \
    ({                                                 \
        list_remove_between((pos)->prev, (pos)->next); \
        /* re-initialize node */                       \
        list_node_init(pos);                           \
    })

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_LIST_H_ */
