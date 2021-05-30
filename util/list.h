/*
 * This file is part of the koala-lang project, under the MIT License.
 *
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
 */

#ifndef _KOALA_LIST_H_
#define _KOALA_LIST_H_

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _List {
    /* Previous list entry */
    struct _List *prev;
    /* Next list entry */
    struct _List *next;
} List, *ListRef;

// clang-format off
#define LIST_INIT(name) { &(name), &(name) }
// clang-format on

static inline void init_list(ListRef list)
{
    list->prev = list;
    list->next = list;
}

/* Test whether a list is empty */
static inline int list_empty(ListRef list)
{
    return list->next == list;
}

/* Insert a new entry between two known consecutive entries */
static inline void __list_add(ListRef __new, ListRef prev, ListRef next)
{
    next->prev = __new;
    __new->next = next;
    __new->prev = prev;
    prev->next = __new;
}

/* Insert a new entry after 'prev' entry */
static inline void list_add(ListRef prev, ListRef __new)
{
    __list_add(__new, prev, prev->next);
}

/* Insert a new entry before 'next' entry */
static inline void list_add_before(ListRef next, ListRef __new)
{
    __list_add(__new, next->prev, next);
}

/* Insert a new entry at front */
static inline void list_push_front(ListRef list, ListRef __new)
{
    __list_add(__new, list, list->next);
}

/* Insert a new entry at tail */
static inline void list_push_back(ListRef list, ListRef __new)
{
    __list_add(__new, list->prev, list);
}

/* Remove a entry by making the prev/next entries pointer to each other. */
static inline void __list_remove(ListRef prev, ListRef next)
{
    next->prev = prev;
    prev->next = next;
}

/* Remove an entry from list */
static inline void list_remove(ListRef entry)
{
    __list_remove(entry->prev, entry->next);
    init_list(entry);
}

/* Pop an entry at front */
static inline ListRef list_pop_front(ListRef list)
{
    ListRef entry = list->next;
    if (entry == list) return NULL;
    list_remove(entry);
    return entry;
}

/* Pop an entry at tail */
static inline ListRef list_pop_back(ListRef list)
{
    ListRef entry = list->prev;
    if (entry == list) return NULL;
    list_remove(entry);
    return entry;
}

// clang-format off

/* Get the entry in which is embedded */
#define list_entry(ptr, type, member) CONTAINER_OF(ptr, type, member)

/* Get the first element from a list */
#define list_first(list, type, member) ({ \
    ListRef head__ = (list); \
    ListRef pos__ = head__->next; \
    pos__ != head__ ? list_entry(pos__, type, member) : NULL; \
})

/* Get the last element from a list */
#define list_last(list, type, member) ({ \
    ListRef head__ = (list); \
    ListRef pos__ = head__->prev; \
    pos__ != head__ ? list_entry(pos__, type, member) : NULL; \
})

/* Get the next element from a list */
#define list_next(pos, member, list) ({ \
    ListRef head__ = (list); \
    ListRef nxt__ = (pos)->member.next; \
    nxt__ != head__ ? list_entry(nxt__, typeof(*(pos)), member) : NULL; \
})

/* Get the previous element from a list */
#define list_prev(pos, member, list) ({ \
    ListRef head__ = (list); \
    ListRef prev__ = (pos)->member.prev; \
    prev__ != head__ ? list_entry(prev__, typeof(*(pos)), member) : NULL; \
})

#define list_foreach(v__, member, list, closure) \
    for (v__ = list_first(list, typeof(*(v__)), member); \
         v__; v__ = list_next(v__, member, list)) closure;

#define list_foreach_reverse(v__, member, list, closure) \
    for (v__ = list_last(list, typeof(*(v__)), member); \
         v__; v__ = list_prev(v__, member, list)) closure;

#define list_foreach_safe(v__, n__, member, list, closure) \
    for (v__ = list_first(list, typeof(*(v__)), member), \
         n__ = v__ ? list_next(v__, member, list) : NULL; \
         v__ && ({ n__ = list_next(v__, member, list); 1;}); v__ = n__) closure;

// clang-format on

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_LIST_H_ */
