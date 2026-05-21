/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "modobj.h"
#include "atom.h"
#include "codespec.h"
#include "klc.h"
#include "listobj.h"
#include "log.h"
#include "rangeobj.h"
#include "tupleobj.h"

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
    FuncEntry entry = { .obj = obj };
    vector_push_back(&m->func_entries, &entry);
    int func_idx = vector_size(&m->func_entries) - 1;
    kl_set_func_idx(obj, func_idx);
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
        if (code->flags & CODE_FLAG_METH) return 0;
        stbl_add_obj(&m->symbols, code->cs.name, obj);
        name = code->cs.name;
    }

    if (str_equal(name, "__init__")) {
        m->__init__ = obj;
    } else if (str_equal(name, "main")) {
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

int kl_mo_add_int(Object *_m, int64_t k, int type_info)
{
    ModuleObject *m = (ModuleObject *)_m;
    TValue val = { .tag = type_info, .ival = k };
    return kl_mo_add_const(_m, &val);
}

int kl_mo_add_uint(Object *_m, uint64_t k, int type_info)
{
    ModuleObject *m = (ModuleObject *)_m;
    TValue val = { .tag = type_info, .ival = k };
    return kl_mo_add_const(_m, &val);
}

int kl_mo_add_float(Object *_m, double k, int type_info)
{
    ModuleObject *m = (ModuleObject *)_m;
    TValue val = { .tag = type_info, .fval = k };
    return kl_mo_add_const(_m, &val);
}

int kl_mo_add_bool(Object *_m, int v)
{
    ModuleObject *m = (ModuleObject *)_m;
    TValue val = bool_value(v);
    return kl_mo_add_const(_m, &val);
}

int kl_mo_add_none(Object *_m)
{
    ModuleObject *m = (ModuleObject *)_m;
    TValue val = none_value;
    return kl_mo_add_const(_m, &val);
}

int kl_mo_add_str(Object *_m, char *s)
{
    ModuleObject *m = (ModuleObject *)_m;
    Object *sobj = kl_new_str(s);
    TValue val = obj_value(sobj);
    return kl_mo_add_const(_m, &val);
}

int _mo_add_vector(Object *_m, Vector *list) {}

int kl_mo_add_tuple(Object *_m, Vector *list)
{
    ModuleObject *m = (ModuleObject *)_m;
    Vector vec;
    vector_init(&vec, sizeof(TValue));

    KlcConst *item;
    vector_foreach(item, list) {
        switch (item->type) {
            case KLC_CONST_NONE: {
                TValue val = none_value;
                vector_push_back(&vec, &val);
                break;
            }

            case KLC_CONST_INT: {
                TValue val = { .tag = item->type_info, .ival = item->ival };
                vector_push_back(&vec, &val);
                break;
            }

            case KLC_CONST_FLT: {
                TValue val = { .tag = item->type_info, .fval = item->fval };
                vector_push_back(&vec, &val);
                break;
            }

            case KLC_CONST_BOOL: {
                TValue val = bool_value(item->ival);
                vector_push_back(&vec, &val);
                break;
            }

            case KLC_CONST_SHORT_ASCII:
            case KLC_CONST_SHORT_UTF8:
            case KLC_CONST_ASCII:
            case KLC_CONST_UTF8: {
                Object *sobj = kl_new_str(item->sval);
                TValue val = obj_value(sobj);
                vector_push_back(&vec, &val);
                break;
            }

            case KLC_CONST_SHORT_TUPLE:
            case KLC_CONST_TUPLE: {
                Vector *_sub = item->val;
                int _index = kl_mo_add_tuple(_m, _sub);
                TValue val = { .tag = item->type_info, .ival = _index };
                vector_push_back(&vec, &val);
                break;
            }

            case KLC_CONST_RANGE: {
                Vector *_sub = item->val;
                TValue values[3];

                KlcConst *_item;
                vector_foreach(_item, _sub) {
                    ASSERT(_item->type == KLC_CONST_INT);
                    values[i__] = int64_value(_item->ival);
                }

                Object *tobj = kl_new_range(values);
                TValue val = obj_value(tobj);
                vector_push_back(&vec, &val);
                break;
            }

            case KLC_CONST_SHORT_LIST:
            case KLC_CONST_LIST: {
                Vector *_sub = item->val;
                int _index = kl_mo_add_list(_m, _sub);
                TValue val = { .tag = item->type_info, .ival = _index };
                vector_push_back(&vec, &val);
                break;
            }

            default: {
                NYI();
                break;
            }
        }
    }

    TValue *items = VECTOR_RAW(&vec, TValue);
    int size = vector_size(&vec);
    Object *tobj = kl_new_tuple(items, size);
    TValue val = obj_value(tobj);
    vector_fini(&vec);
    return kl_mo_add_const(_m, &val);
}

int kl_mo_add_range(Object *_m, Vector *list)
{
    ModuleObject *m = (ModuleObject *)_m;
    Vector vec;
    vector_init(&vec, sizeof(TValue));

    KlcConst *item;
    vector_foreach(item, list) {
        ASSERT(item->type == KLC_CONST_INT);
        TValue val = int64_value(item->ival);
        vector_push_back(&vec, &val);
    }

    TValue *items = VECTOR_RAW(&vec, TValue);
    int size = vector_size(&vec);
    ASSERT(size == 3);
    Object *tobj = kl_new_range(items);
    TValue val = obj_value(tobj);
    vector_fini(&vec);
    return kl_mo_add_const(_m, &val);
}

int kl_mo_add_list(Object *_m, Vector *list)
{
    ModuleObject *m = (ModuleObject *)_m;
    Vector vec;
    vector_init(&vec, sizeof(TValue));

    KlcConst *item;
    vector_foreach(item, list) {
        switch (item->type) {
            case KLC_CONST_INT: {
                TValue val = { .tag = item->type_info, .ival = item->ival };
                vector_push_back(&vec, &val);
                break;
            }

            case KLC_CONST_FLT: {
                TValue val = { .tag = item->type_info, .fval = item->fval };
                vector_push_back(&vec, &val);
                break;
            }

            case KLC_CONST_SHORT_ASCII:
            case KLC_CONST_SHORT_UTF8:
            case KLC_CONST_ASCII:
            case KLC_CONST_UTF8: {
                Object *sobj = kl_new_str(item->sval);
                TValue val = obj_value(sobj);
                vector_push_back(&vec, &val);
                break;
            }

            case KLC_CONST_SHORT_TUPLE:
            case KLC_CONST_TUPLE: {
                Vector *_sub = item->val;
                int _index = kl_mo_add_tuple(_m, _sub);
                TValue val = { .tag = item->type_info, .ival = _index };
                vector_push_back(&vec, &val);
                break;
            }

            case KLC_CONST_RANGE: {
                Vector *_sub = item->val;
                TValue values[3];

                KlcConst *_item;
                vector_foreach(_item, _sub) {
                    ASSERT(_item->type == KLC_CONST_INT);
                    values[i__] = int64_value(_item->ival);
                }

                Object *tobj = kl_new_range(values);
                TValue val = obj_value(tobj);
                vector_push_back(&vec, &val);
                break;
            }

            case KLC_CONST_SHORT_LIST:
            case KLC_CONST_LIST: {
                Vector *_sub = item->val;
                int _index = kl_mo_add_list(_m, _sub);
                TValue val = { .tag = item->type_info, .ival = _index };
                vector_push_back(&vec, &val);
                break;
            }

            default: {
                NYI();
                break;
            }
        }
    }

    TValue *items = VECTOR_RAW(&vec, TValue);
    int size = vector_size(&vec);
    Object *tobj = kl_new_list(items, size);
    TValue val = obj_value(tobj);
    vector_fini(&vec);
    return kl_mo_add_const(_m, &val);
}

int kl_mo_add_import(Object *_m, ImportKind kind, char *path, char *kls, char *name)
{
    ModuleObject *m = (ModuleObject *)_m;

    ImportEntry entry = {
        .kind = kind,
        .path = atom(path),
        .kls = kls ? atom(kls) : NULL,
        .name = atom(name),
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
