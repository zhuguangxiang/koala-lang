/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.call
 */

#include "cfuncobject.h"
#include "hashmap.h"
#include "moduleobject.h"
#include "stringobject.h"

#ifdef __cplusplus
extern "C" {
#endif

static int _type_ready(TypeObject *tp, Object *m)
{
    tp->fields = vector_create_ptr();
    ASSERT(tp->fields);
    tp->funcs = vector_create_ptr();
    ASSERT(tp->funcs);
    init_symbol_table(&tp->map);

    // add method to type
    MethodDef *def = tp->methods;
    while (def && def->name) {
        Object *cfunc = kl_new_cfunc(def, tp->module, tp);
        table_add_object(&tp->map, def->name, cfunc);
        ++def;
    }

    TypeObject *base;

    if (!tp->base) {
        base = &base_type;
        tp->base = base;
    }

    if (base != &base_type) {
        if (!(base->flags & TP_FLAGS_READY)) {
            if (type_ready(base, m)) return -1;
        }
    }

    // inherit from base
    Object **item;
    vector_foreach(item, base->funcs) {
        // if (IS_CFUNC(*item)) {
        //     CFuncObject *cfunc = (CFuncObject *)(*item);
        //     Object *r = vtbl_find(map, cfunc->def->name);
        //     if (!r) {
        //         // not override, inherit it
        //         type_add_code(tp, (Object *)cfunc);
        //     } else {
        //         // override, add current function
        //         type_add_code(tp, r);
        //     }
        // }
    }

    // check symbol table for traits
    TraitObject **traits = tp->traits;
    TraitObject *trait;
    while (traits && (trait = *traits)) {
        void **item;
        vector_foreach(item, trait->intfs) {
            // TODO:
        }
    }
}

int type_ready(TypeObject *tp, Object *module)
{
    // The type is fully initialized.
    if (tp->flags & TP_FLAGS_READY) return 0;

    // The type is initializing, to prevent recursive ready calls
    if (tp->flags & TP_FLAGS_READYING) return 0;

    tp->flags |= TP_FLAGS_READYING;

    // set module
    tp->module = module;

    if (_type_ready(tp, module)) {
        tp->flags &= ~TP_FLAGS_READYING;
        return -1;
    }

    module_add_object(module, tp->name, (Object *)tp);

    // mark the type is ready
    tp->flags |= TP_FLAGS_READY;
    tp->flags &= ~TP_FLAGS_READYING;
    return 0;
}

#ifdef __cplusplus
}
#endif
