/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "collections/dequeobj.h"
#include "buffer.h"
#include "excobj.h"
#include "rangeobj.h"
#include "tupleobj.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Elements are stored by value (TValue); the ring's buffer holds them, and
 * `ring.size` is the number of live elements. */
#define DEQUE_ELEM_SIZE ((int)sizeof(TValue))

/* Deques longer than DEQUE_FMT_MAX only show DEQUE_FMT_EDGE items at each end. */
#define DEQUE_FMT_MAX  10
#define DEQUE_FMT_EDGE 4

/* Number of live elements. */
static inline int dq_size(DequeObject *dq) { return ring_size(&dq->ring); }

/* Pointer to the live element at `index`, 0 <= index < dq_size(). */
static inline TValue *dq_at(DequeObject *dq, int index)
{
    return (TValue *)ring_get_ptr(&dq->ring, index);
}

/* General-purpose TValue equality, mirroring the VM's OP_EQ layering:
 * value types compare by payload; everything else dispatches __eq__.
 * Same helper as `ll_item_equal` in linkedlistobj.c. */
static TValue dq_item_equal(TValue *a, TValue *b)
{
    if (is_int(a) && is_int(b)) return bool_value(a->ival == b->ival);
    if (is_uint(a) && is_uint(b)) return bool_value((uint64_t)a->ival == (uint64_t)b->ival);
    if (is_float(a) && is_float(b)) return bool_value(a->fval == b->fval);
    if (is_bool(a) && is_bool(b)) return bool_value(a->ival == b->ival);
    if (is_nil(a) && is_nil(b)) return bool_value(true);
    if (is_nil(a) || is_nil(b)) return bool_value(false);
    return kl_call_cmp(a, b, SLOT_EQ);
}

/* Append one element at the back. `mm_alloc` aborts when out of memory, so the
 * push cannot fail. */
static void dq_push_back(DequeObject *dq, TValue value)
{
    ring_push_back(&dq->ring, &value);
    ++dq->mod_count;
}

/* Prepend one element at the front. Same no-failure reasoning as above. */
static void dq_push_front(DequeObject *dq, TValue value)
{
    ring_push_front(&dq->ring, &value);
    ++dq->mod_count;
}

/*---------------------------------------------------------------------------+
 |  Construction and basic properties                                        |
 +---------------------------------------------------------------------------*/

// __init__()
static TValue _dq_init(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    DequeObject *dq = SELF_AS(deque_type);
    ring_init(&dq->ring, DEQUE_ELEM_SIZE);
    dq->mod_count = 0;
    return no_value;
}

// len() -> int
static TValue _dq_len(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    DequeObject *dq = SELF_AS(deque_type);
    return int64_value(dq_size(dq));
}

// empty() -> bool
static TValue _dq_empty(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    DequeObject *dq = SELF_AS(deque_type);
    return bool_value(ring_empty(&dq->ring));
}

/*---------------------------------------------------------------------------+
 |  Lookup: __contains__ / index / count                                      |
 +---------------------------------------------------------------------------*/

// __contains__(v T) -> bool
static TValue _dq_contains(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    DequeObject *dq = SELF_AS(deque_type);

    int size = dq_size(dq);
    for (int i = 0; i < size; ++i) {
        TValue eq = dq_item_equal(&args[0], dq_at(dq, i));
        if (is_error(&eq)) return error_value;
        if (to_bool(&eq)) return bool_value(true);
    }

    return bool_value(false);
}

// index(value T, start = 0, end = -1) -> int
static TValue _dq_index(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 3);
    DequeObject *dq = SELF_AS(deque_type);

    int64_t start = kl_arg_int64(1);
    int64_t end = kl_arg_int64(2);

    int64_t n = (int64_t)dq_size(dq);
    if (end < 0 || end > n) end = n;
    if (start < 0) start = 0;
    if (start > n) start = n;
    if (start >= end) return int64_value(-1);

    for (int64_t i = start; i < end; ++i) {
        TValue eq = dq_item_equal(&args[0], dq_at(dq, (int)i));
        if (is_error(&eq)) return error_value;
        if (to_bool(&eq)) return int64_value(i);
    }

    return int64_value(-1);
}

