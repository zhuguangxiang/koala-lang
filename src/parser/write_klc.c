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
                    if (lit->which == LIT_INT) {
                        def_val_idx = klc_add_int(&klc, lit->ival, lit->sign, lit->len);
                        // } else if (lit->which == LIT_FLT) {
                        //     def_val_idx = klc_add_float(&klc, lit->fval);
                        // } else if (lit->which == LIT_BOOL) {
                        //     def_val_idx = klc_add_int(&klc, lit->bval);
                        // } else if (lit->which == LIT_STR) {
                        //     def_val_idx = klc_add_str(&klc, lit->sval, lit->len);
                        // } else if (lit->which == LIT_NONE) {
                        //     def_val_idx = klc_add_none(&klc);
                    } else {
                        UNREACHABLE();
                    }
                }

                int flags = 0;
                if (var->flags & SYM_FLAGS_MUTABLE) {
                    flags |= KLC_FLAGS_MUTABLE;
                }
                if (var->flags & SYM_FLAGS_PUBLIC) {
                    flags |= KLC_FLAGS_PUB;
                }

                BUF(buf);
                type_spec_to_str(var->ts, &buf);
                klc_add_var(&klc, var->name, BUF_STR(buf), def_val_idx, flags);
                FINI_BUF(buf);
                break;
            }
            case SYM_FUNC: {
                FuncSymbol *fn = (FuncSymbol *)sym;

                int flags = 0;
                if (fn->flags & SYM_FLAGS_PUBLIC) {
                    flags |= KLC_FLAGS_PUB;
                }

                BUF(buf);

                type_spec_to_str(fn->ts, &buf);
                KlcFunc *f = klc_add_func(&klc, fn->name, BUF_STR(buf), flags);

                // add argument info
                ArgInfo **item_p;
                ArgInfo *item;
                vector_foreach(item_p, fn->params) {
                    RESET_BUF(buf);
                    item = *item_p;
                    type_spec_to_str(item->ts, &buf);
                    klc_func_add_arg(f, item->name, BUF_STR(buf), item->dfl_val_idx);
                }

                FINI_BUF(buf);

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

                if (kls->flags & SYM_FLAGS_FINAL) {
                    flags |= KLC_FLAGS_FINAL;
                }

                KlcKlass *klass = klc_add_klass(&klc, kls->name, flags);

                if (vector_size(kls->tps) > 0) {
                    TypeParamSymbol *tp;
                    vector_foreach_object(tp, kls->tps)
                    {
                        KlcTypeParam *klc_tp = klc_klass_add_tp(klass, tp->name);

                        if (vector_size(tp->bound) > 0) {
                            BUF(buf);
                            TypeSpec *ts;
                            vector_foreach_object(ts, tp->bound)
                            {
                                type_spec_to_str(ts, &buf);
                                uint16_t index =
                                    klc_add_str(klass->filp, BUF_STR(buf), BUF_LEN(buf));
                                vector_push_back(&klc_tp->bounds, &index);
                                RESET_BUF(buf);
                            }
                            FINI_BUF(buf);
                        }
                    }
                }

                if (vector_size(kls->bases) > 0) {
                    BUF(buf);
                    TypeSpec *ts;
                    vector_foreach_object(ts, kls->bases)
                    {
                        type_spec_to_str(ts, &buf);
                        uint16_t index =
                            klc_add_str(klass->filp, BUF_STR(buf), BUF_LEN(buf));
                        vector_push_back(&klass->bases, &index);
                        RESET_BUF(buf);
                    }
                    FINI_BUF(buf);
                }

                FuncSymbol **fn_p;
                FuncSymbol *fn;
                KlcFunc *klc_fn;
                BUF(buf);
                vector_foreach(fn_p, kls->funcs) {
                    fn = *fn_p;

                    int flags_ = 0;
                    if (fn->flags & SYM_FLAGS_PUBLIC) {
                        flags_ |= KLC_FLAGS_PUB;
                    }

                    type_spec_to_str(fn->ts, &buf);
                    klc_fn = klc_klass_add_func(klass, fn->name, BUF_STR(buf), flags_);

                    // add argument info
                    ArgInfo **item_p;
                    ArgInfo *item;
                    vector_foreach(item_p, fn->params) {
                        RESET_BUF(buf);
                        item = *item_p;
                        type_spec_to_str(item->ts, &buf);
                        klc_func_add_arg(klc_fn, item->name, BUF_STR(buf),
                                         item->dfl_val_idx);
                    }

                    // add annotations
                    if (fn->ann) {
                        klc_func_add_ann(klc_fn, fn->ann, fn->ann_key, NULL);
                    }
                    RESET_BUF(buf);
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
                            BUF(buf);
                            TypeSpec *ts;
                            vector_foreach_object(ts, tp->bound)
                            {
                                type_spec_to_str(ts, &buf);
                                uint16_t index =
                                    klc_add_str(klass->filp, BUF_STR(buf), BUF_LEN(buf));
                                vector_push_back(&klc_tp->bounds, &index);
                                RESET_BUF(buf);
                            }
                            FINI_BUF(buf);
                        }
                    }
                }

                if (vector_size(kls->bases) > 0) {
                    BUF(buf);
                    TypeSpec *ts;
                    vector_foreach_object(ts, kls->bases)
                    {
                        type_spec_to_str(ts, &buf);
                        uint16_t index =
                            klc_add_str(klass->filp, BUF_STR(buf), BUF_LEN(buf));
                        vector_push_back(&klass->bases, &index);
                        RESET_BUF(buf);
                    }
                    FINI_BUF(buf);
                }

                FuncSymbol **fn_p;
                FuncSymbol *fn;
                KlcFunc *klc_fn;
                BUF(buf);
                vector_foreach(fn_p, kls->funcs) {
                    fn = *fn_p;

                    int flags_ = KLC_FLAGS_TRAIT;
                    if (fn->flags & SYM_FLAGS_PUBLIC) {
                        flags_ |= KLC_FLAGS_PUB;
                    }

                    type_spec_to_str(fn->ts, &buf);
                    klc_fn = klc_klass_add_func(klass, fn->name, BUF_STR(buf), flags_);

                    // add argument info
                    ArgInfo **item_p;
                    ArgInfo *item;
                    vector_foreach(item_p, fn->params) {
                        RESET_BUF(buf);
                        item = *item_p;
                        type_spec_to_str(item->ts, &buf);
                        klc_func_add_arg(klc_fn, item->name, BUF_STR(buf),
                                         item->dfl_val_idx);
                    }

                    // add annotations
                    if (fn->ann) {
                        klc_func_add_ann(klc_fn, fn->ann, fn->ann_key, NULL);
                    }
                    RESET_BUF(buf);
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
