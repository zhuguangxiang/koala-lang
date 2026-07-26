/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "atom.h"
#include "modobj.h"

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------+
 |  Koala Code type definition                                               |
 +---------------------------------------------------------------------------*/

static TValue code_str(TValue *self, TValue *args, int nargs)
{
    Object *obj = to_obj(self);
    ASSERT(IS_CODE(obj));
    CodeObject *co = (CodeObject *)obj;
    Object *owner = co->owner;

    Object *r;
    if (IS_MODULE(owner)) {
        ModuleObject *m = (ModuleObject *)owner;
        r = kl_new_fmt_str("<function(code) '%s' in module '%s'>", co->cs.name, m->path);
    } else {
        TypeObject *tp = (TypeObject *)owner;
        r = kl_new_fmt_str("<method(code) '%s' of class '%s'>", co->cs.name, tp->name);
    }
    ASSERT(r);

    return obj_value(r);
}

static MethodDef code_methods[] = {
    { "__str__", code_str },
    { NULL },
};

TypeObject code_type = {
    ._type = &type_type,
    .name = "code",
    .flags = TP_FLAGS_CLASS,
    .methdefs = code_methods,
    .call = kl_eval_code,
};

Object *kl_new_code(char *name, Object *owner)
{
    CodeObject *code = mm_alloc_obj(code);
    INIT_OBJECT_HEAD(code, &code_type);
    code->cs.name = atom(name);
    code->owner = owner;
    return (Object *)code;
}

/*---------------------------------------------------------------------------+
 |  C-Func type definition                                                   |
 +---------------------------------------------------------------------------*/

static TValue cfunc_str(TValue *self, TValue *args, int nargs)
{
    Object *obj = to_obj(self);
    ASSERT(IS_CFUNC(obj));
    CFuncObject *fo = (CFuncObject *)obj;
    Object *owner = fo->owner;

    Object *r;
    if (IS_MODULE(owner)) {
        ModuleObject *m = (ModuleObject *)owner;
        r = kl_new_fmt_str("<function(native) '%s' in module '%s'>", fo->name, m->path);
    } else {
        TypeObject *tp = (TypeObject *)owner;
        r = kl_new_fmt_str("<method(native) '%s' of class '%s'>", fo->name, tp->name);
    }
    ASSERT(r);

    return obj_value(r);
}

static MethodDef cfunc_methods[] = {
    { "__str__", cfunc_str },
    { NULL },
};

static TValue cfunc_call(TValue *self, TValue *args, int nargs)
{
    Object *obj = to_obj(self);
    ASSERT(IS_CFUNC(obj));
    CFuncObject *cfunc = (CFuncObject *)obj;
    Object *owner = cfunc->owner;
    if (IS_MODULE(owner)) {
        return cfunc->func(self, args, nargs);
    }

    ASSERT(IS_TYPE(owner, &type_type));
    ASSERT(nargs >= 1);
    return cfunc->func(args, args + 1, nargs - 1);
}

TypeObject cfunc_type = {
    ._type = &type_type,
    .name = "cfunc",
    .flags = TP_FLAGS_CLASS | TP_FLAGS_PUBLIC,
    .methdefs = cfunc_methods,
    .call = cfunc_call,
};

Object *kl_new_cfunc(char *name, NativeFunc fn, Object *owner)
{
    CFuncObject *cfunc = mm_alloc_obj(cfunc);
    INIT_OBJECT_HEAD(cfunc, &cfunc_type);
    cfunc->name = atom(name);
    cfunc->func = fn;
    cfunc->owner = owner;
    return (Object *)cfunc;
}

#ifdef __cplusplus
}
#endif