// count(value T, start = 0, end = -1) -> int
static TValue _dq_count(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 3);
    DequeObject *dq = SELF_AS(deque_type);

    int64_t start = kl_arg_int64(1);
    int64_t end = kl_arg_int64(2);

    int64_t n = (int64_t)dq_size(dq);
    if (end < 0 || end > n) end = n;
    if (start < 0) start = 0;
    if (start > n) start = n;
    if (start >= end) return int64_value(0);

    int64_t count = 0;
    for (int64_t i = start; i < end; ++i) {
        TValue eq = dq_item_equal(&args[0], dq_at(dq, (int)i));
        if (is_error(&eq)) return error_value;
        if (to_bool(&eq)) ++count;
    }

    return int64_value(count);
}

/*---------------------------------------------------------------------------+
 |  Indexed access                                                           |
 +---------------------------------------------------------------------------*/

// __getitem__(index int) -> T
static TValue _dq_getitem(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    DequeObject *dq = SELF_AS(deque_type);

    int64_t index = kl_arg_int64(0);
    if (index < 0 || index >= (int64_t)dq_size(dq)) {
        raise_exc_str("Deque index out of range");
        return error_value;
    }

    return *dq_at(dq, (int)index);
}

// __setitem__(index int, v T)
static TValue _dq_setitem(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    DequeObject *dq = SELF_AS(deque_type);

    int64_t index = kl_arg_int64(0);
    if (index < 0 || index >= (int64_t)dq_size(dq)) {
        raise_exc_str("Deque index out of range");
        return error_value;
    }

    /* No structural change: existing iterators stay valid. */
    *dq_at(dq, (int)index) = args[1];
    return no_value;
}

/*---------------------------------------------------------------------------+
 |  Push / pop / peek at both ends                                           |
 +---------------------------------------------------------------------------*/

// append(v T)
static TValue _dq_append(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    DequeObject *dq = SELF_AS(deque_type);
    dq_push_back(dq, args[0]);
    return no_value;
}

// push_front(v T)
static TValue _dq_push_front(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    DequeObject *dq = SELF_AS(deque_type);
    dq_push_front(dq, args[0]);
    return no_value;
}

// push_back(v T)
static TValue _dq_push_back(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    DequeObject *dq = SELF_AS(deque_type);
    dq_push_back(dq, args[0]);
    return no_value;
}

// pop_front() -> T
static TValue _dq_pop_front(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    DequeObject *dq = SELF_AS(deque_type);

    TValue value = nil_value;
    if (ring_pop_front(&dq->ring, &value)) {
        raise_exc_str("pop from empty Deque");
        return error_value;
    }

    ++dq->mod_count;
    return value;
}

// pop_back() -> T
static TValue _dq_pop_back(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    DequeObject *dq = SELF_AS(deque_type);

    TValue value = nil_value;
    if (ring_pop_back(&dq->ring, &value)) {
        raise_exc_str("pop from empty Deque");
        return error_value;
    }

    ++dq->mod_count;
    return value;
}

// peek_front() -> T
static TValue _dq_peek_front(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    DequeObject *dq = SELF_AS(deque_type);

    TValue *front = ring_peek_front(&dq->ring);
    if (!front) {
        raise_exc_str("peek from empty Deque");
        return error_value;
    }

    return *front;
}

// peek_back() -> T
static TValue _dq_peek_back(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    DequeObject *dq = SELF_AS(deque_type);

    TValue *back = ring_peek_back(&dq->ring);
    if (!back) {
        raise_exc_str("peek from empty Deque");
        return error_value;
    }

    return *back;
}

/*---------------------------------------------------------------------------+
 |  Structural modification                                                  |
 +---------------------------------------------------------------------------*/

