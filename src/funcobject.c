/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "funcobject.h"
#include "run.h"

#ifdef __cplusplus
extern "C" {
#endif

static Value _func_call(Object *self, Value *args, int nargs, Object *kwargs)
{
    Value ret = kl_eval_func(self, args, nargs, kwargs);
    return ret;
}

TypeObject func_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "func",
    .call = _func_call,
};

Object *kl_new_func(Object *code, Object *mod, TypeObject *cls)
{
    FuncObject *func = gc_alloc_obj_p(func);
    INIT_OBJECT_HEAD(func, &func_type);
    func->code = code;
    func->mod = mod;
    func->cls = cls;
    return (Object *)func;
}

#ifdef __cplusplus
}
#endif
