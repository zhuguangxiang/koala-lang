/*===-- stringobject.c - String Object ----------------------------*- C -*-===*\
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) 2020 James, https://github.com/zhuguangxiang                 *|
|*                                                                            *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* This file implements the Koala `String` object.                            *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "stringobject.h"

#ifdef __cplusplus
extern "C" {
#endif

static TypeObject *string_type;

Object *string_new(const char *s)
{
    int len = strlen(s);
    int size = sizeof(void **) + sizeof(StringObject);
    size += len + 1;
    void **obj = gc_alloc(size);
    *obj = string_type;
    StringObject *sobj = (StringObject *)(obj + 1);
    sobj->len = len;
    memcpy(sobj->s, s, len);
    return (Object *)sobj;
}

// DLLEXPORT int length(StringObject *sobj) __attribute__ ((alias
// ("kl_string_length")));

DLLEXPORT int kl_string_length(StringObject *sobj)
{
    return sobj->len;
}

void init_string_type(void)
{
    /* public final class */
    string_type = type_new_class("String");
    type_set_public_final(string_type);

    /* `String` methods */
    MethodDef defs[] = {
        { "length", NULL, "i", "kl_string_length" },
        { NULL },
    };
    type_add_methdefs(string_type, defs);

    /* string_type ready */
    type_ready(string_type);

    /* show string_type */
    type_show(string_type);
}

#ifdef __cplusplus
}
#endif