// insert(index int, v T)
static TValue _dq_insert(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    DequeObject *dq = SELF_AS(deque_type);

    int64_t index = kl_arg_int64(0);
    if (index < 0 || index > (int64_t)dq_size(dq)) {
        raise_exc_str("Deque insert index out of range");
        return error_value;
    }

    /* O(n): the ring is flattened first, so the tail moves with one memmove.
     * Only OOM can fail, and `mm_alloc` aborts on OOM. */
    ring_insert(&dq->ring, (int)index, &args[1]);
    ++dq->mod_count;
    return no_value;
}

// remove(value T)
static TValue _dq_remove(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    DequeObject *dq = SELF_AS(deque_type);

    int size = dq_size(dq);
    for (int i = 0; i < size; ++i) {
        TValue eq = dq_item_equal(&args[0], dq_at(dq, i));
        if (is_error(&eq)) return error_value;
        if (to_bool(&eq)) {
            ring_remove(&dq->ring, i, NULL);
            ++dq->mod_count;
            return no_value;
        }
    }

    raise_exc_str("Deque.remove(x): x not in Deque");
    return error_value;
}

// pop(index = -1) -> T
static TValue _dq_pop(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    DequeObject *dq = SELF_AS(deque_type);

    int64_t n = (int64_t)dq_size(dq);
    if (n == 0) {
        raise_exc_str("pop from empty Deque");
        return error_value;
    }

    int64_t index = kl_arg_int64(0);
    if (index < 0) index += n; /* normalize negative index */
    if (index < 0 || index >= n) {
        raise_exc_str("pop index out of range");
        return error_value;
    }

    TValue value = nil_value;

    if (index == 0) {
        /* Both ends stay O(1), only the middle pays for the shift. */
        ring_pop_front(&dq->ring, &value);
    } else if (index == n - 1) {
        ring_pop_back(&dq->ring, &value);
    } else {
        ring_remove(&dq->ring, (int)index, &value);
    }

    ++dq->mod_count;
    return value;
}

// clear()
static TValue _dq_clear(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    DequeObject *dq = SELF_AS(deque_type);
    /* Keep the buffer, drop the elements. */
    ring_clear(&dq->ring);
    ++dq->mod_count;
    return no_value;
}

// extend(items Iterable[T])
static TValue _dq_extend(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    DequeObject *dq = SELF_AS(deque_type);
    Object *ob = kl_arg_obj(0);

    if (IS_DEQUE(ob)) {
        DequeObject *other = (DequeObject *)ob;
        /* Snapshot the length first: `dq.extend(dq)` must terminate. */
        int n = dq_size(other);
        for (int i = 0; i < n; ++i) {
            dq_push_back(dq, *dq_at(other, i));
        }
        return no_value;
    }

    if (IS_TUPLE(ob)) {
        TupleObject *other = (TupleObject *)ob;
        for (size_t i = 0; i < other->size; ++i) {
            dq_push_back(dq, other->array[i]);
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
            dq_push_back(dq, int64_value(value));
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
            dq_push_back(dq, item);
        }
        return no_value;
    }

    if (kl_is_iter(ob)) {
        TValue it = kl_call_iter(args, 1);
        if (is_error(&it)) return error_value;

        TValue item = kl_call_next(&it);
        while (!is_error(&item) && !is_nil(&item)) {
            dq_push_back(dq, item);
            item = kl_call_next(&it);
        }
        return is_error(&item) ? error_value : no_value;
    }

    raise_exc_str("Deque.extend: unsupported argument");
    return error_value;
}

/*---------------------------------------------------------------------------+
 |  Format                                                                   |
 +---------------------------------------------------------------------------*/

