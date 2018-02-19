/*
 * list.h - double linked list, from linux kernel include/linux/list.h
 */
#ifndef _KOALA_LIST_H_
#define _KOALA_LIST_H_

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Simple doubly linked list implementation.
 * Some functions ("__xxx") are used internally.
 */
struct list_head {
	struct list_head *next, *prev;
};

#define LIST_HEAD_INIT(name) {&(name), &(name)}

#define LIST_HEAD(name) \
	struct list_head name = LIST_HEAD_INIT(name)

static inline void init_list_head(struct list_head *list)
{
	list->next = list;
	list->prev = list;
}

#define list_unlinked(node) \
	((node)->next == (node) && (node)->prev == (node))

/*
 * Insert a new entry between two known consecutive entries.
 */
static inline void __list_add(struct list_head *entry,
															struct list_head *prev,
															struct list_head *next)
{
	entry->next = next;
	entry->prev = prev;
	prev->next = entry;
	next->prev = entry;
}

/**
 * list_add - add a new entry
 * @entry: new entry to be added
 * @head: list head to add it after
 */
static inline void list_add(struct list_head *entry, struct list_head *head)
{
	__list_add(entry, head, head->next);
}


/**
 * list_add_tail - add a new entry
 * @entry: new entry to be added
 * @head: list head to add it before
 */
static inline void list_add_tail(struct list_head *entry,
																 struct list_head *head)
{
	__list_add(entry, head->prev, head);
}

/*
 * Delete a list entry by making the prev/next entries point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_del(struct list_head *prev, struct list_head *next)
{
	next->prev = prev;
	prev->next = next;
}

/**
 * list_del - deletes entry from list and reinitialize it.
 * @entry: the element to delete from the list.
 */
static inline void list_del(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
	init_list_head(entry);
}

/**
 * list_empty - tests whether a list is empty
 * @head: the list to test.
 */
#define list_empty(head)  ((head)->next == head)

/**
 * list_first - get the first element from a list
 * @head:	the head for your list.
 *
 * Note that if the list is empty, it returns NULL.
 */
#define list_first(head)  (list_empty(head) ? NULL : (head)->next)

/**
 * list_last - get the last element from a list
 * @head:	the head for your list.
 *
 * Note that if the list is empty, it returns NULL.
 */
#define list_last(head)   (list_empty(head) ? NULL : (head)->prev)

/**
 * list_entry - get the struct for this entry
 * @ptr:	the &struct list_head pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_head within the struct.
 */
#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)

/**
 * list_for_each	-	iterate over a list
 * @pos:	the &struct list_head to use as a loop cursor.
 * @head:	the head for your list.
 */
#define list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * list_for_each_prev	-	iterate over a list backwards
 * @pos:	the &struct list_head to use as a loop cursor.
 * @head:	the head for your list.
 */
#define list_for_each_prev(pos, head) \
	for (pos = (head)->prev; pos != (head); pos = pos->prev)

/**
 * list_for_each_safe - iterate over a list safe against removal of list entry
 * @pos:	the &struct list_head to use as a loop cursor.
 * @n:		another &struct list_head to use as temporary storage
 * @head:	the head for your list.
 */
#define list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
			 pos = n, n = pos->next)

/**
 * list_for_each_prev_safe - iterate over a list backwards safe
 * against removal of list entry
 * @pos:	the &struct list_head to use as a loop cursor.
 * @n:		another &struct list_head to use as temporary storage
 * @head:	the head for your list.
 */
#define list_for_each_prev_safe(pos, n, head) \
	for (pos = (head)->prev, n = pos->prev; \
			 pos != (head); \
			 pos = n, n = pos->prev)

/**
 * list_first_entry - get the first element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_head within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define list_first_entry(ptr, type, member) \
	list_entry((ptr)->next, type, member)

/**
 * list_next_entry - get the next element in list
 * @pos:	the type * to cursor
 * @member:	the name of the list_head within the struct.
 */
#define list_next_entry(pos, member) \
	list_entry((pos)->member.next, __typeof__(*(pos)), member)

/**
 * list_for_each_entry	-	iterate over list of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_head within the struct.
 */
#define list_for_each_entry(pos, head, member) \
	for (pos = list_first_entry(head, __typeof__(*pos), member); \
			 &pos->member != (head); \
			 pos = list_next_entry(pos, member))

/**
 * list_for_each_entry_safe - iterate over list of given type safe against removal of list entry
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the list_head within the struct.
 */
#define list_for_each_entry_safe(pos, n, head, member) \
	for (pos = list_first_entry(head, typeof(*pos), member), \
			 n = list_next_entry(pos, member); \
			 &pos->member != (head); \
			 pos = n, n = list_next_entry(n, member))

/**
* list_merge_tail - move 'from' list to the tail of 'to'
* @from: the merged list
* @to: the list
*/
static inline
struct list_head *list_merge_tail(struct list_head *from, struct list_head *to)
{
	struct list_head *pos, *nxt;
	list_for_each_safe(pos, nxt, from) {
		list_del(pos);
		list_add_tail(pos, to);
	}
	return to;
}

/*
 * Double linked lists with a single pointer of list head.
 * Mostly useful for hash tables where the two pointer list head is
 * too wasteful.
 * You lose the ability to access the tail in O(1).
 */
struct hlist_head {
	struct hlist_node *first;
};

struct hlist_node {
	struct hlist_node *next, **pprev;
};

