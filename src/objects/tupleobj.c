/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "tupleobj.h"

#ifdef __cplusplus
extern "C" {
#endif

static TValue tuple_str(TValue *self, TValue *args, int nargs) { return *self; }

static MethodDef tuple_methods[] = {
    { "__str__", tuple_str },
    { NULL },
};

TypeObject tuple_type = {
    ._type = &type_type,
    .name = "tuple",
    .flags = TP_FLAGS_CLASS,
    .methdefs = tuple_methods,
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
