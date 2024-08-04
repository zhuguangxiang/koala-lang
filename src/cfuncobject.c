/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "cfuncobject.h"
#include "exception.h"

#ifdef __cplusplus
extern "C" {
#endif

static Value _cfunc_call(Object *obj, Value *args, int nargs)
{
    ASSERT(IS_CFUNC(obj));
    CFuncObject *cfunc = (CFuncObject *)obj;
    CFunc fn = cfunc->def->cfunc;
    ASSERT(cfunc->module || cfunc->cls);
    if (cfunc->module) {
        Value self = ObjectValue(cfunc->module);
        return fn(&self, args, nargs);
    } else if (cfunc->cls) {
        return fn(args, args + 1, nargs - 1);
    } else {
        raise_exc("'%s' is invalid cfunc", cfunc->def->name);
        return ErrorValue;
    }
}

TypeObject cfunc_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "cfunc",
    .call = _cfunc_call,
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
