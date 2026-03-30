/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "exception.h"
#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

static Value number_not_impl_add(Value *self, Value *rhs)
{
    raise_exc_str("'__add__()' in trait 'Number' is not implemented");
    return error_value;
}

static Value number_not_impl_sub(Value *self, Value *rhs)
{
    raise_exc_str("'__sub__()' in trait 'Number' is not implemented");
    return error_value;
}

static Value number_not_impl_mul(Value *self, Value *rhs)
{
    raise_exc_str("'__mul__()' in trait 'Number' is not implemented");
    return error_value;
}

static Value number_not_impl_div(Value *self, Value *rhs)
{
    raise_exc_str("'__div__()' in trait 'Number' is not implemented");
    return error_value;
}

static Value number_not_impl_mod(Value *self, Value *rhs)
{
    raise_exc_str("'__mod__()' in trait 'Number' is not implemented");
    return error_value;
}

static Value number_not_impl_lsh(Value *self, Value *rhs)
{
    raise_exc_str("'__lsh__()' in trait 'Number' is not implemented");
    return error_value;
}

static Value number_not_impl_rsh(Value *self, Value *rhs)
{
    raise_exc_str("'__rsh__()' in trait 'Number' is not implemented");
    return error_value;
}

static Value number_not_impl_and(Value *self, Value *rhs)
{
    raise_exc_str("'__and__()' in trait 'Number' is not implemented");
    return error_value;
}

static Value number_not_impl_xor(Value *self, Value *rhs)
{
    raise_exc_str("'__xor__()' in trait 'Number' is not implemented");
    return error_value;
}

static Value number_not_impl_or(Value *self, Value *rhs)
{
    raise_exc_str("'__or__()' in trait 'Number' is not implemented");
    return error_value;
}

static MethodDef Number_methods[] = {
    { "__add__", number_not_impl_add, METH_ONE_ARG },
    { "__sub__", number_not_impl_sub, METH_ONE_ARG },
    { "__mul__", number_not_impl_mul, METH_ONE_ARG },
    { "__div__", number_not_impl_div, METH_ONE_ARG },
    { "__mod__", number_not_impl_mod, METH_ONE_ARG },
    { "__lsh__", number_not_impl_lsh, METH_ONE_ARG },
    { "__rsh__", number_not_impl_rsh, METH_ONE_ARG },
    { "__and__", number_not_impl_and, METH_ONE_ARG },
    { "__xor__", number_not_impl_xor, METH_ONE_ARG },
    { "__or__", number_not_impl_or, METH_ONE_ARG },
    { NULL },
};

TypeObject Number_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "Number",
    .flags = TP_FLAGS_TRAIT,
    .methdefs = Number_methods,
};

#ifdef __cplusplus
}
#endif
