/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "tupleobj.h"
#include "buffer.h"
#include "listobj.h"

#ifdef __cplusplus
extern "C" {
#endif

static TValue kl_tuple_tolist(TValue *self, TValue *args, int nargs)
{
    TupleObject *tuple = (TupleObject *)to_obj(self);
    Object *lst = kl_new_list(tuple->array, tuple->size);
    return obj_value(lst);
}

static TValue tuple_str(TValue *self, TValue *args, int nargs)
{
    TupleObject *tuple = (TupleObject *)to_obj(self);
    BUF(buf);
    buf_write_char(&buf, '(');
    for (size_t i = 0; i < tuple->size; i++) {
        if (i > 0) {
            buf_write_str(&buf, ", ");
        }
        TValue *item = &tuple->array[i];
        Object *sobj = kl_to_str(item);
        buf_write_str(&buf, STR_BUF(sobj));
    }
    buf_write_char(&buf, ')');

    Object *sobj = kl_new_nstr(BUF_STR(buf), BUF_LEN(buf));
    FINI_BUF(buf);
    return obj_value(sobj);
}

static MethodDef tuple_methods[] = {
    { "to_list", kl_tuple_tolist },
    { "__str__", tuple_str },
    { NULL },
};

static size_t kl_tuple_seq_len(TValue *self)
{
    TupleObject *tuple = (TupleObject *)to_obj(self);
    return tuple->size;
}

static TValue kl_tuple_seq_get(TValue *self, size_t index)
{
    TupleObject *tuple = (TupleObject *)to_obj(self);
    if (index >= tuple->size) {
        panic("tuple index out of range");
        return none_value;
    }
    return tuple->array[index];
}

static SeqMethods tuple_seq_methods = {
    .len = kl_tuple_seq_len,
    // .contains = kl_tuple_seq_contains,
    .get = kl_tuple_seq_get,
};

TypeObject tuple_type = {
    ._type = &type_type,
    .name = "tuple",
    .flags = TP_FLAGS_CLASS | TP_FLAGS_PUBLIC,
    .methdefs = tuple_methods,
    .seq = &tuple_seq_methods,
};

Object *kl_new_tuple(TValue *items, int count)
{
    int msize = sizeof(TupleObject) + count * sizeof(TValue);
    TupleObject *x = mm_alloc(msize);
    INIT_OBJECT_HEAD(x, &tuple_type);

    x->size = count;

    TValue *data = x->array;
    memcpy(data, items, count * sizeof(TValue));

    return (Object *)x;
}

#ifdef __cplusplus
}
#endif
