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

static TValue kl_list_append(TValue *self, TValue *args, int nargs)
{
    ListObject *list = (ListObject *)to_obj(self);
    expand_capacity(list, list->end + 1);
    // write_barrier(list, args[0]);
    list->array[list->end++] = args[0];
    return none_value;
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

static MethodDef list_methods[] = {
    { "append", kl_list_append },
    { "__len__", kl_list_len },
    { "__str__", kl_list_str },
    { NULL },
};

/*
pub class list[T] : MutableSequence[T] { ... }
*/
TypeObject list_type = {
    ._type = &type_type,
    .name = "list",
    .flags = TP_FLAGS_CLASS | TP_FLAGS_PUBLIC,
    .methdefs = list_methods,
};

Object *kl_new_list(TValue *items, int size)
{
    ListObject *list = mm_alloc_obj(list);
    INIT_OBJECT_HEAD(list, &list_type);
    expand_capacity(list, size);
    memcpy(list->array, items, sizeof(TValue) * size);
    list->start = 0;
    list->end = size;
    return (Object *)list;
}

#ifdef __cplusplus
}
#endif
