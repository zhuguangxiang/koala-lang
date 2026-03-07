/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "moduleobject.h"
#include "cfuncobject.h"
#include "run.h"
#include "stringobject.h"

#ifdef __cplusplus
extern "C" {
#endif

static void module_fini(ModuleObject *m)
{
    ASSERT(IS_MODULE(m));
    vector_fini(&m->symbols);
    vector_fini(&m->rels);
}

TypeObject module_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "module",
    .fini = (FiniFunc)module_fini,
};

Object *kl_new_module(char *path)
{
    ModuleObject *m = mm_alloc_obj(m);
    INIT_OBJECT_HEAD(m, &module_type);

    vector_init_ptr(&m->symbols);
    init_sym_tbl(&m->map);
    vector_init(&m->consts, sizeof(Value));
    vector_init(&m->rels, sizeof(RelocInfo));

    RelocInfo not_used = { NULL, NULL };
    vector_push_back(&m->rels, &not_used);

    sym_tbl_add(&_gs_modules, path, strlen(path), (Object *)m);

    return (Object *)m;
}

Object *kl_module_from_moddef(ModuleDef *def)
{
    Object *obj = kl_new_module(def->name);
    ModuleObject *m = (ModuleObject *)obj;

    if (def->size) {
        void *state = mm_alloc(def->size);
        m->state = state;
    }

    if (def->init) def->init(obj);

    MethodDef *meth = def->methods;
    while (meth && meth->name) {
        module_add_cfunc(obj, meth);
        ++meth;
    }

    m->def = def;

    return obj;
}

// int module_add_member(Object *_m, MemberDef *member)
// {
//     ModuleObject *m = (ModuleObject *)_m;
//     Object *obj = kl_field_from_member(member);
//     ASSERT(obj);
//     module_add_obj(_m, member->name, obj);
//     return 0;
// }

// int module_add_getset(Object *_m, GetSetDef *getset) {}

int module_add_cfunc(Object *_m, MethodDef *def)
{
    ModuleObject *m = (ModuleObject *)_m;
    Object *obj = kl_new_cfunc(def, (Object *)m, NULL);
    ASSERT(obj);
    module_add_obj(_m, def->name, obj);
    return 0;
}

int module_add_obj(Object *_m, char *name, Object *obj)
{
    ModuleObject *m = (ModuleObject *)_m;
    vector_push_back(&m->symbols, &obj);
    sym_tbl_add(&m->map, name, strlen(name), obj);
    return 0;
}

Object *module_lookup(Object *_m, char *name, int len)
{
    ModuleObject *m = (ModuleObject *)_m;
    Object *obj = sym_tbl_find(&m->map, name, len);
    return obj;
}

int cp_add_int(Object *_m, int64_t val)
{
    ModuleObject *m = (ModuleObject *)_m;
    Value v = int64_value(val);
    vector_push_back(&m->consts, &v);
    return 0;
}

int cp_add_str(Object *_m, char *s)
{
    Object *sobj = kl_new_str(s);
    ASSERT(sobj);
    cp_add_obj(_m, sobj);
    return 0;
}

int cp_add_obj(Object *_m, Object *obj)
{
    ModuleObject *m = (ModuleObject *)_m;
    Value v = obj_value(obj);
    vector_push_back(&m->consts, &v);
    return 0;
}

int kl_do_link(Object *_m)
{
    ASSERT(IS_MODULE(_m));
    ModuleObject *m = (ModuleObject *)_m;

    // RelocInfo *rel;
    // vector_foreach_ptr(rel, &m->rels) {
    //     const char *ns = rel->ns;
    //     if (!ns) continue;
    //     int len = strlen(rel->ns);
    //     char *dot = strrchr(rel->ns, '.');
    //     if (dot) len = dot - ns;
    //     Object *obj = sym_tbl_find(&_gs_modules, ns, len);
    //     ASSERT(obj);

    //     if (dot) {
    //         // find from class, if dot exists.
    //         obj = module_lookup_object(obj, dot + 1, strlen(dot + 1));
    //         ASSERT(obj);
    //         SymbolInfo *sym;
    //         vector_foreach_ptr(sym, &rel->syms) {
    //             Object *o = type_lookup(obj, sym->name, strlen(sym->name));
    //             ASSERT(o);
    //             sym->obj = o;
    //         }
    //     } else {
    //         SymbolInfo *sym;
    //         vector_foreach_ptr(sym, &rel->syms) {
    //             Object *o = module_lookup_object(obj, sym->name, strlen(sym->name));
    //             ASSERT(o);
    //             sym->obj = o;
    //         }
    //     }
    // }

    return 0;
}

int kl_add_rel(Object *_m, RelocInfo *rel)
{
    ModuleObject *m = (ModuleObject *)_m;
    RelocInfo *item = NULL;
    vector_foreach_ptr(item, &m->rels) {
        const char *key = item->key;
        if (!key) continue;
        if (!strcmp(key, rel->key)) return -1;
    }

    vector_push_back(&m->rels, rel);
    return 0;
}

#ifdef __cplusplus
}
#endif
