/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "collections/linkedlistobj.h"
#include "buffer.h"
#include "excobj.h"
#include "rangeobj.h"
#include "tupleobj.h"

#ifdef __cplusplus
extern "C" {
#endif

/* One element node, private to this translation unit: nodes are an
 * implementation detail, `LinkedListObject` owns them through the sentinel
 * only. The embedded `List` link makes splicing O(1). */
typedef struct _LinkedListNode {
    List link;
    TValue value;
} LinkedListNode;

/*---------------------------------------------------------------------------+
 |  Node helpers                                                             |
 |                                                                           |
 |  All of them return NULL once the sentinel is reached, so callers never   |
 |  have to compare against `&ll->head` themselves.                          |
 +---------------------------------------------------------------------------*/

static inline LinkedListNode *ll_first(LinkedListObject *ll)
{
    List *link = ll->head.next;
    return (link == &ll->head) ? NULL : list_entry(link, LinkedListNode, link);
}

static inline LinkedListNode *ll_last(LinkedListObject *ll)
{
    List *link = ll->head.prev;
    return (link == &ll->head) ? NULL : list_entry(link, LinkedListNode, link);
}

static inline LinkedListNode *ll_next(LinkedListObject *ll, LinkedListNode *node)
{
    List *link = node->link.next;
    return (link == &ll->head) ? NULL : list_entry(link, LinkedListNode, link);
}

static inline LinkedListNode *ll_prev(LinkedListObject *ll, LinkedListNode *node)
{
    List *link = node->link.prev;
    return (link == &ll->head) ? NULL : list_entry(link, LinkedListNode, link);
}

static LinkedListNode *ll_new_node(TValue value)
{
    LinkedListNode *node = mm_alloc(sizeof(LinkedListNode));
    node->value = value;
    init_list(&node->link);
    return node;
}

/* Append one element at the tail. O(1). */
static void ll_append(LinkedListObject *ll, TValue value)
{
    list_push_back(&ll->head, &ll_new_node(value)->link);
    ++ll->len;
    ++ll->mod_count;
}

/* Prepend one element at the front. O(1). */
static void ll_prepend(LinkedListObject *ll, TValue value)
{
    list_push_front(&ll->head, &ll_new_node(value)->link);
    ++ll->len;
    ++ll->mod_count;
}

/* Unlink `node` and release it. O(1). */
static void ll_unlink(LinkedListObject *ll, LinkedListNode *node)
{
    list_remove(&node->link);
    --ll->len;
    ++ll->mod_count;
    mm_free(node);
}

/* Node at `index`, which must satisfy 0 <= index < len.
 * Walks from the nearer end, so the worst case is n/2 steps. */
static LinkedListNode *ll_node_at(LinkedListObject *ll, size_t index)
{
    if (index < ll->len / 2) {
        LinkedListNode *node = ll_first(ll);
        for (size_t i = 0; i < index; ++i) node = ll_next(ll, node);
        return node;
    }

    LinkedListNode *node = ll_last(ll);
    for (size_t i = ll->len - 1; i > index; --i) node = ll_prev(ll, node);
    return node;
}

/* General-purpose TValue equality, mirroring the VM's OP_EQ layering:
 * value types compare by payload; everything else dispatches __eq__.
 * Same helper as `list_item_equal` in listobj.c. */
static TValue ll_item_equal(TValue *a, TValue *b)
{
    if (is_int(a) && is_int(b)) return bool_value(a->ival == b->ival);
    if (is_uint(a) && is_uint(b)) return bool_value((uint64_t)a->ival == (uint64_t)b->ival);
    if (is_float(a) && is_float(b)) return bool_value(a->fval == b->fval);
    if (is_bool(a) && is_bool(b)) return bool_value(a->ival == b->ival);
    if (is_nil(a) && is_nil(b)) return bool_value(true);
    if (is_nil(a) || is_nil(b)) return bool_value(false);
    return kl_call_cmp(a, b, SLOT_EQ);
}

/*---------------------------------------------------------------------------+
 |  Construction and basic properties                                        |
 +---------------------------------------------------------------------------*/

/* pub func __init__() */
static TValue _ll_init(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    LinkedListObject *ll = SELF_AS(linkedlist_type);
    init_list(&ll->head);
    ll->len = 0;
    ll->mod_count = 0;
    return no_value;
}

/* pub func len() int */
static TValue _ll_len(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    LinkedListObject *ll = SELF_AS(linkedlist_type);
    return int64_value((int64_t)ll->len);
}

/* pub func empty() bool */
static TValue _ll_empty(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    LinkedListObject *ll = SELF_AS(linkedlist_type);
    return bool_value(ll->len == 0);
}

Object *kl_new_linkedlist(void)
{
    LinkedListObject *ll = mm_alloc_obj(ll);
    INIT_OBJECT_HEAD(ll, &linkedlist_type, 0);
    init_list(&ll->head);
    ll->len = 0;
    ll->mod_count = 0;
    return (Object *)ll;
}

void kl_linkedlist_append(Object *ob, TValue item)
{
    ASSERT(IS_LINKEDLIST(ob));
    ll_append((LinkedListObject *)ob, item);
}

int kl_linkedlist_len(Object *ob)
{
    ASSERT(IS_LINKEDLIST(ob));
    return (int)((LinkedListObject *)ob)->len;
}
/*---------------------------------------------------------------------------+
 |  Lookup: __contains__ / index / count                                     |
 +---------------------------------------------------------------------------*/

/* pub func __contains__(v T) bool */
static TValue _ll_contains(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    LinkedListObject *ll = SELF_AS(linkedlist_type);

    for (LinkedListNode *node = ll_first(ll); node; node = ll_next(ll, node)) {
        TValue eq = ll_item_equal(&args[0], &node->value);
        if (is_error(&eq)) return error_value;
        if (to_bool(&eq)) return bool_value(true);
    }

    return bool_value(false);
}

/* pub func index(value T, start = 0, end = -1) int */
static TValue _ll_index(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 3);
    LinkedListObject *ll = SELF_AS(linkedlist_type);

    int64_t start = kl_arg_int64(1);
    int64_t end = kl_arg_int64(2);

    int64_t n = (int64_t)ll->len;
    if (end < 0 || end > n) end = n;
    if (start < 0) start = 0;
    if (start > n) start = n;
    if (start >= end) return int64_value(-1);

    LinkedListNode *node = ll_node_at(ll, (size_t)start);
    for (int64_t i = start; i < end && node; ++i, node = ll_next(ll, node)) {
        TValue eq = ll_item_equal(&args[0], &node->value);
        if (is_error(&eq)) return error_value;
        if (to_bool(&eq)) return int64_value(i);
    }

    return int64_value(-1);
}

