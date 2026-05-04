/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "atom.h"
#include "cgen.h"
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

static void write_meta(HashMap *stbl, KlcFile *klc)
{
    HashMapIter it = { 0 };
    while (hashmap_next(stbl, &it)) {
        Symbol *sym = (Symbol *)it.entry;
        switch (sym->kind) {
            case SYM_VAR: {
                VarSymbol *var = (VarSymbol *)sym;
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
                break;
            }
            case SYM_FUNC: {
                FuncSymbol *fn = (FuncSymbol *)sym;

                int flags = 0;
                if (fn->flags & SYM_FLAGS_PUBLIC) {
                    flags |= KLC_FLAGS_PUB;
                }

                KlcFunc *f = klc_add_func(klc, fn->name, fn->ret->signature, flags);

                // add argument info
                ArgInfo *item;
                vector_foreach(item, fn->params) {
                    if (!item) continue;
                    ASSERT(item->sym->kind == SYM_VAR);
                    VarSymbol *var_sym = (VarSymbol *)item->sym;
                    ASSERT(var_sym->scope == VAR_SCOPE_PARAM);
                    uint16_t dfl_val_idx = 0;
                    if (var_sym->lit) {
                        // has default value
                        dfl_val_idx = klc_add_const(klc, var_sym->lit);
                    }
                    klc_func_add_arg(f, item->name, item->ts->signature, dfl_val_idx);
                }

                // add annotations
                if (fn->ann) {
                    klc_func_add_ann(f, fn->ann, fn->ann_key, NULL);
                }
                break;
            }
            case SYM_CLASS: {
                KlassSymbol *kls = (KlassSymbol *)sym;

                int flags = 0;
                if (kls->flags & SYM_FLAGS_PUBLIC) {
                    flags |= KLC_FLAGS_PUB;
                }

                KlcKlass *klass = klc_add_klass(klc, kls->name, flags);

                if (vector_size(&kls->tps) > 0) {
                    TypeParamSymbol *tp;
                    vector_foreach(tp, &kls->tps) {
                        if (!tp) continue;

                        KlcTypeParam *klc_tp = klc_klass_add_tp(klass, tp->name);
                        klc_tp->which = (int8_t)tp->which;

                        if (vector_size(&tp->bound) > 0) {
                            TypeSpec *ts;
                            vector_foreach(ts, &tp->bound) {
                                if (!ts) continue;
                                uint16_t index =
                                    klc_add_str(klass->filp, ts->signature, strlen(ts->signature));
                                vector_push_back(&klc_tp->bounds, &index);
                            }
                        }
                    }
                }

                if (vector_size(&kls->bases) > 0) {
                    TypeSpec *ts;
                    vector_foreach(ts, &kls->bases) {
                        if (!ts) continue;
                        uint16_t index =
                            klc_add_str(klass->filp, ts->signature, strlen(ts->signature));
                        vector_push_back(&klass->bases, &index);
                    }
                }

                TypeSpec *ts;
                vector_foreach(ts, &kls->pip) {
                    if (!ts) continue;
                    uint16_t index =
                        klc_add_str(klass->filp, ts->signature, strlen(ts->signature));
                    vector_push_back(&klass->pip, &index);
                }

                vector_foreach(ts, &kls->lro) {
                    if (!ts) continue;
                    uint16_t index =
                        klc_add_str(klass->filp, ts->signature, strlen(ts->signature));
                    vector_push_back(&klass->lro, &index);
                }

                VarSymbol *field;
                vector_foreach(field, kls->fields) {
                    if (!field) continue;
                    int flags_ = 0;
                    if (field->flags & SYM_FLAGS_MUTABLE) {
                        flags_ |= KLC_FLAGS_MUT;
                    }

                    if (field->flags & SYM_FLAGS_PUBLIC) {
                        flags_ |= KLC_FLAGS_PUB;
                    }

                    klc_klass_add_field(klass, field->name, field->ts->signature, flags_);
                }

                FuncSymbol *fn;
                KlcFunc *klc_fn;
                vector_foreach(fn, kls->funcs) {
                    if (!fn) continue;

                    int flags_ = 0;
                    if (fn->flags & SYM_FLAGS_PUBLIC) {
                        flags_ |= KLC_FLAGS_PUB;
                    }

                    klc_fn = klc_klass_add_func(klass, fn->name, fn->ret->signature, flags_);

                    if (vector_size(&fn->tps) > 0) {
                        TypeParamSymbol *tp;
                        vector_foreach(tp, &fn->tps) {
                            if (!tp) continue;

                            KlcTypeParam *klc_tp = klc_func_add_tp(klc_fn, tp->name);
                            klc_tp->which = (int8_t)tp->which;

                            if (vector_size(&tp->bound) > 0) {
                                TypeSpec *ts;
                                vector_foreach(ts, &tp->bound) {
                                    if (!ts) continue;
                                    uint16_t index = klc_add_str(klc_fn->filp, ts->signature,
                                                                 strlen(ts->signature));
                                    vector_push_back(&klc_tp->bounds, &index);
                                }
                            }
                        }
                    }

                    // add argument info
                    ArgInfo *item;
                    vector_foreach(item, fn->params) {
                        if (!item) continue;
                        ASSERT(item->sym->kind == SYM_VAR);
                        VarSymbol *var_sym = (VarSymbol *)item->sym;
                        ASSERT(var_sym->scope == VAR_SCOPE_PARAM);
                        uint16_t dfl_val_idx = 0;
                        if (var_sym->lit) {
                            // has default value
                            dfl_val_idx = klc_add_const(klc, var_sym->lit);
                        }
                        klc_func_add_arg(klc_fn, item->name, item->ts->signature, dfl_val_idx);
                    }

                    // add annotations
                    if (fn->ann) {
                        klc_func_add_ann(klc_fn, fn->ann, fn->ann_key, NULL);
                    }
                }
                break;
            }
            case SYM_TRAIT: {
                KlassSymbol *kls = (KlassSymbol *)sym;

                int flags = KLC_FLAGS_PUB | KLC_FLAGS_TRAIT;

                KlcKlass *klass = klc_add_klass(klc, kls->name, flags);

                if (vector_size(&kls->tps) > 0) {
                    TypeParamSymbol *tp;
                    vector_foreach(tp, &kls->tps) {
                        if (!tp) continue;
                        KlcTypeParam *klc_tp = klc_klass_add_tp(klass, tp->name);

                        if (vector_size(&tp->bound) > 0) {
                            TypeSpec *ts;
                            vector_foreach(ts, &tp->bound) {
                                if (!ts) continue;
                                uint16_t index =
                                    klc_add_str(klass->filp, ts->signature, strlen(ts->signature));
                                vector_push_back(&klc_tp->bounds, &index);
                            }
                        }
                    }
                }

                if (vector_size(&kls->bases) > 0) {
                    TypeSpec *ts;
                    vector_foreach(ts, &kls->bases) {
                        if (!ts) continue;
                        uint16_t index =
                            klc_add_str(klass->filp, ts->signature, strlen(ts->signature));
                        vector_push_back(&klass->bases, &index);
                    }
                }

                TypeSpec *ts;
                vector_foreach(ts, &kls->pip) {
                    if (!ts) continue;
                    uint16_t index =
                        klc_add_str(klass->filp, ts->signature, strlen(ts->signature));
                    vector_push_back(&klass->pip, &index);
                }

                vector_foreach(ts, &kls->lro) {
                    if (!ts) continue;
                    uint16_t index =
                        klc_add_str(klass->filp, ts->signature, strlen(ts->signature));
                    vector_push_back(&klass->lro, &index);
                }

                FuncSymbol *fn;
                KlcFunc *klc_fn;
                vector_foreach(fn, kls->funcs) {
                    if (!fn) continue;

                    int flags_ = KLC_FLAGS_TRAIT;
                    if (fn->flags & SYM_FLAGS_PUBLIC) {
                        flags_ |= KLC_FLAGS_PUB;
                    }

                    klc_fn = klc_klass_add_func(klass, fn->name, fn->ret->signature, flags_);

                    // add argument info
                    ArgInfo *item;
                    vector_foreach(item, fn->params) {
                        if (!item) continue;
                        ASSERT(item->sym->kind == SYM_VAR);
                        VarSymbol *var_sym = (VarSymbol *)item->sym;
                        ASSERT(var_sym->scope == VAR_SCOPE_PARAM);
                        uint16_t dfl_val_idx = 0;
                        if (var_sym->lit) {
                            // has default value
                            dfl_val_idx = klc_add_const(klc, var_sym->lit);
                        }
                        klc_func_add_arg(klc_fn, item->name, item->ts->signature, dfl_val_idx);
                    }

                    // add annotations
                    if (fn->ann) {
                        klc_func_add_ann(klc_fn, fn->ann, fn->ann_key, NULL);
                    }
                }
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
}

static uint16_t _write_rt_const(KlcFile *klc, KlMachConst *kc)
{
    switch (kc->tag) {
        case KL_MACH_CONST_INT:
            return klc_add_rt_int(klc, kc->i64, 1, kc->len);
        case KL_MACH_CONST_UINT:
            return klc_add_rt_int(klc, kc->u64, 0, kc->len);
        case KL_MACH_CONST_FLOAT:
            return klc_add_rt_float(klc, kc->f64, kc->len);
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
        case KL_MACH_CONST_RANGE:
            Vector *list = vector_create(sizeof(uint16_t));
            Vector *vec = kc->list;
            KlMachConst *range_item;
            vector_foreach(range_item, vec) {
                if (!range_item) continue;
                uint16_t idx = _write_rt_const(klc, range_item);
                vector_push_back(list, &idx);
            }
            return klc_add_rt_range(klc, list);
        default:
            UNREACHABLE();
    }
}

static void write_rt_data(KlMachModule *m, KlcFile *klc)
{
    klc->num_rt_consts = vector_size(&m->const_pool);

    KlMachConst *c;
    vector_foreach(c, &m->const_pool) {
        _write_rt_const(klc, c);
    }

    KlMachImport *imp;
    vector_foreach(imp, &m->import_table) {
        klc_add_import(klc, imp->kind, imp->path, imp->name);
    }

    KlrFunc *fn;
    KlMachFunc *mach;
    vector_foreach(mach, &m->funcs) {
        fn = mach->origin;
        klc_add_code(klc, fn->name, fn->nlocals, fn->max_call_args, mach->start_pc,
                     mach->total_insns);
    }

    klc_add_bytecodes(klc, m->codes.size, m->codes.data);
}

void write_to_klc(ParserModule *pm)
{
    KlcFile klc;
    init_klc_file(&klc, pm->path);

    write_meta(pm->stbl, &klc);

    if (pm->module) {
        write_rt_data(pm->module->mach, &klc);
    }

    write_klc_file(&klc);

    fini_klc_file(&klc);
}

#ifdef __cplusplus
}
#endif
