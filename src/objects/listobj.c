/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "listobj.h"
#include "buffer.h"
#include "excobj.h"
#include "rangeobj.h"
#include "tupleobj.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LIST_MINIMUM_CAPACITY 4

static void expand_capacity(ListObject *list, size_t new_capacity)
{
    if (new_capacity <= list->capacity) return;

    size_t capacity = list->capacity == 0 ? LIST_MINIMUM_CAPACITY : list->capacity;
    while (capacity < new_capacity) {
        capacity *= 2;
    }

    list->array = mm_realloc(list->array, sizeof(TValue) * capacity);
    list->capacity = capacity;
}

void kl_list_append(Object *ob, TValue item)
{
    ASSERT(IS_LIST(ob));
    ListObject *list = (ListObject *)ob;
    expand_capacity(list, list->end + 1);
    list->array[list->end++] = item;
}

void kl_list_prepend(Object *ob, TValue item)
{
    ASSERT(IS_LIST(ob));
    ListObject *list = (ListObject *)ob;

    // Ensure capacity for one more element
    expand_capacity(list, list->end + 1);

    // Shift elements to the right by 1
    memmove(list->array + 1, list->array, list->end * sizeof(TValue));

    // Insert new item at the front
    list->array[0] = item;

    // Increase element count
    list->end++;
}

/* General-purpose TValue equality, mirroring the VM's OP_EQ layering:
 * value types compare by payload; everything else dispatches __eq__.
 * Only reached for homogeneous typed lists, so mixed-tag fall-through
 * to the slot call assumes the object implements Equatable.
 */
static TValue list_item_equal(TValue *a, TValue *b)
{
    if (is_int(a) && is_int(b)) return bool_value(a->ival == b->ival);
    if (is_uint(a) && is_uint(b)) return bool_value((uint64_t)a->ival == (uint64_t)b->ival);
    if (is_float(a) && is_float(b)) return bool_value(a->fval == b->fval);
    if (is_bool(a) && is_bool(b)) return bool_value(a->ival == b->ival);
    if (is_nil(a) && is_nil(b)) return bool_value(true);
    if (is_nil(a) || is_nil(b)) return bool_value(false);
    return kl_call_cmp(a, b, SLOT_EQ);
}

/* pub func __contains__(v T) bool */
static TValue _list_contains(TValue *self, TValue *args, int nargs)
{
    ListObject *list = SELF_AS(list_type);
    ASSERT(nargs == 1);

    TValue *target = args;

    for (size_t i = list->start; i < list->end; ++i) {
        TValue eq = list_item_equal(target, list->array + i);
        if (is_error(&eq)) return error_value;
        if (to_bool(&eq)) return bool_value(true);
    }

    return bool_value(false);
}

/* pub func index(value T, start = 0, end = -1) int */
static TValue _list_index(TValue *self, TValue *args, int nargs)
{
    ListObject *list = SELF_AS(list_type);
    ASSERT(nargs == 3);

    int64_t start = kl_arg_int64(1);
    int64_t end = kl_arg_int64(2);

    size_t n = list->end - list->start;
    if (end < 0 || (uint64_t)end > n) end = (int64_t)n;
    if (start < 0) start = 0;
    if ((uint64_t)start > n) start = (int64_t)n;

    TValue *target = args;

    for (int64_t i = start; i < end; ++i) {
        TValue eq = list_item_equal(target, list->array + list->start + i);
        if (is_error(&eq)) return error_value;
        if (to_bool(&eq)) return int64_value(i);
    }

    return int64_value(-1);
}

/* pub func count(value T, start = 0, end = -1) int */
static TValue _list_count(TValue *self, TValue *args, int nargs)
{
    ListObject *list = SELF_AS(list_type);
    ASSERT(nargs == 3);

    int64_t start = kl_arg_int64(1);
    int64_t end = kl_arg_int64(2);

    size_t n = list->end - list->start;
    if (end < 0 || (uint64_t)end > n) end = (int64_t)n;
    if (start < 0) start = 0;
    if ((uint64_t)start > n) start = (int64_t)n;

    TValue *target = args;
    int64_t count = 0;

    for (int64_t i = start; i < end; ++i) {
        TValue eq = list_item_equal(target, list->array + list->start + i);
        if (is_error(&eq)) return error_value;
        if (to_bool(&eq)) count++;
    }

    return int64_value(count);
}

