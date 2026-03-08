/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "exception.h"
#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------Iterable trait------------------------------*/

static Value iterable_not_impl_iter(Value *self)
{
    raise_exc_str("'__iter__()' in trait 'Iterable' is not implemented");
    return error_value;
}

static MethodDef Iterable_methods[] = {
    { "__iter__", iterable_not_impl_iter, METH_NO_ARGS },
    { NULL },
};

TypeObject Iterable_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "Iterable",
    .flags = TP_FLAGS_TRAIT,
    .methdefs = Iterable_methods,
};

/*------------------------------Iterator trait------------------------------*/

static Value iterator_not_impl_has_next(Value *self)
{
    raise_exc_str("'__has_next__()' in trait 'Iterator' is not implemented");
    return error_value;
}

static Value iterator_not_impl_next(Value *self)
{
    raise_exc_str("'__next__()' in trait 'Iterator' is not implemented");
    return error_value;
}

static MethodDef Iterator_methods[] = {
    { "__has_next__", iterator_not_impl_has_next, METH_NO_ARGS },
    { "__next__", iterator_not_impl_next, METH_NO_ARGS },
    { NULL },
};

// clang-format off
TypeObject Iterator_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "Iterator",
    .flags = TP_FLAGS_TRAIT,
    .methdefs = Iterator_methods,
};
// clang-format on

#ifdef __cplusplus
}
#endif