/* pub func count(value T, start = 0, end = -1) int */
static TValue _ll_count(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 3);
    LinkedListObject *ll = SELF_AS(linkedlist_type);

    int64_t start = kl_arg_int64(1);
    int64_t end = kl_arg_int64(2);

    int64_t n = (int64_t)ll->len;
    if (end < 0 || end > n) end = n;
    if (start < 0) start = 0;
    if (start > n) start = n;
    if (start >= end) return int64_value(0);

    int64_t count = 0;
    LinkedListNode *node = ll_node_at(ll, (size_t)start);
    for (int64_t i = start; i < end && node; ++i, node = ll_next(ll, node)) {
        TValue eq = ll_item_equal(&args[0], &node->value);
        if (is_error(&eq)) return error_value;
        if (to_bool(&eq)) ++count;
    }

    return int64_value(count);
}

/*---------------------------------------------------------------------------+
 |  Indexed access                                                           |
 +---------------------------------------------------------------------------*/

/* pub func __getitem__(index int) T */
static TValue _ll_getitem(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    LinkedListObject *ll = SELF_AS(linkedlist_type);

    int64_t index = kl_arg_int64(0);
    int64_t n = (int64_t)ll->len;
    if (index < 0 || index >= n) {
        raise_exc_str("LinkedList index out of range");
        return error_value;
    }

    return ll_node_at(ll, (size_t)index)->value;
}

