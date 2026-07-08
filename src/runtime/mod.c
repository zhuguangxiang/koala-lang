/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "atom.h"
#include "modobj.h"
#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

/* global module table */
static HashMap _gm_stbl;

/* the module has native implementations */
static HashMap _gm_native_stbl;

void kl_init_gm_stbl(void)
{
    stbl_init(&_gm_stbl);
    stbl_init(&_gm_native_stbl);
}

Object *kl_get_module(char *path)
{
    Object *m = stbl_find_obj(&_gm_stbl, path);
    if (m) return m;
    return kl_load_module(path);
}

int kl_register_module(Object *_m)
{
    ModuleObject *m = (ModuleObject *)_m;
    stbl_add_obj(&_gm_stbl, m->path, _m);
    return 0;
}

NativeFunc kl_get_native(char *name)
{
    Object *obj = stbl_find_obj(&_gm_native_stbl, name);
    return obj ? ((CFuncObject *)obj)->fn : NULL;
}

int kl_register_native(char *name, NativeFunc fn)
{
    Object *obj = kl_new_cfunc(name, fn, NULL);
    stbl_add_obj(&_gm_native_stbl, name, obj);
    return 0;
}

void kl_resolve_import(Object *_m)
{
    ModuleObject *m = (ModuleObject *)_m;

    ImportEntry *e;
    vector_foreach_ptr(e, &m->import_table) {
        Object *obj = NULL;

        if (e->kind == IMPORT_KIND_FUNC || e->kind == IMPORT_KIND_GLOBAL ||
            e->kind == IMPORT_KIND_TYPE) {
            Object *mod = kl_get_module(e->path);
            if (!mod) {
                panic("failed to resolve import: module '%s' is not found", e->path);
                return;
            }

            obj = kl_mo_find(mod, e->name);
            if (!obj) {
                panic("failed to resolve import: symbol '%s::%s' is not found", e->path, e->name);
                return;
            }
        } else {
            ASSERT(e->kind == IMPORT_KIND_METHOD || e->kind == IMPORT_KIND_FIELD);
            Object *mod = kl_get_module(e->path);
            if (!mod) {
                panic("failed to resolve import: module '%s' is not found", e->path);
                return;
            }

            Object *cls = kl_mo_find(mod, e->kls);
            if (!cls || !IS_TYPE(cls, &type_type)) {
                panic("failed to resolve import: class '%s::%s' is not found", e->path, e->kls);
                return;
            }

            obj = kl_type_find((TypeObject *)cls, e->name);
            if (!obj) {
                panic("failed to resolve import: symbol '%s::%s' is not found", e->kls, e->name);
                return;
            }
        }

        e->address = obj;
    }
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

#ifdef __cplusplus
}
#endif
