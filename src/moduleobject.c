/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "moduleobject.h"
#include "cfuncobject.h"

#ifdef __cplusplus
extern "C" {
#endif

TypeObject module_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "module",
};

Object *kl_new_module(const char *name)
{
    ModuleObject *m = gc_alloc_obj_p(m);
    INIT_OBJECT_HEAD(m, &module_type);
    vector_init_ptr(&m->symbols);
    vector_init(&m->values, sizeof(Value));
    vector_init(&m->relocs, sizeof(RelocInfo));
    RelocInfo self = { .name = name, .module = m };
    vector_push_back(&m->relocs, &self);
    return (Object *)m;
}

int module_add_cfunc(Object *_m, MethodDef *def)
{
    ModuleObject *m = (ModuleObject *)_m;
    Object *cfunc = kl_new_cfunc(def, (Object *)m, NULL);
    ASSERT(cfunc);
    vector_push_back(&m->symbols, &cfunc);
    return 0;
}

int module_add_code(Object *_m, Object *code)
{
    ModuleObject *m = (ModuleObject *)_m;
    vector_push_back(&m->symbols, &code);
    return 0;
}

int module_add_type(Object *_m, TypeObject *type)
{
    ModuleObject *m = (ModuleObject *)_m;
    vector_push_back(&m->symbols, &type);
    return 0;
}

#ifdef __cplusplus
}
#endif
