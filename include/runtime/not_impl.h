/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_NOT_IMPL_H_
#define _KOALA_NOT_IMPL_H_

#include <string.h>
#include "excobj.h"
#include "modobj.h"

#ifdef __cplusplus
extern "C" {
#endif

static TValue kl_not_impl_func(TValue *self, TValue *args, int nargs)
{
    Object *obj = to_obj(self);
    ASSERT(IS_CFUNC(obj));

    CFuncObject *cfunc = (CFuncObject *)obj;
    Object *owner = cfunc->owner;

    if (cfunc->priv) {
        raise_exc_str((char *)cfunc->priv);
        return error_value;
    }

    if (IS_MODULE(owner)) {
        ModuleObject *m = (ModuleObject *)owner;
        raise_exc_fmt("func '%s' in '%s' is not implemented!", cfunc->name, m->path);
        return error_value;
    }

    ASSERT(IS_TYPE(owner, &type_type));
    TypeObject *tp = (TypeObject *)owner;
    char *dollar = strchr(cfunc->name, '$') + 1;
    raise_exc_fmt("func '%s::%s' is not implemented!", tp->name, dollar);
    return error_value;
}

static Object *new_not_impl_func(Object *m, char *func_name)
{
    Object *cfunc = kl_new_cfunc(func_name, kl_not_impl_func, m);
    ((CFuncObject *)cfunc)->not_impl = 1;
    return cfunc;
}

static Object *new_not_impl_trait_func(char *kls_name, char *trait_name, char *fn_name, Object *m)
{
    BUF(name);
    BUF(msg);

    buf_write_fmt(&name, "%s_%s_%s", trait_name, kls_name, fn_name);
    buf_write_fmt(&msg, "func '%s::%s' for '%s' is not implemented!", trait_name, fn_name,
                  kls_name);

    Object *cfunc = kl_new_cfunc(BUF_STR(name), kl_not_impl_func, m);

    char *_msg = mm_alloc(BUF_LEN(msg) + 1);
    memcpy(_msg, BUF_STR(msg), BUF_LEN(msg));
    _msg[BUF_LEN(msg)] = '\0';

    ((CFuncObject *)cfunc)->not_impl = 1;
    ((CFuncObject *)cfunc)->priv = _msg;

    FINI_BUF(name);
    FINI_BUF(msg);
    return cfunc;
}

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_NOT_IMPL_H_ */