/* pub func __setitem__(index int, value T) */
static TValue _ll_setitem(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    LinkedListObject *ll = SELF_AS(linkedlist_type);

    int64_t index = kl_arg_int64(0);
    int64_t n = (int64_t)ll->len;
    if (index < 0 || index >= n) {
        raise_exc_str("LinkedList index out of range");
        return error_value;
    }

    /* No structural change: existing iterators stay valid. */
    ll_node_at(ll, (size_t)index)->value = args[1];
    return no_value;
}

/*---------------------------------------------------------------------------+
 |  Push / pop / peek at both ends                                           |
 +---------------------------------------------------------------------------*/

/* pub func append(value T) */
static TValue _ll_append(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    LinkedListObject *ll = SELF_AS(linkedlist_type);
    ll_append(ll, args[0]);
    return no_value;
}

/* pub func push_front(v T) */
static TValue _ll_push_front(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    LinkedListObject *ll = SELF_AS(linkedlist_type);
    ll_prepend(ll, args[0]);
    return no_value;
}

/* pub func push_back(v T) */
static TValue _ll_push_back(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    LinkedListObject *ll = SELF_AS(linkedlist_type);
    ll_append(ll, args[0]);
    return no_value;
}

/* pub func pop_front() T */
static TValue _ll_pop_front(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    LinkedListObject *ll = SELF_AS(linkedlist_type);

    LinkedListNode *node = ll_first(ll);
    if (node == NULL) {
        raise_exc_str("pop from empty LinkedList");
        return error_value;
    }

    TValue value = node->value;
    ll_unlink(ll, node);
    return value;
}

/* pub func pop_back() T */
static TValue _ll_pop_back(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    LinkedListObject *ll = SELF_AS(linkedlist_type);

    LinkedListNode *node = ll_last(ll);
    if (node == NULL) {
        raise_exc_str("pop from empty LinkedList");
        return error_value;
    }

    TValue value = node->value;
    ll_unlink(ll, node);
    return value;
}

/* pub func peek_front() T */
static TValue _ll_peek_front(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    LinkedListObject *ll = SELF_AS(linkedlist_type);

    LinkedListNode *node = ll_first(ll);
    if (node == NULL) {
        raise_exc_str("peek from empty LinkedList");
        return error_value;
    }

    return node->value;
}

/* pub func peek_back() T */
static TValue _ll_peek_back(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    LinkedListObject *ll = SELF_AS(linkedlist_type);

    LinkedListNode *node = ll_last(ll);
    if (node == NULL) {
        raise_exc_str("peek from empty LinkedList");
        return error_value;
    }

    return node->value;
}
/*---------------------------------------------------------------------------+
 |  Structural modification                                                  |
 +---------------------------------------------------------------------------*/

/* pub func insert(index int, value T) */
static TValue _ll_insert(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    LinkedListObject *ll = SELF_AS(linkedlist_type);

    int64_t index = kl_arg_int64(0);
    int64_t n = (int64_t)ll->len;
    if (index < 0 || index > n) {
        raise_exc_str("LinkedList insert index out of range");
        return error_value;
    }

    LinkedListNode *node = ll_new_node(args[1]);
    if (index == n) {
        list_push_back(&ll->head, &node->link);
    } else {
        list_add_before(&ll_node_at(ll, (size_t)index)->link, &node->link);
    }

    ++ll->len;
    ++ll->mod_count;
    return no_value;
}

/* pub func remove(value T) */
static TValue _ll_remove(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    LinkedListObject *ll = SELF_AS(linkedlist_type);

    for (LinkedListNode *node = ll_first(ll); node; node = ll_next(ll, node)) {
        TValue eq = ll_item_equal(&args[0], &node->value);
        if (is_error(&eq)) return error_value;
        if (to_bool(&eq)) {
            ll_unlink(ll, node);
            return no_value;
        }
    }

    raise_exc_str("LinkedList.remove(x): x not in LinkedList");
    return error_value;
}