#define HLIST_HEAD_INIT   {.first = NULL}
#define HLIST_HEAD(name)  struct hlist_head name = {.first = NULL}
#define init_hlist_head(ptr)  ((ptr)->first = NULL)
static inline void init_hlist_node(struct hlist_node *h)
{
	h->next = NULL;
	h->pprev = NULL;
}

static inline int hlist_unhashed(const struct hlist_node *node)
{
	return !node->pprev;
}

static inline int hlist_empty(const struct hlist_head *head)
{
	return !head->first;
}

static inline void hlist_del(struct hlist_node *n)
{
	if (!hlist_unhashed(n)) {
		struct hlist_node *next = n->next;
		struct hlist_node **pprev = n->pprev;

		*pprev = next;
		if (next) next->pprev = pprev;
		init_hlist_node(n);
	}
}

static inline void hlist_add_head(struct hlist_node *n, struct hlist_head *h)
{
	struct hlist_node *first = h->first;
	n->next = first;
	if (first) first->pprev = &n->next;
	h->first = n;
	n->pprev = &h->first;
}

#define hlist_entry(ptr, type, member) container_of(ptr, type, member)

#define hlist_for_each(pos, head) \
	for (pos = (head)->first; pos ; pos = pos->next)

#define hlist_for_each_safe(pos, n, head) \
	for (pos = (head)->first; pos && ({ n = pos->next; 1; }); \
			 pos = n)

/* Tail queue:
 * Double linked lists with a last node has not a pointer of list head.
 */
struct tailq_head {
	struct tailq_node *first;
	struct tailq_node **last;
};

struct tailq_node {
	struct tailq_node *next, **pprev;
};

#define TAILQ_HEAD_INIT(name)   {.first = NULL, .last = &(name).first}
#define TAILQ_HEAD(name)  struct tailq_head name = TAILQ_HEAD_INIT(name)
#define init_tailq_head(ptr) do { \
	(ptr)->first = NULL; \
	(ptr)->last  = &(ptr)->first; \
} while (0)

static inline void init_tailq_node(struct tailq_node *node)
{
	node->next = NULL;
	node->pprev = NULL;
}

static inline int tailq_unhashed(const struct tailq_node *node)
{
	return !node->pprev;
}

static inline int tailq_empty(const struct tailq_head *head)
{
	return !head->first;
}

static inline void tailq_del(struct tailq_node *n, struct tailq_head *h)
{
	if (!tailq_unhashed(n)) {
		struct tailq_node *next = n->next;
		struct tailq_node **pprev = n->pprev;

		*pprev = next;
		if (next)
			next->pprev = pprev;
		else
			h->last = pprev;

		init_tailq_node(n);
	}
}

static inline void tailq_add_head(struct tailq_node *n, struct tailq_head *h)
{
	struct tailq_node *first = h->first;
	n->next = first;
	if (first)
		first->pprev = &n->next;
	else
		h->last = &n->next;
	h->first = n;
	n->pprev = &h->first;
}

static inline void tailq_add_tail(struct tailq_node *n, struct tailq_head *h)
{
	n->pprev = h->last;
	*h->last = n;
	h->last = &n->next;
}

#define tailq_entry(ptr, type, member) container_of(ptr, type, member)

#define tailq_for_each(pos, head) \
	for (pos = (head)->first; pos ; pos = pos->next)

#define tailq_for_each_safe(pos, n, head) \
	for (pos = (head)->first; pos && (n = pos->next, 1); \
			 pos = n)

#define tailq_for_each_entry(pos, head, member) \
	for (pos = tailq_entry((head)->first, __typeof__(*(pos)), member); \
			 pos;	\
			 pos = (pos)->member.next ? \
			 tailq_entry((pos)->member.next, __typeof__(*(pos)), member) : NULL)

#define tailq_for_each_entry_safe(pos, n, head, member) \
	for (pos = tailq_entry((head)->first, __typeof__(*pos), member); \
			 pos && (n = pos->member.next, 1); \
			 pos = n ? tailq_entry(n, __typeof__(*pos), member) : NULL)

/**
 * linked list with counting
 */
struct clist {
	struct list_head head;
	int count;
};

#define init_clist(list) do { \
	init_list_head(&(list)->head); \
	(list)->count = 0; \
} while (0)

#define clist_add(link, list) do { \
	list_add((link), &(list)->head); \
	++(list)->count; \
} while (0)

#define clist_add_tail(link, list) do { \
	list_add_tail((link), &(list)->head); \
	++(list)->count; \
} while (0)

#define clist_del(pos, list) do { \
	list_del(pos); \
	--(list)->count; \
	ASSERT((list)->count >= 0); \
} while (0)

#define clist_empty(list) \
	(list_empty(&(list)->head) && ((list)->count == 0))

#define clist_length(list) ((list)->count)

#define clist_first(list) list_first(&(list)->head)

#define clist_entry(ptr, type) list_entry(ptr, type, link)

#define clist_foreach(pos, list) \
	if (list) list_for_each_entry(pos, &(list)->head, link)

#define clist_foreach_safe(pos, list, tmp) \
	if (list) list_for_each_entry_safe(pos, tmp, &(list)->head, link)

static inline struct clist *new_clist(void)
{
	struct clist *list = malloc(sizeof(struct clist));
	init_clist(list);
	return list;
}

static inline void free_clist(struct clist *list)
{
	ASSERT(clist_empty(list));
	free(list);
}

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_LIST_H_ */
