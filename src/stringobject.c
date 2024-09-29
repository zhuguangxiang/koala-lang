/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "stringobject.h"
#include "shadowstack.h"

#ifdef __cplusplus
extern "C" {
#endif

static void str_gc_mark(StrObject *obj, Queue *que)
{
    if (obj->array) gc_mark_obj((GcObject *)obj->array, que);
}

TypeObject str_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "str",
    .flags = TP_FLAGS_CLASS | TP_FLAGS_PUBLIC | TP_FLAGS_FINAL,
    .size = sizeof(StrObject),
    .mark = (GcMarkFunc)str_gc_mark,
};

Object *kl_new_nstr(const char *s, int len)
{
    init_gc_stack(1);

    StrObject *sobj = gc_alloc_obj(sobj);
    INIT_OBJECT_HEAD(sobj, &str_type);
    sobj->start = 0;
    sobj->stop = len;

    gc_stack_push(sobj);

    GcArrayObject *arr = gc_alloc_array(GC_KIND_ARRAY_INT8, len + 1);
    char *data = (char *)(arr + 1);
    memcpy(data, s, len);
    data[len] = '\0';
    sobj->array = arr;

    fini_gc_stack();

    return (Object *)sobj;
}

Object *kl_new_fmt_str(const char *fmt, ...)
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
