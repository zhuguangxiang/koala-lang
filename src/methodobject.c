/*===-- methodobject.c - Method Object ----------------------------*- C -*-===*\
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) 2020 James, https://github.com/zhuguangxiang                 *|
|*                                                                            *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* This file implements the Koala `Method` object.                            *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "methodobject.h"
#include <dlfcn.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline void *__get_cfunc(const char *name)
{
    return dlsym(NULL, name);
}

static TypeObject *method_type;

Object *cmethod_new(MethodDef *def)
{
    MethodObject *mobj = alloc_metaobject(MethodObject);
    mobj->name = def->name;
    mobj->kind = CFUNC_KIND;
    mobj->ptr = __get_cfunc(def->funcname);
    assert(mobj->ptr);
    return (Object *)mobj;
}

Object *method_new(char *name, Object *code)
{
    return NULL;
}

#define method_check(obj) (obj_get_type(obj) == method_type)

void init_method_type(void)
{
    /* `Method` is public final class */
    method_type = type_new_class("Method");
    type_set_public_final(method_type);

    /* method_type ready */
    type_ready(method_type);

    /* show method_type */
    type_show(method_type);
}

#ifdef __cplusplus
}
#endif
