/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "atom.h"
#include "klc.h"
#include "log.h"
#include "mm.h"
#include "parser.h"

#ifdef __cplusplus
extern "C" {
#endif

void kl_emit_func(ParserState *ps, KlrFunc *fn, KlcFunc *klc_fn);

static uint16_t klc_add_const(KlcFile *klc, Literal *lit)
{
    uint16_t index = 0;
    switch (lit->which) {
        case LIT_INT: {
            index = klc_add_int(klc, lit->ival, lit->sign, lit->len);
            break;
        }
        case LIT_FLT: {
            index = klc_add_float(klc, lit->fval);
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

void kl_write_to_klc(ParserState *ps)
{
    HashMap *stbl = ps->stbl;

    BUF(output);
    buf_write_str(&output, ps->filename);
    buf_write_char(&output, 'c');

    KlcFile klc;
    init_klc_file(&klc, BUF_STR(output));

    HashMapIter it = { 0 };
    while (hashmap_next(stbl, &it)) {
        Symbol *sym = (Symbol *)it.entry;
        switch (sym->kind) {
            case SYM_VAR: {
                VarSymbol *var = (VarSymbol *)sym;
                uint16_t def_val_idx = 0;
                Literal *lit = var->lit;
                if (lit) {
                    def_val_idx = klc_add_const(&klc, lit);
                }

                int flags = 0;
                if (var->flags & SYM_FLAGS_MUTABLE) {
                    flags |= KLC_FLAGS_MUTABLE;
                }
                if (var->flags & SYM_FLAGS_PUBLIC) {
                    flags |= KLC_FLAGS_PUB;
                }

                klc_add_var(&klc, var->name, var->ts->signature, def_val_idx, flags);
                break;
            }
            case SYM_FUNC: {
                FuncSymbol *fn = (FuncSymbol *)sym;

                int flags = 0;
                if (fn->flags & SYM_FLAGS_PUBLIC) {
                    flags |= KLC_FLAGS_PUB;
                }

                KlcFunc *f = klc_add_func(&klc, fn->name, fn->ret->signature, flags);

                // add argument info
                ArgInfo **item_p;
                ArgInfo *item;
                vector_foreach(item_p, fn->params) {
                    item = *item_p;
                    ASSERT(item->sym->kind == SYM_VAR);
                    VarSymbol *var_sym = (VarSymbol *)item->sym;
                    ASSERT(var_sym->scope == VAR_SCOPE_PARAM);
                    uint16_t def_val_idx = 0;
                    if (var_sym->lit) {
                        // has default value
                        def_val_idx = klc_add_const(&klc, var_sym->lit);
                    }
                    klc_func_add_arg(f, item->name, item->ts->signature, def_val_idx);
                }

                // add annotations
                if (fn->ann) {
                    klc_func_add_ann(f, fn->ann, fn->ann_key, NULL);
                }

                // add byte codes
                if (fn->ir_val) {
                    printf("byte codes: %p\n", fn->ir_val);
                    kl_emit_func(ps, (KlrFunc *)fn->ir_val, f);
                }
                break;
            }
            case SYM_CLASS: {
                KlassSymbol *kls = (KlassSymbol *)sym;

                int flags = 0;
                if (kls->flags & SYM_FLAGS_PUBLIC) {
                    flags |= KLC_FLAGS_PUB;
                }

                KlcKlass *klass = klc_add_klass(&klc, kls->name, flags);

                if (vector_size(kls->tps) > 0) {
                    TypeParamSymbol *tp;
                    vector_foreach_object(tp, kls->tps)
                    {
                        KlcTypeParam *klc_tp = klc_klass_add_tp(klass, tp->name);

                        if (vector_size(tp->bound) > 0) {
                            TypeSpec *ts;
                            vector_foreach_object(ts, tp->bound)
                            {
                                uint16_t index = klc_add_str(klass->filp, ts->signature,
                                                             strlen(ts->signature));
                                vector_push_back(&klc_tp->bounds, &index);
                            }
                        }
                    }
                }

                if (vector_size(kls->bases) > 0) {
                    TypeSpec *ts;
                    vector_foreach_object(ts, kls->bases)
                    {
                        uint16_t index = klc_add_str(klass->filp, ts->signature,
                                                     strlen(ts->signature));
                        vector_push_back(&klass->bases, &index);
                    }
                }

                FuncSymbol **fn_p;
                FuncSymbol *fn;
                KlcFunc *klc_fn;
                vector_foreach(fn_p, kls->funcs) {
                    fn = *fn_p;

                    int flags_ = 0;
                    if (fn->flags & SYM_FLAGS_PUBLIC) {
                        flags_ |= KLC_FLAGS_PUB;
                    }

                    klc_fn =
                        klc_klass_add_func(klass, fn->name, fn->ret->signature, flags_);

                    // add argument info
                    ArgInfo **item_p;
                    ArgInfo *item;
                    vector_foreach(item_p, fn->params) {
                        item = *item_p;
                        ASSERT(item->sym->kind == SYM_VAR);
                        VarSymbol *var_sym = (VarSymbol *)item->sym;
                        ASSERT(var_sym->scope == VAR_SCOPE_PARAM);
                        uint16_t def_val_idx = 0;
                        if (var_sym->lit) {
                            // has default value
                            def_val_idx = klc_add_const(&klc, var_sym->lit);
                        }
                        klc_func_add_arg(klc_fn, item->name, item->ts->signature,
                                         def_val_idx);
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

                KlcKlass *klass = klc_add_klass(&klc, kls->name, flags);

                if (vector_size(kls->tps) > 0) {
                    TypeParamSymbol *tp;
                    vector_foreach_object(tp, kls->tps)
                    {
                        KlcTypeParam *klc_tp = klc_klass_add_tp(klass, tp->name);

                        if (vector_size(tp->bound) > 0) {
                            TypeSpec *ts;
                            vector_foreach_object(ts, tp->bound)
                            {
                                uint16_t index = klc_add_str(klass->filp, ts->signature,
                                                             strlen(ts->signature));
                                vector_push_back(&klc_tp->bounds, &index);
                            }
                        }
                    }
                }

                if (vector_size(kls->bases) > 0) {
                    TypeSpec *ts;
                    vector_foreach_object(ts, kls->bases)
                    {
                        uint16_t index = klc_add_str(klass->filp, ts->signature,
                                                     strlen(ts->signature));
                        vector_push_back(&klass->bases, &index);
                    }
                }

                FuncSymbol **fn_p;
                FuncSymbol *fn;
                KlcFunc *klc_fn;
                vector_foreach(fn_p, kls->funcs) {
                    fn = *fn_p;

                    int flags_ = KLC_FLAGS_TRAIT;
                    if (fn->flags & SYM_FLAGS_PUBLIC) {
                        flags_ |= KLC_FLAGS_PUB;
                    }

                    klc_fn =
                        klc_klass_add_func(klass, fn->name, fn->ret->signature, flags_);

                    // add argument info
                    ArgInfo **item_p;
                    ArgInfo *item;
                    vector_foreach(item_p, fn->params) {
                        item = *item_p;
                        ASSERT(item->sym->kind == SYM_VAR);
                        VarSymbol *var_sym = (VarSymbol *)item->sym;
                        ASSERT(var_sym->scope == VAR_SCOPE_PARAM);
                        uint16_t def_val_idx = 0;
                        if (var_sym->lit) {
                            // has default value
                            def_val_idx = klc_add_const(&klc, var_sym->lit);
                        }
                        klc_func_add_arg(klc_fn, item->name, item->ts->signature,
                                         item->dfl_val_idx);
                    }

                    // add annotations
                    if (fn->ann) {
                        klc_func_add_ann(klc_fn, fn->ann, fn->ann_key, NULL);
                    }
                }
                break;
            }
            default: {
                UNREACHABLE();
                break;
            }
        }
    }

    write_klc_file(&klc);

    klc_dump(&klc);

    fini_klc_file(&klc);

    printf("read klc file: %s\n", BUF_STR(output));

    KlcFile klc2;
    init_klc_file(&klc2, BUF_STR(output));
    read_klc_file(&klc2, 1);
    klc_dump(&klc2);

    FINI_BUF(output);
}

#ifdef __cplusplus
}
#endif