/* pub func pop(index = -1) T */
static TValue _ll_pop(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    LinkedListObject *ll = SELF_AS(linkedlist_type);

    int64_t n = (int64_t)ll->len;
    if (n == 0) {
        raise_exc_str("pop from empty LinkedList");
        return error_value;
    }

    int64_t index = kl_arg_int64(0);
    if (index < 0) index += n; /* normalize negative index */
    if (index < 0 || index >= n) {
        raise_exc_str("pop index out of range");
        return error_value;
    }

    /* No special case for the two ends: `ll_node_at` walks from the nearer end,
     * so it reaches the head or the tail in zero steps. */
    LinkedListNode *node = ll_node_at(ll, (size_t)index);

    TValue value = node->value;
    ll_unlink(ll, node);
    return value;
}

/* pub func clear() */
static TValue _ll_clear(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    LinkedListObject *ll = SELF_AS(linkedlist_type);

    LinkedListNode *node = ll_first(ll);
    while (node) {
        LinkedListNode *next = ll_next(ll, node);
        mm_free(node);
        node = next;
    }

    init_list(&ll->head);
    ll->len = 0;
    ++ll->mod_count;
    return no_value;
}

/* pub func extend(items Iterable[T]) */
static TValue _ll_extend(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    LinkedListObject *ll = SELF_AS(linkedlist_type);
    Object *ob = kl_arg_obj(0);

    if (IS_LINKEDLIST(ob)) {
        LinkedListObject *other = (LinkedListObject *)ob;
        /* Snapshot the length first: `ll.extend(ll)` must terminate. */
        size_t n = other->len;
        LinkedListNode *node = ll_first(other);
        for (size_t i = 0; i < n && node; ++i) {
            ll_append(ll, node->value);
            node = ll_next(other, node);
        }
        return no_value;
    }

    if (IS_TUPLE(ob)) {
        TupleObject *other = (TupleObject *)ob;
        for (size_t i = 0; i < other->size; ++i) {
            ll_append(ll, other->array[i]);
        }
        return no_value;
    }

    if (IS_RANGE(ob)) {
        RangeObject *other = (RangeObject *)ob;
        int64_t len;
        if (!kl_range_len(other, &len)) return error_value;
        for (int64_t i = 0; i < len; ++i) {
            int64_t value;
            if (!kl_range_index(other, i, &value)) return error_value;
            ll_append(ll, int64_value(value));
        }
        return no_value;
    }

    if (kl_is_seq(ob)) {
        TValue len = kl_call_len(args);
        if (is_error(&len)) return error_value;
        int64_t m = (int64_t)to_int64(&len);
        for (int64_t i = 0; i < m; ++i) {
            TValue idx = int64_value(i);
            TValue item = kl_call_get_item(args, &idx);
            if (is_error(&item)) return error_value;
            ll_append(ll, item);
        }
        return no_value;
    }

    if (kl_is_iter(ob)) {
        TValue it = kl_call_iter(args, 1);
        if (is_error(&it)) return error_value;

        TValue item = kl_call_next(&it);
        while (!is_error(&item) && !is_nil(&item)) {
            ll_append(ll, item);
            item = kl_call_next(&it);
        }
        return is_error(&item) ? error_value : no_value;
    }

    raise_exc_str("LinkedList.extend: unsupported argument");
    return error_value;
}

/*---------------------------------------------------------------------------+
 |  Format                                                                   |
 +---------------------------------------------------------------------------*/

/* Lists longer than LL_FMT_MAX only show LL_FMT_EDGE items at each end. */
#define LL_FMT_MAX  10
#define LL_FMT_EDGE 4

