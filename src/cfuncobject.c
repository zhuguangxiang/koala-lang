/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "cfuncobject.h"

#ifdef __cplusplus
extern "C" {
#endif

static Value _cfunc_call(Object *self, Value *args, int nargs, Object *kwargs)
{
    ASSERT(IS_CFUNC(self));
    CFuncObject *cfunc = (CFuncObject *)self;
    ASSERT(cfunc->call);
    Value ret = cfunc->call(self, args, nargs, kwargs);
    return ret;
}

TypeObject cfunc_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "cfunc",
    .call = _cfunc_call,
};

static Value _cfunc_call_noarg(Object *self, Value *args, int nargs,
                               Object *kwargs)
{
    CFuncObject *cfunc = (CFuncObject *)self;
    MethodDef *def = cfunc->def;
    CFunc fn = def->cfunc;
    return fn(self);
}

static Value _cfunc_call_one(Object *self, Value *args, int nargs,
                             Object *kwargs)
{
    CFuncObject *cfunc = (CFuncObject *)self;
    MethodDef *def = cfunc->def;
    CFuncOne fn = def->cfunc;
    return fn(self, args);
}

static Value _cfunc_call_fast(Object *self, Value *args, int nargs,
                              Object *kwargs)
{
    CFuncObject *cfunc = (CFuncObject *)self;
    MethodDef *def = cfunc->def;
    CFuncFast fn = def->cfunc;
    return fn(self, args, nargs);
}

static Value _cfunc_call_fast_keywords(Object *self, Value *args, int nargs,
                                       Object *kwargs)
{
    CFuncObject *cfunc = (CFuncObject *)self;
    MethodDef *def = cfunc->def;
    CFuncFastWithKeywords fn = def->cfunc;
    return fn(self, args, nargs, kwargs);
}

Object *kl_new_cfunc(MethodDef *def, Object *mod, TypeObject *cls)
{
    CFuncObject *cfunc = gc_alloc_obj_p(cfunc);
    INIT_OBJECT_HEAD(cfunc, &cfunc_type);
    cfunc->def = def;
    cfunc->mod = mod;
    cfunc->cls = cls;
    CallFunc call = NULL;
    int flags = def->flags;
    flags &= (METH_NOARG | METH_ONE | METH_FASTCALL | METH_KEYWORDS);
    switch (flags) {
        case METH_NOARG:
            call = _cfunc_call_noarg;
            break;
        case METH_ONE:
            call = _cfunc_call_one;
            break;
        case METH_FASTCALL:
            call = _cfunc_call_fast;
            break;
        case METH_FASTCALL | METH_KEYWORDS:
            call = _cfunc_call_fast_keywords;
            break;
        default:
            ASSERT(0);
            break;
    }
    cfunc->call = call;
    return (Object *)cfunc;
}

#ifdef __cplusplus
}
#endif