static TValue _list_append(TValue *self, TValue *args, int nargs)
{
    ListObject *list = SELF_AS(list_type);
    expand_capacity(list, list->end + 1);
    // write_barrier(list, args[0]);
    list->array[list->end++] = args[0];
    return no_value;
}

static TValue _list_pop(TValue *self, TValue *args, int nargs)
{
    ListObject *list = SELF_AS(list_type);
    ASSERT(nargs == 1);

    int64_t n = (int64_t)(list->end - list->start); /* logical length */
    if (n == 0) {
        raise_exc_str("pop from empty list");
        return error_value;
    }

    int64_t index = kl_arg_int64(0);
    if (index < 0) index += n; /* normalize negative index */
    if (index < 0 || index >= n) {
        raise_exc_str("pop index out of range");
        return error_value;
    }

    TValue *base = list->array + list->start; /* window base */

    if (index == 0) {
        /* pop head: O(1), advance start */
        return list->array[list->start++];
    }

    if (index == n - 1) {
        /* pop tail: O(1), shrink end */
        return list->array[--list->end];
    }

    /* pop middle: read value, shift tail left, shrink end */
    TValue value = base[index];
    memmove(base + index, base + index + 1, (size_t)(n - 1 - index) * sizeof(TValue));
    list->end--;
    return value;
}

static TValue _list_copy(TValue *self, TValue *args, int nargs)
{
    ListObject *list = SELF_AS(list_type);
    ASSERT(nargs == 2);

    int64_t n = (int64_t)(list->end - list->start);
    int64_t start = to_int64(&args[0]);
    int64_t end = to_int64(&args[1]);

    /* negative defaults: start < 0 → 0, end < 0 → n ("to end") */
    if (start < 0) start = 0;
    if (end < 0) end = n;

    /* clamp to [0, n] */
    if (start > n) start = n;
    if (end > n) end = n;
    if (start > end) start = end;

    int64_t len = end - start;
    if (len == 0) return obj_value(kl_new_list());

    TValue *src = list->array + list->start + start;
    Object *copy = kl_list_from_array(src, len);
    return obj_value(copy);
}

static TValue _list_len(TValue *self, TValue *args, int nargs)
{
    ListObject *list = SELF_AS(list_type);
    return int64_value(list->end - list->start);
}

static TValue _list_empty(TValue *self, TValue *args, int nargs)
{
    ListObject *list = SELF_AS(list_type);
    ASSERT(nargs == 0);
    return bool_value(list->end == list->start);
}

static TValue _list_fmt(TValue *self, TValue *args, int nargs)
{
    ListObject *list = SELF_AS(list_type);
    ASSERT(nargs == 1);

    Formatter *fmt = kl_arg_fmt(0);

    buf_write_char(&fmt->buf, '[');
    for (size_t i = list->start; i < list->end; ++i) {
        if (i > list->start) {
            buf_write_str(&fmt->buf, ", ");
        }
        kl_call_fmt(list->array + i, fmt);
    }
    buf_write_char(&fmt->buf, ']');

    return no_value;
}

static TValue _list_extend(TValue *self, TValue *args, int nargs)
{
    ListObject *list = SELF_AS(list_type);
    ASSERT(nargs == 1);
    Object *ob = kl_arg_obj(0);

    if (IS_LIST(ob)) {
        ListObject *other = (ListObject *)ob;
        int m = other->end - other->start;
        if (m <= 0) return no_value;
        expand_capacity(list, list->end + m);
        memcpy(list->array + list->end, other->array + other->start, sizeof(TValue) * m);
        list->end += m;
        return no_value;
    }

    if (IS_TUPLE(ob)) {
        TupleObject *other = (TupleObject *)ob;
        int m = other->size;
        if (m <= 0) return no_value;
        expand_capacity(list, list->end + m);
        memcpy(list->array + list->end, other->array, sizeof(TValue) * m);
        list->end += m;
        return no_value;
    }

    if (IS_RANGE(ob)) {
        RangeObject *other = (RangeObject *)ob;
        int64_t len;
        if (!kl_range_len(other, &len)) return error_value;
        if (len <= 0) return no_value;
        expand_capacity(list, list->end + len);
        for (int64_t i = 0; i < len; ++i) {
            int64_t value;
            if (!kl_range_index(other, i, &value)) return error_value;
            list->array[list->end++] = int64_value(value);
        }
        return no_value;
    }

    if (kl_is_seq(ob)) {
        TValue len = kl_call_len(args);
        if (is_error(&len)) return error_value;
        size_t m = (size_t)to_int64(&len);
        if (m <= 0) return no_value;
        expand_capacity(list, list->end + m);
        for (size_t i = 0; i < m; ++i) {
            TValue idx = int64_value((int64_t)i);
            TValue item = kl_call_get_item(args, &idx);
            if (is_error(&item)) return error_value;
            list->array[list->end++] = item;
        }
        return no_value;
    }

    if (kl_is_iter(ob)) {
        TValue it = kl_call_iter(args, 1);
        if (is_error(&it)) return error_value;

        Vector items;
        vector_init(&items, sizeof(TValue));

        TValue item;
        while ((item = kl_call_next(&it), ((is_error(&item) || is_nil(&item)) ? 0 : 1))) {
            vector_push_back(&items, &item);
        }

        if (is_error(&item)) return error_value;

        int size = vector_size(&items);
        expand_capacity(list, size);

        TValue *items_array = VECTOR_ITEMS(&items, TValue);
        memcpy(list->array + list->start, items_array, sizeof(TValue) * size);

        list->end += size;
        return no_value;
    }

    raise_exc_str("list.extend unreachable!");
    return error_value;
}

