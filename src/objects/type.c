/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "function.h"
#include "log.h"
#include "module.h"

#ifdef __cplusplus
extern "C" {
#endif

static TypeObject *_value_typeof(int tag)
{
    static TypeObject *_types_mapping[] = {
        NULL,
    };

    ASSERT(tag < COUNT_OF(_types_mapping));
    return _types_mapping[tag];
}

/*
Get the type of a value. For reference values, return the type of the referenced object.
For primitive values, return the corresponding type based on the tag.
*/
TypeObject *kl_typeof(TValue *val)
{
    if (is_ref(val)) {
        Object *obj = to_ref(val);
        return OB_TYPE(obj);
    }
    return _value_typeof(val->tag);
}

/*---------------------------------------------------------------------------+
 |  Type(meta) type definition                                               |
 +---------------------------------------------------------------------------*/

static TValue type_str(TValue *self, TValue *args, int nargs)
{
    TypeObject *tp = kl_typeof(self);
    const char *s = kl_mo_path(tp->module);
    Object *ret;
    if (!strcmp(s, "std/builtin")) {
        ret = kl_new_fmt_str("<class '%s'>", tp->name);
    } else {
        ret = kl_new_fmt_str("<class '%s.%s'>", s, tp->name);
    }
    return obj_value(ret);
}

static MethodDef type_methods[] = {
    { "__str__", type_str },
    { NULL },
};

TypeObject type_type = {
    ._type = &type_type,
    .name = "type",
    .flags = TP_FLAGS_CLASS,
    .methdefs = type_methods,
};

/*---------------------------------------------------------------------------+
 |  type initialization core implementation                                  |
 +---------------------------------------------------------------------------*/

int kl_init_type(TypeObject *tp)
{
    Object *_m = tp->module;

    // initialization some fields
    vector_init_ptr(&tp->fields);
    vector_init_ptr(&tp->methods);
    stbl_init(&tp->members);

    // add method to type
    MethodDef *def = tp->methdefs;
    while (def && def->name) {
        Object *cfunc = kl_new_cfunc(def->name, def->cfunc, (Object *)tp);
        vector_push_back(&tp->methods, &cfunc);
        stbl_add_obj(&tp->members, def->name, cfunc);
        kl_bind_func(_m, cfunc);
        log_info("added method '%s' to class/trait '%s'", def->name, tp->name);
        ++def;
    }

    return 0;
}

#ifdef __cplusplus
}
#endif
