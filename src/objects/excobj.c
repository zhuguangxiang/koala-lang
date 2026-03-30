/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "excobj.h"

#ifdef __cplusplus
extern "C" {
#endif

TypeObject exc_type = {
    ._type = &type_type,
    .name = "Exception",
    .flags = TP_FLAGS_CLASS,
};

Object *kl_new_exc(char *msg)
{
    Exception *exc = mm_alloc_obj(exc);
    INIT_OBJECT_HEAD(exc, &exc_type);
    exc->msg = strdup(msg);
    exc->back = NULL;
    return (Object *)exc;
}

#ifdef __cplusplus
}
#endif
