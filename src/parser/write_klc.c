/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "atom.h"
#include "cgen.h"
#include "cmd.h"
#include "klc.h"
#include "log.h"
#include "mm.h"
#include "parser.h"

#ifdef __cplusplus
extern "C" {
#endif

static uint16_t klc_add_const(KlcFile *klc, Literal *lit)
{
    uint16_t index = 0;
    switch (lit->which) {
        case LIT_INT: {
            index = klc_add_int(klc, lit->ival, lit->sign, lit->len);
            break;
        }
        case LIT_FLT: {
            index = klc_add_float(klc, lit->fval, lit->len);
            break;
        }
        case LIT_STR: {
            index = klc_add_utf8(klc, lit->sval, lit->len);
            break;
        }
        case LIT_BOOL: {
            index = klc_add_int(klc, lit->bval ? 1 : 0, 0, 1);
            break;
        }
        case LIT_NONE: {
            index = klc_add_none(klc);
            break;
        }
        default: {
            UNREACHABLE();
        }
    }
    return index;
}

static void write_meta_type(TypeSpec *ts, Vector *vec, KlcFile *klc)
{
    int len = strlen(ts->signature);
    uint16_t index = klc_add_str(klc, ts->signature, len);
    vector_push_back(vec, &index);
}

static void write_meta_global(VarSymbol *var, KlcFile *klc)
{
    uint16_t dfl_val_idx = 0;
    Literal *lit = var->lit;
    if (lit) {
        dfl_val_idx = klc_add_const(klc, lit);
    }

    int flags = 0;
    if (var->flags & SYM_FLAGS_MUTABLE) {
        flags |= KLC_FLAGS_MUT;
    }
    if (var->flags & SYM_FLAGS_PUBLIC) {
        flags |= KLC_FLAGS_PUB;
    }

    klc_add_var(klc, var->name, var->ts->signature, dfl_val_idx, flags);
}

static void write_meta_func(FuncSymbol *fn_sym, KlcKlass *klass, KlcFile *klc)
{
    int flags = 0;

    if (fn_sym->flags & SYM_FLAGS_PUBLIC) {
        flags |= KLC_FLAGS_PUB;
    }

    KlcFunc *fn = NULL;
    if (klass) {
        flags |= KLC_FLAGS_METH;
        fn = klc_klass_add_func(klass, fn_sym->name, fn_sym->ret->signature, flags);
    } else {
        fn = klc_add_func(klc, fn_sym->name, fn_sym->ret->signature, flags);
    }

    fn->code_index = fn_sym->code_index;

    // add type parameters
    TypeParamSymbol *tp;
    vector_foreach(tp, &fn_sym->tps) {
        if (!tp) continue;

        KlcTypeParam *klc_tp = klc_func_add_tp(fn, tp->name);
        klc_tp->which = (int8_t)tp->which;

        TypeSpec *_ts;
        vector_foreach(_ts, &tp->bound) {
            if (!_ts) continue;
            write_meta_type(_ts, &klc_tp->bounds, klc);
        }
    }

    // add argument info
    ArgInfo *item;
    vector_foreach(item, fn_sym->params) {
        if (!item) continue;
        ASSERT(item->sym->kind == SYM_VAR);
        VarSymbol *var_sym = (VarSymbol *)item->sym;
        ASSERT(var_sym->scope == VAR_SCOPE_PARAM);
        uint16_t dfl_val_idx = 0;
        if (var_sym->lit) {
            // has default value
            dfl_val_idx = klc_add_const(klc, var_sym->lit);
        }
        klc_func_add_arg(fn, item->name, item->ts->signature, dfl_val_idx);
    }

    // add annotations
    if (fn_sym->ann) {
        klc_func_add_ann(fn, fn_sym->ann, fn_sym->ann_key, NULL);
    }
}

static void write_meta_field(VarSymbol *fld_sym, KlcKlass *klass)
{
    int flags = 0;
    if (fld_sym->flags & SYM_FLAGS_MUTABLE) {
        flags |= KLC_FLAGS_MUT;
    }

    if (fld_sym->flags & SYM_FLAGS_PUBLIC) {
        flags |= KLC_FLAGS_PUB;
    }

    klc_klass_add_field(klass, fld_sym->name, fld_sym->ts->signature, flags);
}

static void write_meta_intf_entry(IntfEntry *intf_entry, KlcKlass *klass)
{
    Symbol *trait_sym = intf_entry->trait;
    ASSERT(trait_sym->kind == SYM_TRAIT);
    uint16_t index = klc_add_str(klass->filp, trait_sym->name, strlen(trait_sym->name));
    KlcIntfEntry *entry = klc_klass_add_intf_entry(klass);
    entry->name_index = index;

    FuncSymbol *fn_sym;
    vector_foreach(fn_sym, &intf_entry->methods) {
        if (!fn_sym) {
            ASSERT(0); // should not happen, but just in case
            uint16_t null_idx = -1;
            vector_push_back(&entry->methods, &null_idx);
        } else {
            ASSERT(fn_sym->kind == SYM_FUNC);
            vector_push_back(&entry->methods, (uint16_t *)&fn_sym->code_index);
        }
    }

    IntfEntry *parent;
    vector_foreach(parent, &intf_entry->parents) {
        if (!parent) continue;
        vector_push_back(&entry->parents, (uint16_t *)&parent->index);
    }
}

static void write_meta_klass(KlassSymbol *kls_sym, KlcFile *klc)
{
    int flags = 0;
    if (kls_sym->flags & SYM_FLAGS_PUBLIC) {
        flags |= KLC_FLAGS_PUB;
    }

    if (kls_sym->kind == SYM_TRAIT) {
        flags |= KLC_FLAGS_TRAIT;
    }

    KlcKlass *klass = klc_add_klass(klc, kls_sym->name, flags);
    kls_sym->klc_entry = klass;

    // add type parameters
    TypeParamSymbol *tp;
    vector_foreach(tp, &kls_sym->tps) {
        if (!tp) continue;

        KlcTypeParam *klc_tp = klc_klass_add_tp(klass, tp->name);
        klc_tp->which = (int8_t)tp->which;
        TypeSpec *_ts;
        vector_foreach(_ts, &tp->bound) {
            if (!_ts) continue;
            write_meta_type(_ts, &klc_tp->bounds, klc);
        }
    }

    TypeSpec *ts;

    // add base classes
    vector_foreach(ts, &kls_sym->bases) {
        if (!ts) continue;
        write_meta_type(ts, &klass->bases, klc);
    }

    // add pip & lro
    vector_foreach(ts, &kls_sym->pip) {
        if (!ts) continue;
        write_meta_type(ts, &klass->pip, klc);
    }

    vector_foreach(ts, &kls_sym->lro) {
        if (!ts) continue;
        write_meta_type(ts, &klass->lro, klc);
    }

    // add fields
    VarSymbol *field;
    vector_foreach(field, kls_sym->fields) {
        if (!field) continue;
        write_meta_field(field, klass);
    }

    // add methods
    Symbol *fn;
    vector_foreach(fn, kls_sym->funcs) {
        if (!fn) continue;
        if (fn->kind != SYM_FUNC) continue;
        write_meta_func((FuncSymbol *)fn, klass, klc);
    }
}

static void write_meta(HashMap *stbl, KlcFile *klc)
{
    // write global variables, functions and classes&traits
    HashMapIter it = { 0 };
    while (hashmap_next(stbl, &it)) {
        Symbol *sym = (Symbol *)it.entry;
        switch (sym->kind) {
            case SYM_VAR: {
                VarSymbol *var = (VarSymbol *)sym;
                write_meta_global(var, klc);
                break;
            }

            case SYM_FUNC: {
                FuncSymbol *fn = (FuncSymbol *)sym;
                write_meta_func(fn, NULL, klc);
                break;
            }

            case SYM_CLASS:
            case SYM_TRAIT: {
                KlassSymbol *kls = (KlassSymbol *)sym;
                write_meta_klass(kls, klc);
                break;
            }

            case SYM_INSTANCE: {
                // do nothing
                break;
            }

            default: {
                UNREACHABLE();
                break;
            }
        }
    }

    if (!is_build_stdlib()) {
        if (dump_itable_enabled()) {
            // dump interface table for debugging
            dump_intf_table(stbl);
        }

        // write interface table for each class
        HashMapIter it2 = { 0 };
        while (hashmap_next(stbl, &it2)) {
            Symbol *sym = (Symbol *)it2.entry;
            if (sym->kind != SYM_CLASS) continue;
            // add interface table
            KlassSymbol *kls_sym = (KlassSymbol *)sym;
            IntfEntry *intf_entry;
            vector_foreach(intf_entry, &kls_sym->intf_table) {
                if (!intf_entry) continue;
                write_meta_intf_entry(intf_entry, kls_sym->klc_entry);
            }
        }
    }
}

static uint16_t _write_rt_const(KlcFile *klc, KlMachConst *kc)
{
    switch (kc->tag) {
        case KL_MACH_CONST_NONE:
            return klc_add_rt_none(klc);
        case KL_MACH_CONST_INT:
            return klc_add_rt_int(klc, kc->i64, 1, kc->len);
        case KL_MACH_CONST_UINT:
            return klc_add_rt_int(klc, kc->u64, 0, kc->len);
        case KL_MACH_CONST_FLOAT:
            return klc_add_rt_float(klc, kc->f64, kc->len);
        case KL_MACH_CONST_BOOL:
            return klc_add_rt_bool(klc, kc->bval);
        case KL_MACH_CONST_STR:
            return klc_add_rt_str(klc, kc->str, strlen(kc->str));
        case KL_MACH_CONST_TUPLE: {
            Vector *list = vector_create(sizeof(uint16_t));
            Vector *vec = kc->list;
            KlMachConst *item;
            vector_foreach(item, vec) {
                if (!item) continue;
                uint16_t idx = _write_rt_const(klc, item);
                vector_push_back(list, &idx);
            }
            return klc_add_rt_tuple(klc, list);
        }
        case KL_MACH_CONST_RANGE: {
            Vector *list = vector_create(sizeof(uint16_t));
            Vector *vec = kc->list;
            KlMachConst *range_item;
            vector_foreach(range_item, vec) {
                if (!range_item) continue;
                uint16_t idx = _write_rt_const(klc, range_item);
                vector_push_back(list, &idx);
            }
            return klc_add_rt_range(klc, list);
        }
        case KL_MACH_CONST_LIST: {
            Vector *list = vector_create(sizeof(uint16_t));
            Vector *vec = kc->list;
            KlMachConst *item;
            vector_foreach(item, vec) {
                if (!item) continue;
                uint16_t idx = _write_rt_const(klc, item);
                vector_push_back(list, &idx);
            }
            return klc_add_rt_list(klc, list);
        }
        default:
            UNREACHABLE();
    }
}

static void write_rt_data(KlMachModule *m, KlcFile *klc, HashMap *stbl)
{
    klc->num_rt_consts = vector_size(&m->const_pool);

    KlMachConst *c;
    vector_foreach(c, &m->const_pool) {
        _write_rt_const(klc, c);
    }

    KlMachImport *imp;
    vector_foreach(imp, &m->import_table) {
        klc_add_import(klc, imp->kind, imp->path, imp->klass, imp->name);
    }

    BUF(buf);

    KlrFunc *fn;
    KlMachFunc *mach;
    vector_foreach(mach, &m->funcs) {
        fn = mach->origin;

        RESET_BUF(buf);

        Symbol *sym;
        int flags_;

        if (fn->klass) {
            buf_write_str(&buf, fn->klass->name);
            buf_write_char(&buf, '$');
            buf_write_str(&buf, fn->name);

            Symbol *_sym = stbl_get(stbl, fn->klass->name);
            ASSERT(_sym && _sym->kind == SYM_CLASS);
            KlassSymbol *kls_sym = (KlassSymbol *)_sym;
            sym = stbl_get(kls_sym->stbl, fn->name);

            flags_ = KLC_FLAGS_METH;
        } else {
            buf_write_str(&buf, fn->name);

            sym = stbl_get(stbl, fn->name);

            flags_ = 0;
        }

        ASSERT(sym && sym->kind == SYM_FUNC);
        FuncSymbol *fn_sym = (FuncSymbol *)sym;

        if (fn_sym->flags & SYM_FLAGS_PUBLIC) {
            flags_ |= KLC_FLAGS_PUB;
        }

        if (fn_sym->flags & SYM_FLAGS_NATIVE) {
            flags_ |= KLC_FLAGS_NATIVE;
        }

        int index = klc_add_code(klc, BUF_STR(buf), flags_, fn->nlocals, fn->max_call_args,
                                 mach->start_pc, mach->total_insns);
        ASSERT(index >= 1);
        ASSERT(index - 1 == mach->index);
        fn_sym->code_index = index - 1;
    }

    FINI_BUF(buf);

    klc_add_bytecodes(klc, m->codes.size, m->codes.data);
}

void write_to_klc(ParserModule *pm)
{
    KlcFile klc;
    init_klc_file(&klc, pm->path, pm->pkg_path);

    if (pm->m) {
        write_rt_data(pm->m->mach, &klc, pm->stbl);
        char *link;
        vector_foreach(link, &pm->links) {
            klc_add_link(&klc, link);
        }
    }

    write_meta(pm->stbl, &klc);

    write_klc_file(&klc);

    fini_klc_file(&klc);
}

#ifdef __cplusplus
}
#endif
