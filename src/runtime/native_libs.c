/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "modobj.h"

#ifdef __cplusplus
extern "C" {
#endif

static TValue _default___str__(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    TypeObject *tp = kl_typeof(self);
    ASSERT(tp);
    unsigned int hash = kl_hash(self);
    ModuleObject *m = (ModuleObject *)tp->module;

    Object *s = kl_new_fmt_str("<%s.%s object at 0x%x>", m->path, tp->name, hash);
    return obj_value(s);
}

static TValue _default___hash__(TValue *self, TValue *args, int nargs)
{
    unsigned int hash = mem_hash(self, sizeof(TValue));
    return int64_value(hash);
}

static TValue _default___eq__(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    int r = memcmp(self, args, sizeof(TValue));
    return bool_value(r == 0);
}

static TValue _default___ne__(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    int r = memcmp(self, args, sizeof(TValue));
    return bool_value(r != 0);
}

Object *kl_get_native(Object *m, char *name)
{
    ModuleObject *mo = (ModuleObject *)m;
    NativeLib *lib;
    vector_foreach_ptr(lib, &mo->libs) {
        Object *ob = stbl_find_obj(&lib->symbols, name);
        if (ob) return ob;
    }

    if (match_suffix(name, "__str__")) {
        Object *ob = kl_new_cfunc("__str__", _default___str__, NULL);
        return ob;
    } else if (match_suffix(name, "hash")) {
        Object *ob = kl_new_cfunc("hash", _default___hash__, NULL);
        return ob;
    } else if (match_suffix(name, "__eq__")) {
        Object *ob = kl_new_cfunc("__eq__", _default___eq__, NULL);
        return ob;
    } else if (match_suffix(name, "__ne__")) {
        Object *ob = kl_new_cfunc("__ne__", _default___ne__, NULL);
        return ob;
    }

    return NULL;
}

int kl_reg_func(NativeLib *lib, char *name, NativeFunc fn)
{
    Object *obj = kl_new_cfunc(name, fn, NULL);
    stbl_add_obj(&lib->symbols, name, obj);
    return 0;
}

int kl_reg_meth(NativeLib *lib, char *cls, char *meth, NativeFunc fn)
{
    char full_name[256];
    snprintf(full_name, sizeof(full_name), "%s$%s", cls, meth);
    Object *obj = kl_new_cfunc(full_name, fn, NULL);
    stbl_add_obj(&lib->symbols, full_name, obj);
    return 0;
}

int kl_reg_type(NativeLib *lib, TypeObject *tp)
{
    kl_init_type(tp);

    MethodDef *def = tp->methdefs;
    while (def && def->name) {
        if (def->cfunc) {
            kl_reg_meth(lib, tp->name, def->name, def->cfunc);
        }
        ++def;
    }

    stbl_add_obj(&lib->symbols, tp->name, (Object *)tp);

    return 0;
}

#ifdef __cplusplus
}
#endif