// fmt(f Formatter)
static TValue _dq_fmt(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    DequeObject *dq = SELF_AS(deque_type);
    Formatter *fmt = kl_arg_fmt(0);

    int size = dq_size(dq);

    buf_write_str(&fmt->buf, "Deque([");

    if (size <= DEQUE_FMT_MAX) {
        for (int i = 0; i < size; ++i) {
            if (i > 0) buf_write_str(&fmt->buf, ", ");
            kl_call_fmt(dq_at(dq, i), fmt);
        }
    } else {
        /* Show the first DEQUE_FMT_EDGE and the last DEQUE_FMT_EDGE items. */
        for (int i = 0; i < DEQUE_FMT_EDGE; ++i) {
            if (i > 0) buf_write_str(&fmt->buf, ", ");
            kl_call_fmt(dq_at(dq, i), fmt);
        }

        buf_write_str(&fmt->buf, ", ..., ");

        for (int i = size - DEQUE_FMT_EDGE; i < size; ++i) {
            if (i > size - DEQUE_FMT_EDGE) buf_write_str(&fmt->buf, ", ");
            kl_call_fmt(dq_at(dq, i), fmt);
        }
    }

    buf_write_str(&fmt->buf, "])");
    return no_value;
}

/*---------------------------------------------------------------------------+
 |  Iterators                                                                |
 |                                                                           |
 |  Any push may reallocate the ring's buffer, so the cursor is a live index |
 |  (0 ..< size) instead of a pointer into it: growing the buffer never      |
 |  dangles the cursor. `mod_count` guards the element sequence itself, so a |
 |  push or a pop during traversal raises instead of silently skipping or    |
 |  repeating an element.                                                    |
 |                                                                           |
 |  `DequeIter` and `DequeRevIter` share this object layout and the same C   |
 |  functions; only `reverse` differs.                                       |
 +---------------------------------------------------------------------------*/

typedef struct _DequeIterObject {
    OBJECT_HEAD
    /* Source deque, kept reachable for the whole iteration. */
    DequeObject *owner;
    /* Next live index to yield; outside [0, size) means exhausted. */
    int64_t cur;
    int64_t step;
    /* Snapshot of owner->mod_count, checked before every element. */
    uint32_t mod_count;
    /* 0: forward, 1: backward. */
    int reverse;
} DequeIterObject;

static TValue dq_iter_init(DequeIterObject *iter, TValue *args, int nargs, int reverse)
{
    ASSERT(nargs == 2);

    DequeObject *owner = kl_arg_obj_as(0, deque_type);
    int64_t step = kl_arg_int64(1);
    if (step <= 0) {
        raise_exc_str("step must be positive");
        return error_value;
    }

    iter->owner = owner;
    iter->step = step;
    iter->reverse = reverse;
    iter->mod_count = owner->mod_count;

    int size = dq_size(owner);
    iter->cur = (size == 0) ? -1 : (reverse ? size - 1 : 0);

    return no_value;
}

/* DequeIter[T].__init__(src Deque[T], step int) */
static TValue _dq_iter_init(TValue *self, TValue *args, int nargs)
{
    Object *ob = to_obj(self);
    ASSERT(OB_TYPE(ob) == &deque_iter_type);
    return dq_iter_init((DequeIterObject *)ob, args, nargs, 0);
}

/* DequeRevIter[T].__init__(src Deque[T], step int) */
static TValue _dq_rev_iter_init(TValue *self, TValue *args, int nargs)
{
    Object *ob = to_obj(self);
    ASSERT(OB_TYPE(ob) == &deque_rev_iter_type);
    return dq_iter_init((DequeIterObject *)ob, args, nargs, 1);
}

/* has_next() bool */
static TValue _dq_iter_has_next(TValue *self, TValue *args, int nargs)
{
    Object *ob = to_obj(self);
    TypeObject *tp = OB_TYPE(ob);
    ASSERT(tp == &deque_iter_type || tp == &deque_rev_iter_type);

    DequeIterObject *iter = (DequeIterObject *)ob;
    return bool_value(iter->cur >= 0 && iter->cur < (int64_t)dq_size(iter->owner));
}

static TValue dq_iter_next(DequeIterObject *iter)
{
    /* Checked before reading the element: a mutation may have moved it. */
    if (iter->mod_count != iter->owner->mod_count) {
        raise_exc_str("Deque modified during iteration");
        return error_value;
    }

    if (iter->cur < 0 || iter->cur >= (int64_t)dq_size(iter->owner)) return nil_value;

    TValue value = *dq_at(iter->owner, (int)iter->cur);
    iter->cur += iter->reverse ? -iter->step : iter->step;
    return value;
}

