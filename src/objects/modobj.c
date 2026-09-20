/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "modobj.h"
#include "atom.h"
#include "klc.h"
#include "listobj.h"
#include "rangeobj.h"
#include "tupleobj.h"

#ifdef __cplusplus
extern "C" {
#endif

/* global module table */
static HashMap _gm_stbl;

void kl_init_gm_stbl(void) { stbl_init(&_gm_stbl); }

Object *kl_get_module(char *path) { return stbl_find_obj(&_gm_stbl, path); }

static int register_module(Object *_m)
{
    ModuleObject *m = (ModuleObject *)_m;
    stbl_add_obj(&_gm_stbl, m->path, _m);
    return 0;
}

static void dump_value(TValue *val)
{
    if (is_int(val)) {
        printf("%" PRId64, val->ival);
    } else if (is_float(val)) {
        printf("%f", val->fval);
    } else if (is_ref(val)) {
        Object *obj = to_obj(val);
        if (IS_STR(obj)) {
            printf("\"%s\"", STR_BUF(obj));
        } else {
            printf("%p", obj);
        }
    } else {
        NYI();
    }
}

void kl_dump_module(Object *_m)
{
    ModuleObject *m = (ModuleObject *)_m;

    printf("========================================\n");
    printf("  Module: %s\n", m->path);
    printf("========================================\n\n");

    printf("  codes        : %p\n", m->codes);
    printf("  num_codes    : %u insns\n", m->num_codes);
    printf("  func_entries : %d entries\n", vector_size(&m->func_entries));
    printf("  const_pool   : %d constants\n", vector_size(&m->const_pool));
    printf("  import_table : %d imports\n", vector_size(&m->import_table));
    printf("  init         : %p\n", m->__init__);
    printf("  main         : %p\n", m->main);
    printf("  funcs        : %d functions\n", vector_size(&m->funcs));
    printf("  types        : %d types\n", vector_size(&m->types));
    printf("\n");

    /* Import Table */
    printf("Import Table (%d imports)\n", vector_size(&m->import_table));
    printf("  IDX  KIND     MODULE                 SYMBOL                 ADDRESS\n");
    printf("  --------------------------------------------------------------------------\n");

    ImportEntry *e;
    vector_foreach_ptr(e, &m->import_table) {
        const char *mod = e->path;
        const char *sym = e->name;
        const char *kind = (e->kind == IMPORT_KIND_FUNC)     ? "func"
                           : (e->kind == IMPORT_KIND_GLOBAL) ? "global"
                           : (e->kind == IMPORT_KIND_TYPE)   ? "type"
                           : (e->kind == IMPORT_KIND_METHOD) ? "method"
                           : (e->kind == IMPORT_KIND_FIELD)  ? "field"
                                                             : "unknown";

        printf("  %03d  %-8s %-22s %-22s %p\n", i__, kind, mod, sym, e->address);
    }
    printf("\n");

    /* Const Pool */
    printf("Const Pool (%d constants)\n", vector_size(&m->const_pool));
    printf("  IDX  VALUE\n");
    printf("  --------------\n");

    TValue *val;
    vector_foreach_ptr(val, &m->const_pool) {
        printf("  %03d  ", i__);
        dump_value(val);
        printf("\n");
    }
    printf("\n");

    // /* Func Entries */
    // printf("Func Entries (%d entries)\n", vector_size(&m->func_entries));

    // FuncEntry *func_entry;
    // vector_foreach_ptr(func_entry, &m->func_entries) {
    //     printf("  %03d  native=%d  obj=%p\n", i__, func_entry->native,
    //     func_entry->obj);
    // }

    printf("\n========================================\n\n");
}

/*---------------------------------------------------------------------------+
 |  Module type definition                                                   |
 +---------------------------------------------------------------------------*/

TypeObject module_type = {
    ._type = &type_type,
    .name = "module",
    .flags = TP_FLAGS_CLASS,
    .priv_size = sizeof(ModuleObject),
};

int kl_bind_func(Object *_m, Object *obj)
{
    ModuleObject *m = (ModuleObject *)_m;
    FuncEntry entry = { .obj = obj };
    vector_push_back(&m->func_entries, &entry);
    int func_idx = vector_size(&m->func_entries) - 1;
    kl_set_func_idx(obj, func_idx);
    return 0;
}

int kl_mo_add_func(Object *_m, char *name, Object *obj)
{
    ModuleObject *m = (ModuleObject *)_m;
    vector_push_back(&m->funcs, &obj);

    if (IS_CFUNC(obj)) {
        CFuncObject *cfunc = (CFuncObject *)obj;
        stbl_add_obj(&m->symbols, name, obj);
    } else {
        ASSERT(IS_CODE(obj));
        CodeObject *code = (CodeObject *)obj;
        if (code->flags & CODE_FLAG_METH) return 0;
        stbl_add_obj(&m->symbols, name, obj);
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

int kl_mo_add_test(Object *_m, char *name, CodeObject *obj, int expect_panic, char *msg)
{
    ModuleObject *m = (ModuleObject *)_m;
    TestCase test_case = {
        .name = atom(name),
        .co = obj,
        .expect_panic = expect_panic,
        .msg = msg ? atom(msg) : "<no message>",
    };
    vector_push_back(&m->tests, &test_case);
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
    TValue val = nil_value;
    return kl_mo_add_const(_m, &val);
}

int kl_mo_add_str(Object *_m, char *s)
{
    ModuleObject *m = (ModuleObject *)_m;
    Object *sobj = kl_new_str(s);
    TValue val = obj_value(sobj);
    return kl_mo_add_const(_m, &val);
}

int kl_mo_add_tuple(Object *_m, Vector *list)
{
    ModuleObject *m = (ModuleObject *)_m;
    Vector vec;
    vector_init(&vec, sizeof(TValue));

    KlcConst *item;
    vector_foreach(item, list) {
        switch (item->type) {
            case KLC_CONST_NONE: {
                TValue val = nil_value;
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
                NYI();
                // Vector *_sub = item->val;
                // int _index = kl_mo_add_tuple(_m, _sub);
                // TValue val = { .tag = item->type_info, .ival = _index };
                // vector_push_back(&vec, &val);
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

            case KLC_CONST_SLICE: {
                Vector *_sub = item->val;
                TValue values[3];

                KlcConst *_item;
                vector_foreach(_item, _sub) {
                    ASSERT(_item->type == KLC_CONST_INT);
                    values[i__] = int64_value(_item->ival);
                }

                Object *tobj = kl_new_slice(values);
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

    TValue *items = VECTOR_ITEMS(&vec, TValue);
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

    TValue *items = VECTOR_ITEMS(&vec, TValue);
    int size = vector_size(&vec);
    ASSERT(size == 3);
    Object *tobj = kl_new_range(items);
    TValue val = obj_value(tobj);
    vector_fini(&vec);
    return kl_mo_add_const(_m, &val);
}

int kl_mo_add_slice(Object *_m, Vector *list)
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

    TValue *items = VECTOR_ITEMS(&vec, TValue);
    int size = vector_size(&vec);
    ASSERT(size == 3);
    Object *tobj = kl_new_slice(items);
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

            case KLC_CONST_SLICE: {
                Vector *_sub = item->val;
                TValue values[3];

                KlcConst *_item;
                vector_foreach(_item, _sub) {
                    ASSERT(_item->type == KLC_CONST_INT);
                    values[i__] = int64_value(_item->ival);
                }

                Object *tobj = kl_new_slice(values);
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

    TValue *items = VECTOR_ITEMS(&vec, TValue);
    int size = vector_size(&vec);
    Object *tobj = kl_list_from_array(items, size);
    TValue val = obj_value(tobj);
    vector_fini(&vec);
    return kl_mo_add_const(_m, &val);
}

int kl_mo_add_import(Object *_m, ImportKind kind, char *path, char *kls, char *name, int slot_index)
{
    ModuleObject *m = (ModuleObject *)_m;

    ImportEntry entry = {
        .kind = kind,
        .path = atom(path),
        .kls = kls ? atom(kls) : NULL,
        .name = atom(name),
        .address = NULL,
        .slot_index = slot_index,
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

    INIT_OBJECT_HEAD(m, &module_type, 0);
    vector_init(&m->const_pool, sizeof(TValue));
    vector_init(&m->import_table, sizeof(ImportEntry));
    vector_init(&m->func_entries, sizeof(FuncEntry));
    vector_init_ptr(&m->funcs);
    vector_init_ptr(&m->types);
    vector_init_ptr(&m->globals);
    vector_init(&m->tests, sizeof(TestCase));
    vector_init(&m->lineinfos, sizeof(LineInfo));
    vector_init(&m->libs, sizeof(NativeLib));
    stbl_init(&m->symbols);
    m->path = atom(path);

    register_module((Object *)m);
    return (Object *)m;
}

void kl_free_module(Object *m)
{
    // TODO: implement module free logic
}

#ifdef __cplusplus
}
#endif
