/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "bytesobj.h"
#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

static TValue str_str(TValue *self, TValue *args, int nargs) { return *self; }

static TValue str_to_bytes(TValue *self, TValue *args, int nargs)
{
    Object *obj = to_obj(self);
    ASSERT(obj && IS_STR(obj));

    StringObject *str = (StringObject *)obj;
    ASSERT(nargs == 0);

    Object *bs = kl_new_bytes((uint32_t)str->size);
    memcpy(((BytesObject *)bs)->data, str->array, str->size);
    return obj_value(bs);
}

static MethodDef str_methods[] = {
    { "__str__", str_str },
    { "to_bytes", str_to_bytes },
    { NULL },
};

TypeObject str_type = {
    ._type = &type_type,
    .name = "str",
    .flags = TP_FLAGS_CLASS | TP_FLAGS_PUBLIC,
    .methdefs = str_methods,
};

static StringObject empty_str = {
    ._type = &str_type,
    .size = 0,
    .array = "",
};

Object *kl_new_nstr(char *s, size_t len)
{
    if (len == 0) {
        return (Object *)&empty_str;
    }

    StringObject *x = mm_alloc_obj(x);
    INIT_OBJECT_HEAD(x, &str_type);
    x->size = len;

    char *data = mm_alloc(len + 1);
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

void kl_free_str(Object *obj)
{
    StringObject *sobj = (StringObject *)obj;
    if (sobj->size > 0) {
        mm_free(sobj->array);
    }
    mm_free(sobj);
}

#ifdef __cplusplus
}
#endif