/* pub func __iadd__(seq Iterable[T]) list[T] */
static TValue _list_iadd(TValue *self, TValue *args, int nargs)
{
    TValue r = _list_extend(self, args, nargs);
    if (is_error(&r)) return error_value;
    return *self;
}

static TValue _list_getitem(TValue *self, TValue *args, int nargs)
{
    ListObject *list = SELF_AS(list_type);
    size_t index = to_int64(args);

    if (index >= list->end - list->start) {
        raise_exc_str("list index out of range");
        return error_value;
    }
    return list->array[list->start + index];
}

static TValue _list_setitem(TValue *self, TValue *args, int nargs)
{
    ListObject *list = SELF_AS(list_type);
    size_t index = to_int64(args);

    if (index >= list->end - list->start) {
        raise_exc_str("list index out of range");
        return error_value;
    }
    // write_barrier(list, args[1]);
    list->array[list->start + index] = args[1];
    return no_value;
}

static Object *list_slice(ListObject *list, SliceObject *slice)
{
    ASSERT(slice->step.ival != 0);

    int64_t start = slice->start.ival;
    int64_t end = slice->end.ival;
    int64_t step = slice->step.ival;

    size_t n = list->end - list->start;
    slice_adjust(&start, &end, step, n);

    // step == 1, return shared view of the original list
    if (step == 1) {
        if (end <= start) {
            return kl_new_list();
        }
        Object *obj = kl_new_list();
        ListObject *sub = (ListObject *)obj;
        sub->array = list->array;
        sub->start = list->start + (size_t)start;
        sub->end = list->start + (size_t)end;
        sub->capacity = (size_t)(end - start);
        return obj;
    }

    // step != 1, copy elements to a new array
    size_t count;
    if (step > 0) {
        count = (end > start) ? (size_t)((end - start + step - 1) / step) : 0;
    } else {
        count = (start > end) ? (size_t)((start - end - step - 1) / (-step)) : 0;
    }
    if (count == 0) {
        return kl_new_list();
    }

    TValue *arr = mm_alloc(count * sizeof(TValue));
    size_t j = 0;
    if (step > 0) {
        for (int64_t i = start; i < end; i += step) {
            arr[j++] = list->array[list->start + (size_t)i];
        }
    } else {
        for (int64_t i = start; i > end; i += step) {
            arr[j++] = list->array[list->start + (size_t)i];
        }
    }

    Object *obj = kl_new_list();
    ListObject *sub = (ListObject *)obj;
    sub->array = arr;
    sub->start = 0;
    sub->end = count;
    sub->capacity = count;
    return obj;
}

static TValue _list_getslice(TValue *self, TValue *args, int nargs)
{
    ListObject *list = SELF_AS(list_type);
    ASSERT(nargs == 1);
    SliceObject *slice = kl_arg_slice(0);
    return obj_value(list_slice(list, slice));
}

