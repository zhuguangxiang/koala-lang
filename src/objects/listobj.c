/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "listobj.h"
#include "buffer.h"

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

static TValue _list_push(TValue *self, TValue *args, int nargs)
{
    ListObject *list = SELF_AS(list_type);
    expand_capacity(list, list->end + 1);
    // write_barrier(list, args[0]);
    list->array[list->end++] = args[0];
    return none_value;
}

static TValue _list_pop(TValue *self, TValue *args, int nargs)
{
    ListObject *list = SELF_AS(list_type);
    ASSERT(nargs == 1);
    int64_t index = to_int64(&args[0]);
    if (index == 0) {
        // pop head
        if (list->end == list->start) {
            panic("pop from empty list");
            return none_value;
        }
        return list->array[list->start++];
    } else {
        if (list->end == list->start) {
            panic("pop from empty list");
            return none_value;
        }
        return list->array[--list->end];
    }
}

static TValue _list_len(TValue *self, TValue *args, int nargs)
{
    ListObject *list = SELF_AS(list_type);
    return int64_value(list->end - list->start);
}

static TValue _list_str(TValue *self, TValue *args, int nargs)
{
    ListObject *list = SELF_AS(list_type);
    int64_t start = list->start;
    int64_t stop = list->end;
    BUF(buf);
    buf_write_char(&buf, '[');
    for (int i = start; i < stop; ++i) {
        if (i > start) {
            buf_write_str(&buf, ", ");
        }
        Object *_s = kl_to_str(list->array + i);
        buf_write_str(&buf, STR_BUF(_s));
    }
    buf_write_char(&buf, ']');
    Object *sobj = kl_new_nstr(BUF_STR(buf), BUF_LEN(buf));
    FINI_BUF(buf);
    return obj_value(sobj);
}

static TValue _list_extend(TValue *self, TValue *args, int nargs)
{
    ListObject *list = SELF_AS(list_type);
    ASSERT(nargs == 1);
    Object *ob = kl_arg_obj(0);

    if (IS_LIST(ob)) {
        ListObject *other = (ListObject *)ob;
        expand_capacity(list, list->end + other->end - other->start);
        memcpy(list->array + list->end, other->array + other->start,
               sizeof(TValue) * (other->end - other->start));
        list->end += other->end - other->start;
        return none_value;
    } else {
        NYI();
    }
}

static TValue _list_getitem(TValue *self, TValue *args, int nargs)
{
    ListObject *list = SELF_AS(list_type);
    size_t index = to_int64(args);

    if (index >= list->end - list->start) {
        panic("list index out of range");
        return none_value;
    }
    return list->array[list->start + index];
}

static TValue _list_setitem(TValue *self, TValue *args, int nargs)
{
    ListObject *list = SELF_AS(list_type);
    size_t index = to_int64(args);

    if (index >= list->end - list->start) {
        panic("list index out of range");
        return none_value;
    }
    // write_barrier(list, args[1]);
    list->array[list->start + index] = args[1];
    return none_value;
}

static inline void slice_adjust(int64_t *start, int64_t *end, int64_t step, size_t n)
{
    if (step > 0) {
        if (*start < 0) *start += (int64_t)n;
        if (*start < 0) *start = 0;
        if (*start > (int64_t)n) *start = (int64_t)n;
        if (*end < 0) *end += (int64_t)n;
        if (*end < 0) *end = 0;
        if (*end > (int64_t)n) *end = (int64_t)n;
    } else {
        if (*start < 0) *start += (int64_t)n;
        if (*start < -1) *start = -1;
        if (*start > (int64_t)n - 1) *start = (int64_t)n - 1;
        if (*end < 0) *end += (int64_t)n;
        if (*end < -1) *end = -1;
        if (*end > (int64_t)n - 1) *end = (int64_t)n - 1;
    }
}

static Object *list_slice(ListObject *list, SliceObject *slice)
{
    ASSERT(slice->step.ival != 0);

    int64_t start = slice->start.ival;
    int64_t end = slice->end.ival;
    int64_t step = slice->step.ival;

    size_t n = list->end - list->start; /* 逻辑长度 */
    slice_adjust(&start, &end, step, n);

    /* --- step == 1：零拷贝共享视图 --- */
    if (step == 1) {
        if (end <= start) {
            return kl_new_list(); /* 空结果 */
        }
        Object *obj = kl_new_list();
        ListObject *sub = (ListObject *)obj;
        sub->array = list->array; /* 共享 backing */
        sub->start = list->start + (size_t)start;
        sub->end = list->start + (size_t)end;
        sub->capacity = (size_t)(end - start); /* 锁死：push 触发 COW */
        return obj;
    }

    /* --- step != 1：元素不连续，拷到新数组 --- */
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

static MethodDef list_methods[] = {
    { "push", _list_push },
    { "pop", _list_pop },
    { "__len__", _list_len },
    { "__str__", _list_str },
    { "extend", _list_extend },
    { "__getitem__", _list_getitem },
    { "__setitem__", _list_setitem },
    { "__getslice__", _list_getslice },
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
