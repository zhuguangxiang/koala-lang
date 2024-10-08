/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "cfuncobject.h"
#include "exception.h"
#include "moduleobject.h"
#include "stringobject.h"

#ifdef __cplusplus
extern "C" {
#endif

static Value cfunc_call(Value *self, Value *args, int nargs, Object *names)
{
    Object *callable = as_obj(self);
    ASSERT(IS_CFUNC(callable));
    CFuncObject *cfunc = (CFuncObject *)callable;
    void *fn = cfunc->def->cfunc;
    int flags = cfunc->def->flags;
    ASSERT(cfunc->module || cfunc->cls);

    Value r = none_value;

    if (flags & METH_NO_ARGS) {
        r = ((CFuncNoArgs)fn)(args);
    } else if (flags & METH_ONE_ARG) {
        if (cfunc->cls) {
            r = ((CFuncOneArg)fn)(args, args + 1);
        } else {
            Value self = obj_value(cfunc->module);
            r = ((CFuncOneArg)fn)(&self, args);
        }
    } else if (flags & METH_VAR_ARGS) {
        r = ((CFuncVarArgs)fn)(args, args + 1, nargs - 1);
    } else if (flags & METH_VAR_NAMES) {
        if (cfunc->cls) {
            r = ((CFuncVarArgsNames)fn)(args, args + 1, nargs - 1, names);
        } else {
            Value self = obj_value(cfunc->module);
            r = ((CFuncVarArgsNames)fn)(&self, args, nargs, names);
        }
    } else {
        ASSERT(0);
        raise_exc_fmt("'%s' is invalid function", cfunc->def->name);
        r = error_value;
    }

    return r;
}

static Value cfunc_str(Value *self)
{
    Object *callable = as_obj(self);
    ASSERT(IS_CFUNC(callable));
    CFuncObject *cfunc = (CFuncObject *)callable;
    Object *r;
    if (cfunc->cls) {
        TypeObject *tp = cfunc->cls;
        r = kl_new_fmt_str("<method '%s' of class '%s'>", cfunc->def->name, tp->name);
    } else {
        ModuleObject *m = (ModuleObject *)cfunc->module;
        r = kl_new_fmt_str("<function '%s' in module '%s'>", cfunc->def->name,
                           m->def->name);
    }
    ASSERT(r);
    return obj_value(r);
}

TypeObject cfunc_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "cfunc",
    .call = cfunc_call,
    .str = cfunc_str,
};

Object *kl_new_cfunc(MethodDef *def, Object *m, TypeObject *cls)
{
    CFuncObject *cfunc = gc_alloc_obj_p(cfunc);
    INIT_OBJECT_HEAD(cfunc, &cfunc_type);
    cfunc->def = def;
    cfunc->module = m;
    cfunc->cls = cls;
    return (Object *)cfunc;
}

#ifdef __cplusplus
}
#endif