static TValue _list_set_slice(TValue *self, TValue *args, int nargs)
{
    ListObject *list = SELF_AS(list_type);
    ASSERT(nargs == 2);
    SliceObject *slice = kl_arg_slice(0);
    Object *iter = kl_arg_obj(1);

    ASSERT(slice->step.ival != 0);

    int64_t start = slice->start.ival;
    int64_t end = slice->end.ival;
    int64_t step = slice->step.ival;

    int64_t n = list->end - list->start;
    int r = slice_adjust(&start, &end, step, n);
    if (r < 0) {
        raise_exc_str("list.set_slice: index out of range");
        return error_value;
    }

    if (r == 0) return no_value;

    int64_t slots = slice_count(start, end, step);
    TValue *dst_base = list->array + list->start;

    if (IS_LIST(iter)) {
        ListObject *rhs = (ListObject *)iter;
        int64_t m = rhs->end - rhs->start;
        int64_t count = MIN(m, slots);
        TValue *src_base = rhs->array + rhs->start;

        if (step == 1) {
            if (count) memmove(dst_base + start, src_base, count * sizeof(TValue));
        } else {
            int64_t i = start;
            for (int64_t j = 0; j < count; ++j, i += step) {
                dst_base[i] = src_base[j];
            }
        }
        return no_value;
    }

    if (IS_RANGE(iter)) {
        RangeObject *rhs = (RangeObject *)iter;
        int64_t m;
        if (!kl_range_len(rhs, &m)) return error_value;

        int64_t count = MIN(m, slots);

        int64_t i = start;
        for (int64_t j = 0; j < count; ++j, i += step) {
            int64_t value;
            if (!kl_range_index(rhs, (int64_t)j, &value)) return error_value;
            dst_base[i] = int64_value(value);
        }

        return no_value;
    }

    if (IS_TUPLE(iter)) {
        TupleObject *rhs = (TupleObject *)iter;
        int64_t m = rhs->size;
        int64_t count = MIN(m, slots);
        TValue *src_base = rhs->array;

        if (step == 1) {
            if (count) memmove(dst_base + start, src_base, count * sizeof(TValue));
        } else {
            int64_t i = start;
            for (int64_t j = 0; j < count; ++j, i += step) {
                dst_base[i] = src_base[j];
            }
        }
        return no_value;
    }

    if (kl_is_seq(iter)) {
        TValue len = kl_call_len(args + 1);
        if (is_error(&len)) return error_value;

        int64_t m = (int64_t)to_int64(&len);
        int64_t count = MIN(m, slots);

        int64_t i = start;
        for (int64_t j = 0; j < count; ++j, i += step) {
            TValue idx = int64_value((int64_t)j);
            TValue item = kl_call_get_item(args + 1, &idx);
            if (is_error(&item)) return error_value;
            dst_base[i] = item;
        }

        return no_value;
    }

    if (kl_is_iter(iter)) {
        TValue it = kl_call_iter(args + 1, 1);
        if (is_error(&it)) return error_value;

        int64_t i = start;
        for (int64_t j = 0; j < slots; ++j, i += step) {
            TValue item = kl_call_next(&it);
            if (is_error(&item)) return error_value;
            if (is_nil(&item)) break;
            dst_base[i] = item;
        }

        return no_value;
    }

    raise_exc_str("list.set_slice unreachable!");
    return error_value;
}

/* pub func insert(index int, value T) */
static TValue _list_insert(TValue *self, TValue *args, int nargs)
{
    ListObject *list = SELF_AS(list_type);
    ASSERT(nargs == 2);

    int64_t index = kl_arg_int64(0);
    TValue value = args[1];

    int64_t n = list->end - list->start;
    if (index < 0 || index > n) {
        raise_exc_str("list insert index out of range");
        return error_value;
    }

    /* Grow by absolute position (end+1): correct even for windowed views.
     * expand_capacity may realloc, so recompute base afterwards. */
    expand_capacity(list, list->end + 1);
    TValue *base = list->array + list->start;

    /* Shift [idx, n) one slot to the right, then place the new value. */
    memmove(base + index + 1, base + index, (n - index) * sizeof(TValue));
    base[index] = value;
    list->end++;
    return no_value;
}

/* pub func remove(value T) */
static TValue _list_remove(TValue *self, TValue *args, int nargs)
{
    ListObject *list = SELF_AS(list_type);
    ASSERT(nargs == 1);

    TValue *target = args + 0;
    int64_t n = list->end - list->start;
    TValue *base = list->array + list->start;

    int64_t found = n;
    for (int64_t i = 0; i < n; ++i) {
        TValue eq = list_item_equal(target, base + i);
        if (is_error(&eq)) return error_value;
        if (to_bool(&eq)) {
            found = i;
            break;
        }
    }

    if (found == n) {
        raise_exc_str("list.remove(x): x not in list");
        return error_value;
    }

    /* Shift the tail one slot to the left to overwrite the removed element. */
    memmove(base + found, base + found + 1, (n - found - 1) * sizeof(TValue));
    list->end--;
    return no_value;
}

