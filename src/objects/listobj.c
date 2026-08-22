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

static TValue _list_append(TValue *self, TValue *args, int nargs)
{
    ListObject *list = (ListObject *)to_obj(self);
    expand_capacity(list, list->end + 1);
    // write_barrier(list, args[0]);
    list->array[list->end++] = args[0];
    return none_value;
}

static TValue kl_list_pop(TValue *self, TValue *args, int nargs)
{
    ListObject *list = (ListObject *)to_obj(self);
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

static TValue kl_list_len(TValue *self, TValue *args, int nargs)
{
    ListObject *list = (ListObject *)to_obj(self);
    return int64_value(list->end - list->start);
}

static TValue kl_list_str(TValue *self, TValue *args, int nargs)
{
    ListObject *list = (ListObject *)to_obj(self);
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

static TValue kl_list_extend(TValue *self, TValue *args, int nargs)
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

static MethodDef list_methods[] = {
    { "append", _list_append }, { "pop", kl_list_pop },       { "__len__", kl_list_len },
    { "__str__", kl_list_str }, { "extend", kl_list_extend }, { NULL },
};

static size_t kl_list_seq_len(TValue *self)
{
    ListObject *list = (ListObject *)to_obj(self);
    return list->end - list->start;
}

static TValue kl_list_seq_get(TValue *self, size_t index)
{
    ListObject *list = (ListObject *)to_obj(self);
    if (index >= list->end - list->start) {
        panic("list index out of range");
        return none_value;
    }
    return list->array[list->start + index];
}

static void kl_list_seq_set(TValue *self, size_t index, TValue *value)
{
    ListObject *list = (ListObject *)to_obj(self);
    if (index >= list->end - list->start) {
        panic("list index out of range");
    }
    // write_barrier(list, *value);
    list->array[list->start + index] = *value;
}

static SeqMethods list_seq_methods = {
    .len = kl_list_seq_len,
    // .contains = kl_list_contains,
    .get = kl_list_seq_get,
    .set = kl_list_seq_set,
};

/*
pub class list[T] : MutableSequence[T] { ... }
*/
TypeObject list_type = {
    ._type = &type_type,
    .name = "list",
    .flags = TP_FLAGS_CLASS,
    .methdefs = list_methods,
    .seq = &list_seq_methods,
};

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
