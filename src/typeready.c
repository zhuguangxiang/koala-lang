/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.call
 */

#include "cfuncobject.h"
#include "hashmap.h"
#include "stringobject.h"

#ifdef __cplusplus
extern "C" {
#endif

static int _vtbl_func_equal_(void *e1, void *e2)
{
    KeywordEntry *n1 = e1;
    KeywordEntry *n2 = e2;
    if (!strcmp(n1->key, n2->key)) return 1;
    return 0;
}

static void vtbl_add_cfunc(HashMap *map, Object *cfunc, const char *name)
{
    KeywordEntry *e = mm_alloc_obj_fast(e);
    unsigned int hash = str_hash(name);
    hashmap_entry_init(e, hash);
    e->key = name;
    e->obj = cfunc;
    int r = hashmap_put_absent(map, e);
    ASSERT(!r);
}

static void *vtbl_find(HashMap *map, const char *name)
{
    KeywordEntry entry;
    unsigned int hash = str_hash(name);
    hashmap_entry_init(&entry, hash);
    KeywordEntry *found = hashmap_get(map, &entry);
    return found ? found->obj : NULL;
}

void type_add_code(TypeObject *tp, Object *code)
{
    Vector *funcs = tp->funcs;
    if (!funcs) {
        funcs = vector_create_ptr();
        tp->funcs = funcs;
    }
    vector_push_back(funcs, &code);
}

static int _type_ready(TypeObject *tp, Object *m)
{
    HashMap *map = mm_alloc_obj_fast(map);
    hashmap_init(map, _vtbl_func_equal_);

    // add method to type
    MethodDef *def = tp->methods;
    while (def && def->name) {
        Object *cfunc = kl_new_cfunc(def, tp->module, tp);
        vtbl_add_cfunc(map, cfunc, def->name);
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

    // inherit base methods
    FuncObject **item;
    vector_foreach(item, base->funcs) {
        if (IS_CFUNC(*item)) {
            CFuncObject *cfunc = (CFuncObject *)(*item);
            Object *r = vtbl_find(map, cfunc->def->name);
            if (!r) {
                // not override, inherit it
                type_add_code(tp, (Object *)cfunc);
            } else {
                // override, add current function
                type_add_code(tp, r);
            }
        }
    }

    // update vtbl
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

    // mark the type is ready
    tp->flags |= TP_FLAGS_READY;
    tp->flags &= ~TP_FLAGS_READYING;
    return 0;
}

#ifdef __cplusplus
}
#endif