/* pub func delete(start int, end int? = nil) */
static TValue _list_delete(TValue *self, TValue *args, int nargs)
{
    ListObject *list = SELF_AS(list_type);
    ASSERT(nargs == 2);

    int64_t n = (int64_t)(list->end - list->start);

    /* Negative `start` counts from the end, like pop/__getitem__ */
    int64_t start = kl_arg_int64(0);
    if (start < 0) start += n;

    /* Panic if `start` is out of bounds */
    if (start < 0 || start > n) {
        raise_exc_str("list delete: start out of bounds");
        return error_value;
    }

    /* nil end means "to the end of the list" */
    int64_t end = is_nil(&args[1]) ? n : kl_arg_int64(1);
    if (end < 0) end += n;

    /* An empty range (start >= end) is a no-op */
    if (end <= start) return no_value;

    /* Clamp `end` to the current length */
    if (end > n) end = n;

    /* Remove [start, end) by shifting the rest down, inside the window */
    TValue *base = list->array + list->start;
    memmove(base + start, base + end, (size_t)(n - end) * sizeof(TValue));
    list->end -= (end - start);
    return no_value;
}

/* pub func clear() */
static TValue _list_clear(TValue *self, TValue *args, int nargs)
{
    ListObject *list = SELF_AS(list_type);
    ASSERT(nargs == 0);

    /* Logical empty; keep the backing buffer for reuse. */
    list->end = list->start = 0;
    return no_value;
}

/* pub func reverse() */
static TValue _list_reverse(TValue *self, TValue *args, int nargs)
{
    ListObject *list = SELF_AS(list_type);
    ASSERT(nargs == 0);

    int64_t n = list->end - list->start;
    if (n < 2) return no_value; /* empty / single: nothing to swap (and guards underflow) */

    TValue *base = list->array + list->start;
    for (int64_t i = 0, j = n - 1; i < j; ++i, --j) {
        TValue tmp = base[i];
        base[i] = base[j];
        base[j] = tmp;
    }
    return no_value;
}

/* pub func __add__(seq list[T]) list[T] */
static TValue _list_add(TValue *self, TValue *args, int nargs)
{
    ListObject *list = SELF_AS(list_type);
    ASSERT(nargs == 1);

    ListObject *other = kl_arg_obj_as(0, list_type);

    int64_t n = list->end - list->start;   /* self logical length */
    int64_t m = other->end - other->start; /* seq  logical length */

    /* fresh 0-based result: one allocation, two memcpys, no shared backing */
    Object *res = kl_new_list();
    ListObject *out = (ListObject *)res;
    expand_capacity(out, n + m);
    if (n > 0) memcpy(out->array, list->array + list->start, n * sizeof(TValue));
    if (m > 0) memcpy(out->array + n, other->array + other->start, m * sizeof(TValue));
    out->end = n + m;
    return obj_value(res);
}

static MethodDef list_methods[] = {
    { "len", _list_len },
    { "empty", _list_empty },
    { "__getitem__", _list_getitem },
    { "__setitem__", _list_setitem },
    { "__contains__", _list_contains },
    { "index", _list_index },
    { "count", _list_count },
    { "__getslice__", _list_getslice },
    { "__setslice__", _list_set_slice },
    { "append", _list_append },
    { "pop", _list_pop },
    { "copy", _list_copy },
    { "fmt", _list_fmt },
    { "extend", _list_extend },
    { "insert", _list_insert },
    { "remove", _list_remove },
    { "delete", _list_delete },
    { "clear", _list_clear },
    { "reverse", _list_reverse },
    { "__add__", _list_add },
    { "__iadd__", _list_iadd },
    { NULL },
};

/* pub class list[T] : MutableSequence[T] { ... } */
DEFINE_TYPE(list, TP_FLAGS_CLASS, sizeof(ListObject), list_methods, NULL);

Object *kl_new_list(void)
{
    ListObject *list = mm_alloc_obj(list);
    INIT_OBJECT_HEAD(list, &list_type, 0);
    list->start = 0;
    list->end = 0;
    return (Object *)list;
}

Object *kl_list_from_array(TValue *items, int size)
{
    ListObject *list = mm_alloc_obj(list);
    INIT_OBJECT_HEAD(list, &list_type, 0);
    expand_capacity(list, size);
    memcpy(list->array, items, sizeof(TValue) * size);
    list->start = 0;
    list->end = size;
    return (Object *)list;
}

#ifdef __cplusplus
}
#endif
