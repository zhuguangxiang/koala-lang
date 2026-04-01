/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "modobj.h"
#include "atom.h"
#include "codespec.h"
#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------+
 |  Module type definition                                                   |
 +---------------------------------------------------------------------------*/

TypeObject module_type = {
    ._type = &type_type,
    .name = "module",
    .flags = TP_FLAGS_CLASS,
};

static TValue not_impl_func(TValue *self, TValue *args, int nargs)
{
    Object *obj = to_obj(self);
    ASSERT(IS_CFUNC(obj));
    CFuncObject *cfunc = (CFuncObject *)obj;
    Object *_m = cfunc->owner;
    ASSERT(IS_MODULE(_m));
    ModuleObject *m = (ModuleObject *)_m;
    // raise_exc_str("function not implemented: %s::%s!", m->path, cfunc->name);
    return error_value;
}

int kl_bind_func(Object *_m, Object *obj)
{
    ModuleObject *m = (ModuleObject *)_m;

    int start_pc;
    int nlocals;
    int native;

    if (IS_CFUNC(obj)) {
        start_pc = 0;
        nlocals = 0;
        native = 1;
    } else {
        ASSERT(IS_CODE(obj));
        CodeObject *code = (CodeObject *)obj;
        start_pc = code->cs.start_pc;
        nlocals = code->cs.nlocals;
        native = 0;
    }

    FuncEntry entry = {
        // .start_pc = start_pc,
        // .nlocals = nlocals,
        .native = native,
        .obj = obj,
    };

    vector_push_back(&m->func_entries, &entry);

    return 0;
}

int kl_mo_add_func(Object *_m, Object *obj)
{
    ModuleObject *m = (ModuleObject *)_m;
    vector_push_back(&m->funcs, &obj);

    char *name;
    if (IS_CFUNC(obj)) {
        CFuncObject *cfunc = (CFuncObject *)obj;
        stbl_add_obj(&m->symbols, cfunc->name, obj);
        name = cfunc->name;
    } else {
        ASSERT(IS_CODE(obj));
        CodeObject *code = (CodeObject *)obj;
        stbl_add_obj(&m->symbols, code->cs.name, obj);
        name = code->cs.name;
    }

    if (str_eq(name, "__init__")) {
        m->__init__ = obj;
    } else if (str_eq(name, "main")) {
        m->main = obj;
    } else {
        // do nothing
    }

    return 0;
}

int kl_mo_add_type(Object *_m, TypeObject *tp)
{
    ModuleObject *m = (ModuleObject *)_m;
    tp->module = _m;
    vector_push_back(&m->types, &tp);
    stbl_add_obj(&m->symbols, tp->name, (Object *)tp);
    return 0;
}

int kl_mo_add_global(Object *_m, char *name)
{
    // ModuleObject *m = (ModuleObject *)_m;
    // int index = vector_size(&m->globals);
    // vector_push_back(&m->globals, &index);
    // stbl_add_obj(&m->symbols, name, (Object *)&index);
    // return index;
    NYI();
}

int kl_mo_add_const(Object *_m, TValue *val)
{
    ModuleObject *m = (ModuleObject *)_m;
    int index = vector_size(&m->const_pool);
    vector_push_back(&m->const_pool, val);
    return index;
}

int kl_mo_add_int(Object *_m, int64_t k)
{
    ModuleObject *m = (ModuleObject *)_m;
    TValue val = int64_value(k);
    return kl_mo_add_const(_m, &val);
}

int kl_mo_add_str(Object *_m, char *s)
{
    ModuleObject *m = (ModuleObject *)_m;
    Object *sobj = kl_new_str(s);
    TValue val = obj_value(sobj);
    return kl_mo_add_const(_m, &val);
}

int kl_mo_add_import(Object *_m, ImportKind kind, char *path, char *name)
{
    ModuleObject *m = (ModuleObject *)_m;

    ImportEntry entry = {
        .kind = kind,
        .path = path,
        .name = name,
        .address = NULL,
    };

    int index = vector_size(&m->import_table);
    vector_push_back(&m->import_table, &entry);
    return index;
}

Object *kl_mo_find(Object *_m, char *name)
{
    ModuleObject *m = (ModuleObject *)_m;
    return stbl_find_obj(&m->symbols, name);
}

void kl_mo_set_code(Object *_m, uint32_t *insns, size_t n)
{
    ModuleObject *m = (ModuleObject *)_m;
    m->codes = insns;
    m->num_codes = n;
}

Object *kl_new_module(char *path)
{
    ModuleObject *m = mm_alloc_obj(m);

    INIT_OBJECT_HEAD(m, &module_type);
    vector_init(&m->const_pool, sizeof(TValue));
    vector_init(&m->import_table, sizeof(ImportEntry));
    vector_init(&m->func_entries, sizeof(FuncEntry));
    vector_init_ptr(&m->funcs);
    vector_init_ptr(&m->types);
    stbl_init(&m->symbols);
    m->path = atom(path);
    Object *cfunc = kl_new_cfunc("not_impl", not_impl_func, (Object *)m);
    m->not_impl = cfunc;
    kl_register_module((Object *)m);
    return (Object *)m;
}

void kl_free_module(Object *m)
{
    // TODO: implement module free logic
}

int kl_init_module(Object *_m)
{
    ModuleObject *m = (ModuleObject *)_m;

    // bind cfunc/code to module
    Object *fn;
    vector_foreach(fn, &m->funcs) {
        kl_bind_func(_m, fn);
    }

    // setup types
    TypeObject *tp;
    vector_foreach(tp, &m->types) {
        kl_init_type(tp);
    }

    // allocate global variables space
    if (m->num_values > 0) {
        m->values = mm_alloc(sizeof(TValue) * m->num_values);
        for (uint32_t i = 0; i < m->num_values; i++) {
            m->values[i] = none_value;
        }
    }

    return 0;
}

Object *kl_new_native_module(ModuleDef *def)
{
    Object *_m = kl_new_module(def->path);
    ModuleObject *m = (ModuleObject *)_m;

    // set global variables' number
    m->num_values = def->nvars;

    // add functions
    MethodDef *fn = def->funcs;
    while (fn && fn->name) {
        Object *cfunc = kl_new_cfunc(fn->name, fn->cfunc, _m);
        kl_mo_add_func(_m, cfunc);
        log_info("added func '%s' to module '%s'", fn->name, m->path);
        fn++;
    }

    // add types
    TypeObject **tp = def->types;
    while (*tp) {
        kl_mo_add_type(_m, *tp);
        tp++;
    }

    kl_init_module(_m);

    return _m;
}

#ifdef __cplusplus
}
#endif
