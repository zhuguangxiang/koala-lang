/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangXiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_LIST_H_
#define _KOALA_LIST_H_

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* List element node */
typedef struct ListNode {
    /* Previous list element node */
    struct ListNode *prev;
    /* Next list element node */
    struct ListNode *next;
} ListNode;

/* List structure */
typedef ListNode List;

/*
 * List initialization
 *
 * A list may be initialized by calling list_init():
 *
 * List list;
 * list_init(&list);
 *
 * or with an initializer using LIST_INITIALIZER:
 *
 * List list = LIST_INITIALIZER(list);
 *
 */
#define LIST_INITIALIZER(name) \
    {                          \
        &(name), &(name)       \
    }

static inline void list_init(List *list)
{
    list->prev = list;
    list->next = list;
}

#define LIST_NODE_INITIALIZER(name) \
    {                               \
        &(name), &(name)            \
    }

static inline void list_node_init(ListNode *node)
{
    node->prev = node;
    node->next = node;
}

/* Test a list is empty */
#define list_empty(list) ((list)->next == (list))

/* Get the first element from a list */
#define list_first(list) (list_empty(list) ? NULL : (list)->next)

/* Get the last element from a list */
#define list_last(list) (list_empty(list) ? NULL : (list)->prev)

/* Get the next element from a list */
#define list_next(list, pos)            \
    ({                                  \
        ListNode *next = (pos)->next;   \
        (next == (list)) ? NULL : next; \
    })

/* Get the previous element from a list */
#define list_prev(list, pos)            \
    ({                                  \
        ListNode *prev = (pos)->prev;   \
        (prev == (list)) ? NULL : prev; \
    })

/* Iterate a list */
#define list_foreach(list, pos) \
    for (pos = (list)->next; pos != (list); pos = pos->next)

/* Iterate a list in reverse order */
#define list_foreach_reverse(list, pos) \
    for (pos = (list)->prev; pos != (list); pos = pos->prev)

/* Get the length of a list */
static inline int list_size(List *list)
{
    int count = 0;
    ListNode *pos;
    list_foreach(list, pos) count++;
    return count;
}

/* Insert a node between two known consecutive nodes */
static inline void list_add_between(
    ListNode *entry, ListNode *prev, ListNode *next)
{
    entry->next = next;
    entry->prev = prev;
    prev->next = entry;
    next->prev = entry;
}

/* Insert a 'pos' node after 'prev' node */
static inline void list_add(ListNode *prev, ListNode *pos)
{
    list_add_between(pos, prev, prev->next);
}

/* Insert a 'pos' node before 'next' node */
static inline void list_add_before(ListNode *next, ListNode *pos)
{
    list_add_between(pos, next->prev, next);
}

/* Insert a 'pos' node at front */
static inline void list_push_front(List *list, ListNode *pos)
{
    list_add_between(pos, list, list->next);
}

/* Insert a 'pos' node at tail */
static inline void list_push_back(List *list, ListNode *pos)
{
    list_add_between(pos, list->prev, list);
}

/* Remove a node by making the prev/next nodes pointer to each other. */
#define list_remove_between(prev, next) \
    ({                                  \
        next->prev = prev;              \
        prev->next = next;              \
    })

/* Remove a node from list */
#define list_remove(pos)                            \
    ({                                              \
        list_del_between((pos)->prev, (pos)->next); \
        /* re-initialize node */                    \
        list_node_init(pos);                        \
    })

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_LIST_H_ */