/* pub func fmt(f Formatter) */
static TValue _ll_fmt(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    LinkedListObject *ll = SELF_AS(linkedlist_type);
    Formatter *fmt = kl_arg_fmt(0);

    buf_write_str(&fmt->buf, "LinkedList([");

    if (ll->len <= LL_FMT_MAX) {
        size_t i = 0;
        for (LinkedListNode *node = ll_first(ll); node; node = ll_next(ll, node), ++i) {
            if (i > 0) buf_write_str(&fmt->buf, ", ");
            kl_call_fmt(&node->value, fmt);
        }
    } else {
        /* Show the first LL_FMT_EDGE and the last LL_FMT_EDGE items. */
        size_t i = 0;
        for (LinkedListNode *node = ll_first(ll); node && i < LL_FMT_EDGE;
             node = ll_next(ll, node), ++i) {
            if (i > 0) buf_write_str(&fmt->buf, ", ");
            kl_call_fmt(&node->value, fmt);
        }

        buf_write_str(&fmt->buf, ", ..., ");

        LinkedListNode *tail = ll_last(ll);
        for (size_t k = 1; k < LL_FMT_EDGE && tail; ++k) {
            tail = ll_prev(ll, tail);
        }

        size_t j = 0;
        for (LinkedListNode *node = tail; node; node = ll_next(ll, node), ++j) {
            if (j > 0) buf_write_str(&fmt->buf, ", ");
            kl_call_fmt(&node->value, fmt);
        }
    }

    buf_write_str(&fmt->buf, "])");
    return no_value;
}
/*---------------------------------------------------------------------------+
 |  Iterators                                                                |
 |                                                                           |
 |  A linked list cannot be iterated by index (that would be O(n^2)), so the |
 |  cursor lives in C: `cur` points at the node to yield next.               |
 |  `LinkedListIter` and `LinkedListRevIter` share this object layout and    |
 |  the same C functions; only `reverse` differs.                            |
 +---------------------------------------------------------------------------*/

typedef struct _LinkedListIterObject {
    OBJECT_HEAD
    /* Source list, kept reachable for the whole iteration. */
    LinkedListObject *owner;
    /* Next node to yield; NULL means exhausted. */
    LinkedListNode *cur;
    int64_t step;
    /* Snapshot of owner->mod_count, checked before every dereference. */
    uint32_t mod_count;
    /* 0: forward, 1: backward. */
    int reverse;
} LinkedListIterObject;

static TValue ll_iter_init(LinkedListIterObject *iter, TValue *args, int nargs, int reverse)
{
    ASSERT(nargs == 2);

    LinkedListObject *owner = kl_arg_obj_as(0, linkedlist_type);
    int64_t step = kl_arg_int64(1);
    if (step <= 0) {
        raise_exc_str("step must be positive");
        return error_value;
    }

    iter->owner = owner;
    iter->step = step;
    iter->reverse = reverse;
    iter->mod_count = owner->mod_count;

    if (owner->len == 0) {
        iter->cur = NULL;
    } else {
        iter->cur = reverse ? ll_last(owner) : ll_first(owner);
    }

    return no_value;
}

/* LinkedListIter[T].__init__(src LinkedList[T], step int) */
static TValue _ll_iter_init(TValue *self, TValue *args, int nargs)
{
    Object *ob = to_obj(self);
    ASSERT(OB_TYPE(ob) == &linkedlist_iter_type);
    return ll_iter_init((LinkedListIterObject *)ob, args, nargs, 0);
}

/* LinkedListRevIter[T].__init__(src LinkedList[T], step int) */
static TValue _ll_rev_iter_init(TValue *self, TValue *args, int nargs)
{
    Object *ob = to_obj(self);
    ASSERT(OB_TYPE(ob) == &linkedlist_rev_iter_type);
    return ll_iter_init((LinkedListIterObject *)ob, args, nargs, 1);
}

/* has_next() bool */
static TValue _ll_iter_has_next(TValue *self, TValue *args, int nargs)
{
    Object *ob = to_obj(self);
    TypeObject *tp = OB_TYPE(ob);
    ASSERT(tp == &linkedlist_iter_type || tp == &linkedlist_rev_iter_type);

    return bool_value(((LinkedListIterObject *)ob)->cur != NULL);
}

