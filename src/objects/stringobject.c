/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangxiang https://github.com/zhuguangxiang               *|
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
    int size = sizeof(void *) + sizeof(StringObject);
    size += len + 1;
    void **gcobj = gc_alloc(size);
    *obj = string_type;
    StringObject *sobj = (StringObject *)(obj + 1);
    sobj->nbytes = len;
    memcpy(sobj->s, s, len);
    return (Object *)sobj;
}

#define string_check(obj) (obj_get_type(obj) == string_type)

const char *string_tocstr(Object *obj)
{
    assert(string_check(obj));
    StringObject *sobj = (StringObject *)obj;
    return sobj->s;
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
