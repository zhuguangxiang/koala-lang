/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "strobject.h"

#ifdef __cplusplus
extern "C" {
#endif

TypeObject str_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "str",
};

Object *kl_new_nstr(const char *s, int len)
{
    StrObject *sobj = gc_alloc_obj(sobj);
    INIT_OBJECT_HEAD(sobj, &str_type);
    sobj->start = 0;
    sobj->end = len;
    sobj->array = gc_alloc(len);
    return (Object *)sobj;
}

#ifdef __cplusplus
}
#endif