static TValue ll_iter_next(LinkedListIterObject *iter)
{
    LinkedListNode *node = iter->cur;
    if (node == NULL) return nil_value;

    /* Checked before dereferencing `cur`: a mutation may have freed it. */
    if (iter->mod_count != iter->owner->mod_count) {
        raise_exc_str("LinkedList modified during iteration");
        return error_value;
    }

    TValue value = node->value;

    /* Advance `step` nodes; running off the end means exhausted. */
    for (int64_t i = 0; i < iter->step && node; ++i) {
        node = iter->reverse ? ll_prev(iter->owner, node) : ll_next(iter->owner, node);
    }
    iter->cur = node;

    return value;
}

/* next() T? */
static TValue _ll_iter_next(TValue *self, TValue *args, int nargs)
{
    Object *ob = to_obj(self);
    TypeObject *tp = OB_TYPE(ob);
    ASSERT(tp == &linkedlist_iter_type || tp == &linkedlist_rev_iter_type);

    return ll_iter_next((LinkedListIterObject *)ob);
}

/* next_strict() T */
static TValue _ll_iter_next_strict(TValue *self, TValue *args, int nargs)
{
    TValue ret = _ll_iter_next(self, args, nargs);
    if (is_error(&ret)) return error_value;
    if (is_nil(&ret)) {
        raise_exc_str("No more elements in LinkedListIter");
        return error_value;
    }
    return ret;
}

/* next_strict() T */
static TValue _ll_rev_iter_next_strict(TValue *self, TValue *args, int nargs)
{
    TValue ret = _ll_iter_next(self, args, nargs);
    if (is_error(&ret)) return error_value;
    if (is_nil(&ret)) {
        raise_exc_str("No more elements in LinkedListRevIter");
        return error_value;
    }
    return ret;
}
/*---------------------------------------------------------------------------+
 |  Type objects                                                             |
 +---------------------------------------------------------------------------*/

static MethodDef linkedlist_methods[] = {
    { "__init__", _ll_init },
    { "len", _ll_len },
    { "empty", _ll_empty },
    { "__contains__", _ll_contains },
    { "index", _ll_index },
    { "count", _ll_count },
    { "__getitem__", _ll_getitem },
    { "__setitem__", _ll_setitem },
    { "append", _ll_append },
    { "extend", _ll_extend },
    { "insert", _ll_insert },
    { "remove", _ll_remove },
    { "pop", _ll_pop },
    { "clear", _ll_clear },
    { "push_front", _ll_push_front },
    { "push_back", _ll_push_back },
    { "pop_front", _ll_pop_front },
    { "pop_back", _ll_pop_back },
    { "peek_front", _ll_peek_front },
    { "peek_back", _ll_peek_back },
    { "fmt", _ll_fmt },
    { NULL },
};

/* pub class LinkedList[T] : MutableSequence[T] { ... } */
TypeObject linkedlist_type = {
    ._type = &type_type,
    .name = "LinkedList",
    .flags = TP_FLAGS_CLASS,
    .objsize = sizeof(LinkedListObject),
    .methdefs = linkedlist_methods,
};

/* class LinkedListIter[T] : Iterator[T] { ... } */
TypeObject linkedlist_iter_type = {
    ._type = &type_type,
    .name = "LinkedListIter",
    .flags = TP_FLAGS_CLASS,
    .objsize = sizeof(LinkedListIterObject),
    .methdefs =
        (MethodDef[]){
            { "__init__", _ll_iter_init },
            { "has_next", _ll_iter_has_next },
            { "next", _ll_iter_next },
            { "next_strict", _ll_iter_next_strict },
            { NULL, NULL },
        },
};

/* class LinkedListRevIter[T] : Iterator[T] { ... } */
TypeObject linkedlist_rev_iter_type = {
    ._type = &type_type,
    .name = "LinkedListRevIter",
    .flags = TP_FLAGS_CLASS,
    .objsize = sizeof(LinkedListIterObject),
    .methdefs =
        (MethodDef[]){
            { "__init__", _ll_rev_iter_init },
            { "has_next", _ll_iter_has_next },
            { "next", _ll_iter_next },
            { "next_strict", _ll_rev_iter_next_strict },
            { NULL, NULL },
        },
};

#ifdef __cplusplus
}
#endif
