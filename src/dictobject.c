/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "dictobject.h"

#ifdef __cplusplus
extern "C" {
#endif

/* dict(iterable Iterable = None) */
static Value _dict_call(Object *self, Value *args, int nargs, Object *kwds)
{
    return NoneValue;
}

TypeObject dict_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "dict",
    .call = NULL,
};

Object *kl_new_dict(void) { return NULL; }

#ifdef __cplusplus
}
#endif
