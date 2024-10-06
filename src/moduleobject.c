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

static void module_fini(Object *self)
{
    ModuleObject *m = (ModuleObject *)self;
    ASSERT(IS_MODULE(m));
    vector_fini(&m->symbols);
    vector_fini(&m->rels);
}

TypeObject module_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "module",
    .fini = module_fini,
};

Object *kl_new_module(const char *name)
{
    ModuleObject *m = gc_alloc_obj_p(m);
    INIT_OBJECT_HEAD(m, &module_type);

    vector_init_ptr(&m->symbols);
    init_symbol_table(&m->map);

    vector_init(&m->consts, sizeof(Value));
    vector_init(&m->rels, sizeof(RelocInfo));
    RelocInfo not_used = { NULL };
    vector_init(&not_used.syms, sizeof(SymbolInfo));
    vector_push_back(&m->rels, &not_used);

    return (Object *)m;
}

int module_add_member(Object *_m, MemberDef *member)
{
    ModuleObject *m = (ModuleObject *)_m;
    Object *cfunc = kl_field_from_member(member);
    ASSERT(cfunc);
    module_add_object(_m, member->name, cfunc);
    return 0;
}

int module_add_getset(Object *_m, GetSetDef *getset) {}

int module_add_cfunc(Object *_m, MethodDef *def)
{
    ModuleObject *m = (ModuleObject *)_m;
    Object *cfunc = kl_new_cfunc(def, (Object *)m, NULL);
    ASSERT(cfunc);
    module_add_object(_m, def->name, cfunc);
    return 0;
}

int module_add_object(Object *_m, const char *name, Object *obj)
{
    ModuleObject *m = (ModuleObject *)_m;
    vector_push_back(&m->symbols, &obj);
    table_add_object(&m->map, name, obj);
    return 0;
}

int kl_add_module(const char *name, Object *m)
{
    table_add_object(&_gs_modules, name, m);
}

Object *kl_lookup_module(const char *path, int len)
{
    Object *obj = table_find(&_gs_modules, path, len);
    return obj;
}

Object *module_lookup_object(Object *_m, const char *name, int len)
{
    ModuleObject *m = (ModuleObject *)_m;
    Object *obj = table_find(&m->map, name, len);
    return obj;
}

int kl_module_def_init(ModuleDef *def)
{
    Object *m = kl_new_module(def->name);

    if (def->size) {
        void *state = mm_alloc(def->size);
        ((ModuleObject *)m)->state = state;
    }

    if (def->init) def->init(m);

    MethodDef *meth = def->methods;
    while (meth && meth->name) {
        module_add_cfunc(m, meth);
        ++meth;
    }

    ((ModuleObject *)m)->def = def;
    kl_add_module(def->name, m);

    return 0;
}

int kl_module_link(Object *_m)
{
    ASSERT(IS_MODULE(_m));
    ModuleObject *m = (ModuleObject *)_m;

    RelocInfo *rel;
    vector_foreach(rel, &m->rels) {
        const char *ns = rel->ns;
        if (!ns) continue;
        int len = strlen(rel->ns);
        char *dot = strrchr(rel->ns, '.');
        if (dot) len = dot - ns;
        Object *obj = kl_lookup_module(ns, len);
        ASSERT(obj);
        SymbolInfo *sym;
        vector_foreach(sym, &rel->syms) {
            Object *o = module_lookup_object(obj, sym->name, strlen(sym->name));
            ASSERT(o);
            sym->obj = o;
        }
    }

    return 0;
}

int module_add_rel(Object *_m, const char *path, SymbolInfo *sym)
{
    ModuleObject *m = (ModuleObject *)_m;
    RelocInfo *rel = NULL;
    vector_foreach(rel, &m->rels) {
        const char *ns = rel->ns;
        if (!ns) continue;
        if (!strcmp(ns, path)) {
            vector_push_back(&rel->syms, sym);
            return 0;
        }
    }

    RelocInfo new_rel = { .ns = path };
    vector_init(&new_rel.syms, sizeof(SymbolInfo));
    vector_push_back(&new_rel.syms, sym);
    vector_push_back(&m->rels, &new_rel);
    return 0;
}

int module_add_int_const(Object *_m, int64_t val)
{
    ModuleObject *m = (ModuleObject *)_m;
    Value v = int_value(val);
    vector_push_back(&m->consts, &v);
    return 0;
}

int module_add_str_const(Object *_m, const char *s)
{
    ModuleObject *m = (ModuleObject *)_m;
    Object *sobj = kl_new_str(s);
    ASSERT(sobj);
    Value v = obj_value(sobj);
    vector_push_back(&m->consts, &v);
    return 0;
}

#ifdef __cplusplus
}
#endif
