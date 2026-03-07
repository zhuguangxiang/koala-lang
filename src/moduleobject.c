/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "moduleobject.h"
#include "cfuncobject.h"
#include "gc.h"
#include "log.h"
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
    ModuleObject *m = gc_alloc_obj_p(m);
    INIT_OBJECT_HEAD(m, &module_type);

    vector_init_ptr(&m->symbols);
    init_sym_tbl(&m->map);
    vector_init(&m->consts, sizeof(Value));
    vector_init(&m->rels, sizeof(RelocEntry));

    RelocEntry not_used = { NULL };
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

static int _do_link(RelocEntry *rel, Object *_m)
{
    if (rel->obj) return 0;

    ModuleObject *m = (ModuleObject *)_m;
    char *key = rel->key;
    if (!key) {
        log_error("invalid relocation entry with empty key");
        return 0;
    }

    if (rel->kind == REL_TYPE_MODULE) {
        Object *obj = sym_tbl_find(&_gs_modules, key, strlen(key));
        ASSERT(obj && IS_MODULE(obj));
        rel->obj = obj;
    } else if (rel->kind == REL_TYPE_KLASS) {
        ASSERT(rel->parent > 0);
        RelocEntry *parent = kl_get_rel(_m, rel->parent);
        ASSERT(parent);
        Object *obj = parent->obj;
        if (!obj) {
            _do_link(parent, _m);
            obj = parent->obj;
            if (!obj) {
                log_error("linking '%s' failed: parent '%s' not found", key, parent->key);
                return -1;
            }
        }
        ASSERT(obj && IS_MODULE(obj));
        obj = module_lookup(obj, key, strlen(key));
        ASSERT(obj && IS_TYPE(obj, &type_type));
        rel->obj = obj;
    } else if (rel->kind == REL_TYPE_FUNC) {
        ASSERT(rel->parent > 0);
        RelocEntry *parent = kl_get_rel(_m, rel->parent);
        ASSERT(parent);
        Object *obj = parent->obj;
        if (!obj) {
            _do_link(parent, _m);
            obj = parent->obj;
            if (!obj) {
                log_error("linking '%s' failed: parent '%s' not found", key, parent->key);
                return -1;
            }
        }
        if (IS_MODULE(obj)) {
            obj = module_lookup(obj, key, strlen(key));
        } else if (IS_TYPE(obj, &type_type)) {
            obj = sym_tbl_find(&((TypeObject *)obj)->map, key, strlen(key));
        } else {
            UNREACHABLE();
        }
        ASSERT(obj && IS_CFUNC(obj));
        rel->obj = obj;
    } else if (rel->kind == REL_TYPE_VAR) {
        NYI();
    } else {
        UNREACHABLE();
    }
    return 0;
}

int kl_do_link(Object *_m)
{
    ASSERT(IS_MODULE(_m));
    ModuleObject *m = (ModuleObject *)_m;

    RelocEntry *rel;
    vector_foreach_ptr(rel, &m->rels) {
        char *key = rel->key;
        if (!key) continue;
        if (_do_link(rel, _m) < 0) {
            log_error("linking '%s' failed", key);
        } else {
            log_info("linked '%s' successfully", key);
        }
    }

    return 0;
}

int kl_add_rel(Object *_m, int kind, char *key, int parent)
{
    RelocEntry rel = { .key = strdup(key), .kind = kind, .parent = parent };
    ModuleObject *m = (ModuleObject *)_m;
    vector_push_back(&m->rels, &rel);
    return vector_size(&m->rels) - 1;
}

#ifdef __cplusplus
}
#endif
