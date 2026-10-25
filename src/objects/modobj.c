/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "modobj.h"
#include "atom.h"
#include "bytesobj.h"
#include "dictobj.h"
#include "klc.h"
#include "listobj.h"
#include "rangeobj.h"
#include "tupleobj.h"

#ifdef __cplusplus
extern "C" {
#endif

/* global module table */
static HashMap _gm_stbl;

void kl_init_module_stbl(void) { stbl_init(&_gm_stbl); }

Object *kl_find_module(char *path) { return stbl_find_obj(&_gm_stbl, path); }

static int register_module(Object *_m)
{
    ModuleObject *m = (ModuleObject *)_m;
    stbl_add_obj(&_gm_stbl, m->path, _m);
    return 0;
}

/*---------------------------------------------------------------------------+
 |  Module type definition                                                   |
 +---------------------------------------------------------------------------*/

TypeObject module_type = {
    ._type = &type_type,
    .name = "module",
    .flags = TP_FLAGS_CLASS,
    .objsize = sizeof(ModuleObject),
};

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

int kl_mo_add_test(Object *_m, char *name, Object *obj, int expect_panic, char *msg, int skipped)
{
    ModuleObject *m = (ModuleObject *)_m;
    TestCase test_case = {
        .name = atom(name),
        .co = obj,
        .expect_panic = expect_panic,
        .msg = (msg && msg[0]) ? atom(msg) : NULL,
        .skipped = skipped,
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

int kl_mo_add_bytes(Object *_m, char *s, int len)
{
    ModuleObject *m = (ModuleObject *)_m;
    Object *bobj = kl_bytes_from_data((uint8_t *)s, len);
    TValue val = obj_value(bobj);
    return kl_mo_add_const(_m, &val);
}

static Object *__new_const_list(Object *_m, Vector *list);

/*
 * Build a tuple object from klc const items. Nested tuples/lists are built
 * recursively and are NOT appended to the module const pool — only the
 * top-level const gets a pool slot, matching the compiler's mach pool layout.
 */
static Object *__new_const_tuple(Object *_m, Vector *list)
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
                Vector *_sub = item->val;
                Object *tobj = __new_const_tuple(_m, _sub);
                TValue val = obj_value(tobj);
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
                Object *lobj = __new_const_list(_m, _sub);
                TValue val = obj_value(lobj);
                vector_push_back(&vec, &val);
                break;
            }

            case KLC_CONST_BYTES: {
                Object *bs = kl_bytes_from_data((uint8_t *)item->sval, (size_t)item->len);
                TValue val = obj_value(bs);
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
    vector_fini(&vec);
    return tobj;
}

int kl_mo_add_tuple(Object *_m, Vector *list)
{
    Object *tobj = __new_const_tuple(_m, list);
    TValue val = obj_value(tobj);
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
    Object *lobj = __new_const_list(_m, list);
    TValue val = obj_value(lobj);
    return kl_mo_add_const(_m, &val);
}

static Object *__new_const_list(Object *_m, Vector *list)
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
                Vector *_sub = item->val;
                Object *tobj = __new_const_tuple(_m, _sub);
                TValue val = obj_value(tobj);
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
                Object *_lobj = __new_const_list(_m, _sub);
                TValue val = obj_value(_lobj);
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
    vector_fini(&vec);
    return tobj;
}

/*
 * Convert one klc const item into a TValue, building nested containers
 * recursively. Mirrors the per-element dispatch inside __new_const_list().
 */
static TValue __const_to_tvalue(Object *_m, KlcConst *item)
{
    switch (item->type) {
        case KLC_CONST_NONE: {
            return nil_value;
        }

        case KLC_CONST_INT: {
            TValue val = { .tag = item->type_info, .ival = item->ival };
            return val;
        }

        case KLC_CONST_FLT: {
            TValue val = { .tag = item->type_info, .fval = item->fval };
            return val;
        }

        case KLC_CONST_BOOL: {
            return bool_value(item->ival);
        }

        case KLC_CONST_SHORT_ASCII:
        case KLC_CONST_SHORT_UTF8:
        case KLC_CONST_ASCII:
        case KLC_CONST_UTF8: {
            Object *sobj = kl_new_str(item->sval);
            return obj_value(sobj);
        }

        case KLC_CONST_BYTES: {
            Object *bs = kl_bytes_from_data((uint8_t *)item->sval, (size_t)item->len);
            return obj_value(bs);
        }

        case KLC_CONST_SHORT_TUPLE:
        case KLC_CONST_TUPLE: {
            Vector *_sub = item->val;
            Object *tobj = __new_const_tuple(_m, _sub);
            return obj_value(tobj);
        }

        case KLC_CONST_RANGE: {
            Vector *_sub = item->val;
            TValue values[3];

            KlcConst *_item;
            vector_foreach(_item, _sub) {
                ASSERT(_item->type == KLC_CONST_INT);
                values[i__] = int64_value(_item->ival);
            }

            Object *robj = kl_new_range(values);
            return obj_value(robj);
        }

        case KLC_CONST_SLICE: {
            Vector *_sub = item->val;
            TValue values[3];

            KlcConst *_item;
            vector_foreach(_item, _sub) {
                ASSERT(_item->type == KLC_CONST_INT);
                values[i__] = int64_value(_item->ival);
            }

            Object *sobj = kl_new_slice(values);
            return obj_value(sobj);
        }

        case KLC_CONST_SHORT_LIST:
        case KLC_CONST_LIST: {
            Vector *_sub = item->val;
            Object *lobj = __new_const_list(_m, _sub);
            return obj_value(lobj);
        }

        default: {
            NYI();
            return nil_value;
        }
    }
}

/*
 * Build a dict object from klc const items. `list` holds pairwise consts
 * (k0, v0, k1, v1, ...), the same layout the mach/klc const pool uses.
 * Dicts are mutable like lists: only the top-level const takes a pool slot,
 * nested containers are built recursively and stay unpooled.
 */
static Object *__new_const_dict(Object *_m, Vector *list)
{
    Object *dobj = kl_new_dict();

    int count = vector_size(list);
    for (int i = 0; i + 1 < count; i += 2) {
        KlcConst *kitem = vector_at(list, i);
        KlcConst *vitem = vector_at(list, i + 1);

        TValue key = __const_to_tvalue(_m, kitem);
        TValue val = __const_to_tvalue(_m, vitem);
        kl_dict_add(dobj, &key, &val);
    }

    return dobj;
}

int kl_mo_add_dict(Object *_m, Vector *list)
{
    Object *dobj = __new_const_dict(_m, list);
    TValue val = obj_value(dobj);
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

/* Human readable name of an import kind, used by the module dump. */
static const char *import_kind_name(ImportKind kind)
{
    switch (kind) {
        case IMPORT_KIND_FUNC:
            return "func";
        case IMPORT_KIND_GLOBAL:
            return "global";
        case IMPORT_KIND_TYPE:
            return "type";
        case IMPORT_KIND_METHOD:
            return "method";
        case IMPORT_KIND_FIELD:
            return "field";
        default:
            return "unknown";
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
    // printf("  func_entries : %d entries\n", vector_size(&m->func_entries));
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
        const char *kind = import_kind_name(e->kind);

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

#ifdef __cplusplus
}
#endif
