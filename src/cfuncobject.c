/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "cfuncobject.h"

#ifdef __cplusplus
extern "C" {
#endif

static Value _cfunc_call(Value *self, Value *args, int nargs, Object *kwargs)
{
    ASSERT(IS_OBJECT(self));
    CFuncObject *obj = (CFuncObject *)self->obj;
    ASSERT(IS_CFUNC(obj));

    Value ret = NoneValue();

    switch (obj->def->flags) {
        case METH_NO_ARGS: {
            CFunc func = obj->def->cfunc;
            ASSERT(nargs == 0);
            ASSERT(args == NULL);
            ASSERT(kwargs == NULL);
            ret = func(self, NULL);
            break;
        }
        case METH_ONE: {
            CFunc func = obj->def->cfunc;
            ASSERT(nargs == 1);
            ASSERT(args != NULL);
            ASSERT(kwargs == NULL);
            ret = func(self, args);
            break;
        }
        case METH_FASTCALL: {
            CFuncFast func = obj->def->cfunc;
            ASSERT(nargs != 0);
            ASSERT(args != NULL);
            ASSERT(kwargs == NULL);
            ret = func(self, args, nargs);
            break;
        }
        case METH_FASTCALL | METH_KEYWORDS: {
            CFuncFastWithKeywords func = obj->def->cfunc;
            ASSERT(nargs != 0);
            ASSERT(args != NULL);
            ASSERT(kwargs != NULL);
            ret = func(self, args, nargs, kwargs);
            break;
        }
        default: {
            ASSERT(0);
            break;
        }
    }
    return ret;
}

TypeObject cfunc_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "cfunc",
    .call = _cfunc_call,
    .offset_kwargs = offsetof(CFuncObject, kwargs),
};

Object *kl_new_cfunc(MethodDef *def)
{
    int msize = sizeof(CFuncObject);
    Object *obj = gc_alloc(msize);
    INIT_OBJECT_HEAD(obj, &cfunc_type);
    ((CFuncObject *)(obj))->def = def;
    return obj;
}

#ifdef __cplusplus
}
#endif
