/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "atom.h"
#include "klc.h"
#include "log.h"
#include "mm.h"
#include "symbol.h"

#ifdef __cplusplus
extern "C" {
#endif

static void add_unsolved_sym_id(TypeSpec **ts, Vector *vec)
{
    if (!ts || !*ts) return;
    TypeSpec *t = *ts;
    if (t->kind != TYPE_SPECIALIZED && t->kind != TYPE_GENERIC_VAR) {
        return;
    }
    log_info("type needs update: %s", t->signature);

    TypeSpec **ts_ptr = ts;
    vector_push_back(vec, &ts_ptr);

    if (t->kind == TYPE_SPECIALIZED) {
        TypeSpec **arg_ts;
        vector_foreach_ptr(arg_ts, t->specialized.args) {
            add_unsolved_sym_id(arg_ts, vec);
        }
    }
}

static void add_func(HashMap *stbl, KlcFile *klc, KlcFunc *item, Vector *vec)
{
    KlcArgument *arg;
    Vector *params = vector_create_ptr();
    vector_foreach(arg, &item->args) {
        if (!arg) continue;
        ArgInfo *arg_info = mm_alloc_obj(arg_info);
        KlcConst *name = klc_get_const(klc, arg->name_index);
        KlcConst *ty_k = klc_get_const(klc, arg->type_index);
        TypeSpec *ts = type_spec_from_str(ty_k->sval);
        KlcConst *def_val = klc_get_const(klc, arg->const_index);
        arg_info->name = name->sval;
        arg_info->ts = ts;
        arg_info->dfl_val_idx = def_val ? 1 : 0;
        vector_push_back(params, &arg_info);
        if (ts->kind == TYPE_SPECIALIZED) {
            log_info("specialized type in function argument: %s", ts->signature);
            TypeSpec **ts_ptr = &arg_info->ts;
            add_unsolved_sym_id(ts_ptr, vec);
        } else if (ts->kind == TYPE_GENERIC_VAR) {
            log_info("generic var type in function argument: %s", ts->signature);
            TypeSpec **ts_ptr = &arg_info->ts;
            add_unsolved_sym_id(ts_ptr, vec);
        } else {
            // nothing
        }
    }

    KlcConst *k = klc_get_const(klc, item->name_index);
    KlcConst *ret = klc_get_const(klc, item->ret_type_index);
    TypeSpec *ret_ts = ret ? type_spec_from_str(ret->sval) : no_type_spec();

    Symbol *sym = stbl_add_func(stbl, k->sval, NULL, ret_ts, params, 0, NULL, NULL);

    if (ret_ts && ret_ts->kind == TYPE_SPECIALIZED) {
        log_info("specialized type in function return: %s", ret_ts->signature);
        TypeSpec **ts_ptr = &((FuncSymbol *)sym)->ret;
        add_unsolved_sym_id(ts_ptr, vec);
    } else if (ret_ts && ret_ts->kind == TYPE_GENERIC_VAR) {
        log_info("generic var type in function return: %s", ret_ts->signature);
        TypeSpec **ts_ptr = &((FuncSymbol *)sym)->ret;
        add_unsolved_sym_id(ts_ptr, vec);
    } else {
        // nothing
    }
}

static void add_klass(HashMap *stbl, KlcFile *klc, KlcKlass *kls, Vector *vec)
{
    KlcConst *k = klc_get_const(klc, kls->name_index);

    Symbol *cls_sym;
    if (kls->flags & KLC_FLAGS_TRAIT) {
        cls_sym = stbl_add_klass(stbl, k->sval, SYM_FLAGS_PUBLIC, 1);
    } else {
        cls_sym = stbl_add_klass(stbl, k->sval, SYM_FLAGS_PUBLIC, 0);
    }

    // add type params
    Vector *tps = vector_create_ptr();
    KlcTypeParam *arg;
    vector_foreach(arg, &kls->tps) {
        if (!arg) continue;

        KlcConst *name = klc_get_const(klc, arg->name_index);
        Symbol *tp_sym = stbl_add_type_param(cls_sym->stbl, name->sval, cls_sym);
        ((TypeParamSymbol *)tp_sym)->index = vector_size(tps);
        vector_push_back(tps, &tp_sym);

        // add bounds
        uint16_t bound;
        vector_foreach(bound, &arg->bounds) {
            if (!bound) continue;
            ASSERT(0);
            KlcConst *bound_k = klc_get_const(klc, bound);
            TypeSpec *ts = type_spec_from_str(bound_k->sval);
            TypeParamSymbol *tp = (TypeParamSymbol *)tp_sym;
            vector_push_back(tp->bound, &ts);
            if (ts->kind == TYPE_SPECIALIZED) {
                log_info("specialized type in type param bound: %s", ts->signature);
                TypeSpec **ts_ptr = &ts;
                add_unsolved_sym_id(ts_ptr, vec);
            } else {
                UNREACHABLE();
            }
        }
    }
    ((KlassSymbol *)cls_sym)->tps = tps;

    // read methods
    KlcFunc *fn;
    vector_foreach(fn, &kls->methods) {
        if (!fn) continue;
        add_func(cls_sym->stbl, klc, fn, vec);
    }
}

static void read_funcs(HashMap *stbl, KlcFile *klc, Vector *vec)
{
    Vector *consts = klc->objs + ITEM_CONST;

    KlcFunc *item;
    vector_foreach(item, klc->objs + ITEM_FUNC) {
        if (!item) continue;
        if (!(item->flags & KLC_FLAGS_PUB)) {
            continue;
        }
        add_func(stbl, klc, item, vec);
    }
}

static void read_klasses(HashMap *stbl, KlcFile *klc, Vector *vec)
{
    Vector *consts = klc->objs + ITEM_CONST;

    KlcKlass *kls;
    vector_foreach(kls, klc->objs + ITEM_CLASS) {
        if (!kls) continue;
        if (!(kls->flags & KLC_FLAGS_PUB)) {
            continue;
        }

        add_klass(stbl, klc, kls, vec);
    }
}

static void update_types_sym_id(HashMap *stbl, Vector *vec)
{
    TypeSpec **ts_ptr;
    TypeSpec *ts;
    vector_foreach(ts_ptr, vec) {
        if (!ts_ptr) continue;
        ts = *ts_ptr;
        if (ts->kind == TYPE_SPECIALIZED) {
            Symbol *sym = stbl_get(stbl, ts->specialized.name);
            if (sym) {
                log_debug("found symbol for specialized type: %s", ts->specialized.name);
                ts->sym_id = sym->id;
            } else {
                log_error("cannot find symbol for specialized type: %s",
                          ts->specialized.name);
            }
        } else if (ts->kind == TYPE_GENERIC_VAR) {
            Symbol *owner = stbl_get(stbl, ts->generic_var.owner);
            if (owner) {
                log_debug("found owner symbol for generic var type: %s",
                          ts->generic_var.owner);
                Symbol *tp_sym = stbl_get(owner->stbl, ts->generic_var.name);
                ASSERT(tp_sym && tp_sym->kind == SYM_TYPE_PARAM);
                log_debug("found type param symbol for generic var type: %s",
                          ts->generic_var.name);
                ts->sym_id = tp_sym->id;
                ts->generic_var.index = ((TypeParamSymbol *)tp_sym)->index;
            } else {
                log_error("cannot find owner symbol for generic var type: %s",
                          ts->generic_var.owner);
            }
        } else {
            UNREACHABLE();
        }
    }
}

void load_module(ModuleSymbol *mod_sym, char *path)
{
    log_debug("read klc file: %s", path);

    KlcFile klc;

    init_klc_file(&klc, path);
    read_klc_file(&klc, 0);

#ifndef NOLOG
    klc_dump(&klc);
#endif

    Vector vec = VECTOR_INIT_PTR;
    HashMap *stbl = mod_sym->stbl;

    // read_vars(stbl, &klc, vec);
    read_funcs(stbl, &klc, &vec);
    read_klasses(stbl, &klc, &vec);

    update_types_sym_id(stbl, &vec);

    vector_fini(&vec);

    fini_klc_file(&klc);
}

#ifdef __cplusplus
}
#endif
