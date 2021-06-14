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

typedef struct _StringObj StringObj;

struct _StringObj {
    VirtTable *vtbl;
    char *gcstr;
};

static TypeInfo string_type = {
    .name = "string",
    .flags = TF_CLASS | TF_FINAL,
};

static int string_objmap[] = {
    1,
    offsetof(StringObj, gcstr),
};

uintptr string_class(uintptr self)
{
    return 0;
}

uintptr string_tostr(uintptr self)
{
    return self;
}

void init_string_type(void)
{
    MethodDef string_methods[] = {
        /* clang-format off */
        METHOD_DEF("__class__", nil, "LClass;", string_class),
        METHOD_DEF("__str__",   nil, "s", string_tostr),
        /* clang-format on */
    };
    type_add_methdefs(&string_type, string_methods);
    type_ready(&string_type);
    type_show(&string_type);
    assert(string_type.num_vtbl == 1);
    pkg_add_type("/", &string_type);
}

uintptr string_new(char *s)
{
    int len = strlen(s);
    StringObj *obj = gc_alloc(sizeof(*obj), string_objmap);

    GC_STACK(1);
    gc_push(&obj, 0);

    void *gcstr = gc_alloc_string(len + 1);
    memcpy(gcstr, s, len);
    obj->vtbl = string_type.vtbl[0];
    obj->gcstr = gcstr;

    gc_pop();

    return (uintptr)obj;
}

void string_show(uintptr self)
{
    StringObj *obj = (StringObj *)self;
    printf(">>> %s\n", obj->gcstr);
}

#ifdef __cplusplus
}
#endif
