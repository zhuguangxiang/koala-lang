/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "exception.h"
#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------Collection trait-----------------------------*/

static Value collection_not_impl_len(Value *self)
{
    raise_exc_str("'__len__()' in trait 'Collection' is not implemented");
    return error_value;
}

static Value collection_not_impl_empty(Value *self)
{
    raise_exc_str("'empty()' in trait 'Collection' is not implemented");
    return error_value;
}

static MethodDef Collection_methods[] = {
    { "__len__", collection_not_impl_len, METH_NO_ARGS },
    { "empty", collection_not_impl_empty, METH_NO_ARGS },
    { NULL },
};

static BaseDef Collection_bases[] = {
    { &Iterable_type },
    { NULL },
};

// clang-format off
TypeObject Collection_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name     = "Collection",
    .flags    = TP_FLAGS_TRAIT,
    .methdefs = Collection_methods,
    .basedefs = Collection_bases,
};
// clang-format on

/*------------------------------Sequence trait-------------------------------*/

static Value seq_not_impl_getitem(Value *self, Value *index)
{
    raise_exc_str("'__getitem__()' in trait 'Sequence' is not implemented");
    return error_value;
}

static Value seq_not_impl_getslice(Value *self, Value *slice)
{
    raise_exc_str("'__getslice__()' in trait 'Sequence' is not implemented");
    return error_value;
}

static Value seq_not_impl_contains(Value *self, Value *arg)
{
    raise_exc_str("'__contains__()' in trait 'Sequence' is not implemented");
    return error_value;
}

static Value seq_not_impl_index(Value *self, Value *arg)
{
    raise_exc_str("'index()' in trait 'Sequence' is not implemented");
    return error_value;
}

static Value seq_not_impl_count(Value *self, Value *arg)
{
    raise_exc_str("'count()' in trait 'Sequence' is not implemented");
    return error_value;
}

static MethodDef Sequence_methods[] = {
    { "__getitem__", seq_not_impl_getitem, METH_ONE_ARG },
    { "__getslice__", seq_not_impl_getslice, METH_ONE_ARG },
    { "__contains__", seq_not_impl_contains, METH_ONE_ARG },
    { "index", seq_not_impl_index, METH_ONE_ARG },
    { "count", seq_not_impl_count, METH_ONE_ARG },
    { NULL },
};

static BaseDef Sequence_bases[] = {
    { &Collection_type },
    { NULL },
};

// clang-format off
TypeObject Sequence_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name     = "Sequence",
    .flags    = TP_FLAGS_TRAIT,
    .basedefs = Sequence_bases,
    .methdefs = Sequence_methods,
};
// clang-format on

/*---------------------------MutableSequence trait---------------------------*/

static Value mutable_seq_not_impl_setitem(Value *self, Value *args, int nargs)
{
    raise_exc_str("'__setitem__()' in trait 'MutableSequence' is not implemented");
    return error_value;
}

static Value mutable_seq_not_impl_setslice(Value *self, Value *args, int nargs)
{
    raise_exc_str("'__setslice__()' in trait 'MutableSequence' is not implemented");
    return error_value;
}

static Value mutable_seq_not_impl_append(Value *self, Value *arg)
{
    raise_exc_str("'append()' in trait 'MutableSequence' is not implemented");
    return error_value;
}

static Value mutable_seq_not_impl_extend(Value *self, Value *arg)
{
    raise_exc_str("'extend()' in trait 'MutableSequence' is not implemented");
    return error_value;
}

static Value mutable_seq_not_impl_insert(Value *self, Value *args, int nargs)
{
    raise_exc_str("'insert()' in trait 'MutableSequence' is not implemented");
    return error_value;
}

static Value mutable_seq_not_impl_remove(Value *self, Value *arg)
{
    raise_exc_str("'remove()' in trait 'MutableSequence' is not implemented");
    return error_value;
}

static Value mutable_seq_not_impl_pop(Value *self, Value *index)
{
    raise_exc_str("'pop()' in trait 'MutableSequence' is not implemented");
    return error_value;
}

static Value mutable_seq_not_impl_clear(Value *self)
{
    raise_exc_str("'clear()' in trait 'MutableSequence' is not implemented");
    return error_value;
}

static MethodDef MutableSequence_methods[] = {
    { "__setitem__", mutable_seq_not_impl_setitem, METH_VAR_ARGS },
    { "__setslice__", mutable_seq_not_impl_setslice, METH_VAR_ARGS },
    { "append", mutable_seq_not_impl_append, METH_ONE_ARG },
    { "extend", mutable_seq_not_impl_extend, METH_ONE_ARG },
    { "insert", mutable_seq_not_impl_insert, METH_VAR_ARGS },
    { "remove", mutable_seq_not_impl_remove, METH_ONE_ARG },
    { "pop", mutable_seq_not_impl_pop, METH_ONE_ARG },
    { "clear", mutable_seq_not_impl_clear, METH_NO_ARGS },
    { NULL },
};

static BaseDef MutableSequence_bases[] = {
    { &Sequence_type },
    { NULL },
};

// clang-format off
TypeObject MutableSequence_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name     = "MutableSequence",
    .flags    = TP_FLAGS_TRAIT,
    .methdefs = MutableSequence_methods,
    .basedefs = MutableSequence_bases,
};
// clang-format on

#ifdef __cplusplus
}
#endif
