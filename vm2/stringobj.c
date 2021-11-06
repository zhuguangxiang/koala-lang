/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "core.h"
#include "gc/gc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* final class string {} */
typedef struct _StringObj StringObj;

struct _StringObj {
    VTable *vtbl;
    int len;
    char data[0];
};

static TypeInfo string_type = {
    .name = "string",
    .flags = TF_CLASS | TF_FINAL,
};

objref string_tostr(objref self)
{
    return self;
}

void init_string_type(void)
{
    MethodDef string_methods[] = {
        /* clang-format off */
        METHOD_DEF("__str__",   null, "s", string_tostr),
        /* clang-format on */
    };
    type_add_methdefs(&string_type, string_methods);
    type_ready(&string_type);
    type_show(&string_type);
    pkg_add_type(root_pkg, &string_type);
}

objref string_new(char *s)
{
    int len = strlen(s);
    StringObj *obj = gc_alloc(sizeof(*obj) + len + 1, null);
    obj->vtbl = string_type.vtbl[0];
    obj->len = len;
    memcpy(obj->data, s, len);
    return (objref)obj;
}

void string_show(objref self)
{
    StringObj *obj = (StringObj *)self;
    printf(">>> %s\n", obj->data);
}

#ifdef __cplusplus
}
#endif
