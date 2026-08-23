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

static TValue _tuple_tolist(TValue *self, TValue *args, int nargs)
{
    TupleObject *tuple = (TupleObject *)to_obj(self);
    Object *lst = kl_list_from_array(tuple->array, tuple->size);
    return obj_value(lst);
}

static TValue _tuple_str(TValue *self, TValue *args, int nargs)
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

static TValue _tuple_len(TValue *self, TValue *args, int nargs)
{
    TupleObject *tuple = SELF_AS(tuple_type);
    return int64_value((int64_t)tuple->size);
}

static TValue _tuple_getitem(TValue *self, TValue *args, int nargs)
{
    TupleObject *tuple = (TupleObject *)to_obj(self);
    size_t index = to_int64(args);

    if (index >= tuple->size) {
        panic("tuple index out of range");
        return none_value;
    }
    return tuple->array[index];
}

static MethodDef tuple_methods[] = {
    { "to_list", _tuple_tolist },
    { "__str__", _tuple_str },
    { "__len__", _tuple_len },
    { "__getitem__", _tuple_getitem },
    { NULL },
};

/* pub class tuple[infer T] : Sequence[T] { ... } */
DEFINE_TYPE(tuple, TP_FLAGS_CLASS, sizeof(TupleObject), tuple_methods, NULL);

Object *kl_new_tuple(TValue *items, int count)
{
    int msize = sizeof(TupleObject) + count * sizeof(TValue);
    TupleObject *x = mm_alloc(msize);
    INIT_OBJECT_HEAD(x, &tuple_type, 0);

    x->size = count;

    TValue *data = x->array;
    memcpy(data, items, count * sizeof(TValue));

    return (Object *)x;
}

void kl_free_tuple(Object *obj) { mm_free(obj); }

#ifdef __cplusplus
}
#endif