/* next() T? */
static TValue _dq_iter_next(TValue *self, TValue *args, int nargs)
{
    Object *ob = to_obj(self);
    TypeObject *tp = OB_TYPE(ob);
    ASSERT(tp == &deque_iter_type || tp == &deque_rev_iter_type);

    return dq_iter_next((DequeIterObject *)ob);
}

/* next_strict() T */
static TValue _dq_iter_next_strict(TValue *self, TValue *args, int nargs)
{
    TValue ret = _dq_iter_next(self, args, nargs);
    if (is_error(&ret)) return error_value;
    if (is_nil(&ret)) {
        raise_exc_str("No more elements in DequeIter");
        return error_value;
    }
    return ret;
}

/* next_strict() T */
static TValue _dq_rev_iter_next_strict(TValue *self, TValue *args, int nargs)
{
    TValue ret = _dq_iter_next(self, args, nargs);
    if (is_error(&ret)) return error_value;
    if (is_nil(&ret)) {
        raise_exc_str("No more elements in DequeRevIter");
        return error_value;
    }
    return ret;
}

/*---------------------------------------------------------------------------+
 |  Type objects                                                             |
 +---------------------------------------------------------------------------*/

static MethodDef deque_methods[] = {
    { "__init__", _dq_init },
    { "len", _dq_len },
    { "empty", _dq_empty },
    { "__contains__", _dq_contains },
    { "index", _dq_index },
    { "count", _dq_count },
    { "__getitem__", _dq_getitem },
    { "__setitem__", _dq_setitem },
    { "append", _dq_append },
    { "extend", _dq_extend },
    { "insert", _dq_insert },
    { "remove", _dq_remove },
    { "pop", _dq_pop },
    { "clear", _dq_clear },
    { "push_front", _dq_push_front },
    { "push_back", _dq_push_back },
    { "pop_front", _dq_pop_front },
    { "pop_back", _dq_pop_back },
    { "peek_front", _dq_peek_front },
    { "peek_back", _dq_peek_back },
    { "fmt", _dq_fmt },
    { NULL },
};

/* pub class Deque[T] : MutableSequence[T] { ... } */
TypeObject deque_type = {
    ._type = &type_type,
    .name = "Deque",
    .flags = TP_FLAGS_CLASS,
    .objsize = sizeof(DequeObject),
    .methdefs = deque_methods,
};

/* class DequeIter[T] : Iterator[T] { ... } */
TypeObject deque_iter_type = {
    ._type = &type_type,
    .name = "DequeIter",
    .flags = TP_FLAGS_CLASS,
    .objsize = sizeof(DequeIterObject),
    .methdefs =
        (MethodDef[]){
            { "__init__", _dq_iter_init },
            { "has_next", _dq_iter_has_next },
            { "next", _dq_iter_next },
            { "next_strict", _dq_iter_next_strict },
            { NULL, NULL },
        },
};

/* class DequeRevIter[T] : Iterator[T] { ... } */
TypeObject deque_rev_iter_type = {
    ._type = &type_type,
    .name = "DequeRevIter",
    .flags = TP_FLAGS_CLASS,
    .objsize = sizeof(DequeIterObject),
    .methdefs =
        (MethodDef[]){
            { "__init__", _dq_rev_iter_init },
            { "has_next", _dq_iter_has_next },
            { "next", _dq_iter_next },
            { "next_strict", _dq_rev_iter_next_strict },
            { NULL, NULL },
        },
};

Object *kl_new_deque(void)
{
    DequeObject *dq = mm_alloc_obj(dq);
    INIT_OBJECT_HEAD(dq, &deque_type, 0);
    ring_init(&dq->ring, DEQUE_ELEM_SIZE);
    dq->mod_count = 0;
    return (Object *)dq;
}

#ifdef __cplusplus
}
#endif
