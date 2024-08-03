/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "exception.h"

#ifdef __cplusplus
extern "C" {
#endif

TypeObject exc_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "Exception",
};

static Object *_new_exc(char *msg)
{
    Exception *exc = gc_alloc_obj(exc);
    INIT_OBJECT_HEAD(exc, &exc_type);
    exc->msg = msg;
    return (Object *)exc;
}

void _raise_exc(KoalaState *ks, const char *fmt, ...)
{
    char *msg = "";
    ks->exc = _new_exc(msg);
}

void _print_exc(KoalaState *ks)
{
    Object *obj = ks->exc;
    if (!obj) return;
    Exception *exc = (Exception *)obj;
    printf("%s\n", exc->msg);
}

#ifdef __cplusplus
}
#endif
