/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "stringobject.h"
#include "gc.h"
#include "shadowstack.h"

#ifdef __cplusplus
extern "C" {
#endif

static void str_gc_mark(StringObject *obj, Queue *que)
{
    if (obj->array) gc_mark_array(obj->array, que);
}

TypeObject str_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "str",
    .flags = TP_FLAGS_CLASS,
    .mark = (MarkFunc)str_gc_mark,
};

Object *kl_new_nstr(char *s, size_t len)
{
    StringObject *x = gc_alloc_obj(x);
    INIT_OBJECT_HEAD(x, &str_type);
    x->size = len;

    kl_gc_protect(x);

    char *data = gc_alloc_byte_array(len + 1);
    memcpy(data, s, len);
    data[len] = '\0';
    x->array = data;

    return (Object *)x;
}

Object *kl_new_fmt_str(char *fmt, ...)
{
    char buf[256];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, 255, fmt, args);
    va_end(args);
    buf[len] = '\0';
    return kl_new_nstr(buf, len);
}

#ifdef __cplusplus
}
#endif
