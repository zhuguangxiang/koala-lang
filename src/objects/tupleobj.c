/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "tupleobj.h"
#include "buffer.h"
#include "excobj.h"
#include "gc.h"
#include "listobj.h"

#ifdef __cplusplus
extern "C" {
#endif

static TValue _tuple_tolist(TValue *self, TValue *args, int nargs)
{
    TupleObject *tuple = SELF_AS(tuple_type);
    Object *lst = kl_list_from_array(TUPLE_ITEMS(tuple), TUPLE_SIZE(tuple));
    return obj_value(lst);
}

static TValue _tuple_fmt(TValue *self, TValue *args, int nargs)
{
    TupleObject *tuple = SELF_AS(tuple_type);
    ASSERT(nargs == 1);

    Formatter *fmt = kl_arg_obj_as(0, fmt_type);

    buf_write_char(&fmt->buf, '(');
    for (size_t i = 0; i < TUPLE_SIZE(tuple); i++) {
        if (i > 0) {
            buf_write_str(&fmt->buf, ", ");
        }
        TValue *item = TUPLE_ITEMS(tuple) + i;
        kl_call_fmt(item, fmt);
    }
    buf_write_char(&fmt->buf, ')');

    return no_value;
}

static TValue _tuple_len(TValue *self, TValue *args, int nargs)
{
    TupleObject *tuple = SELF_AS(tuple_type);
    return int64_value((int64_t)TUPLE_SIZE(tuple));
}

static TValue _tuple_getitem(TValue *self, TValue *args, int nargs)
{
    TupleObject *tuple = SELF_AS(tuple_type);
    size_t index = to_int64(args);

    if (index >= TUPLE_SIZE(tuple)) {
        raise_exc_str("tuple index out of range");
        return error_value;
    }
    return TUPLE_ITEMS(tuple)[index];
}

static Object *tuple_new_view(TValue *values, size_t size, size_t start, size_t end)
{
    TupleObject *x = gc_alloc_obj(x);
    INIT_OBJECT_HEAD(x, &tuple_type, 0);
    x->values = values;
    x->size = size;
    x->start = start;
    x->end = end;
    return (Object *)x;
}

static TValue _tuple_getslice(TValue *self, TValue *args, int nargs)
{
    TupleObject *tuple = SELF_AS(tuple_type);
    ASSERT(nargs == 1);
    SliceObject *slice = kl_arg_slice(0);
    ASSERT(slice->step.ival != 0);

    int64_t start = slice->start.ival;
    int64_t end = slice->end.ival;
    int64_t step = slice->step.ival;
    slice_adjust(&start, &end, step, (int64_t)TUPLE_SIZE(tuple));

    int64_t count = slice_count(start, end, step);
    if (count <= 0) return obj_value(kl_new_tuple(NULL, 0));

    // step == 1, share the elements of the source tuple
    if (step == 1) {
        size_t base = tuple->start;
        return obj_value(tuple_new_view(tuple->values, tuple->size, base + start, base + end));
    }

    // step != 1, copy elements to a new values block
    TValue *values = gc_alloc_values((int)count);
    TValue *src = TUPLE_ITEMS(tuple);
    for (int64_t i = 0; i < count; i++) {
        values[i] = src[start + i * step];
    }
    return obj_value(tuple_new_view(values, (size_t)count, 0, (size_t)count));
}

static void tuple_mark(Object *self)
{
    TupleObject *tuple = (TupleObject *)self;
    if (tuple->values) gc_mark(tuple->values);
}

static MethodDef tuple_methods[] = {
    { "to_list", _tuple_tolist },
    { "fmt", _tuple_fmt },
    { "len", _tuple_len },
    { "__getitem__", _tuple_getitem },
    { "__getslice__", _tuple_getslice },
    { NULL },
};

/* pub class tuple[infer T] : Sequence[T] { ... } */
TypeObject tuple_type = {
    ._type = &type_type,
    .name = "tuple",
    .flags = TP_FLAGS_CLASS,
    .objsize = sizeof(TupleObject),
    .methdefs = tuple_methods,
    ._mark = tuple_mark,
};

Object *kl_new_tuple(TValue *items, int count)
{
    TValue *values = NULL;
    if (count > 0) {
        values = gc_alloc_values(count);
        memcpy(values, items, count * sizeof(TValue));
    }
    return tuple_new_view(values, (size_t)count, 0, (size_t)count);
}

/* a perm tuple for the const pool; its items must be perm (or values) too */
Object *kl_new_perm_tuple(TValue *items, int count)
{
    TupleObject *x = gc_alloc_perm_obj(x, 0);
    INIT_OBJECT_HEAD(x, &tuple_type, 0);
    x->values = NULL;
    x->size = 0;
    if (count > 0) {
        x->values = gc_alloc_perm_values(count);
        x->size = count;
        memcpy(x->values, items, count * sizeof(TValue));
    }
    x->start = 0;
    x->end = count;
    return (Object *)x;
}

#ifdef __cplusplus
}
#endif
