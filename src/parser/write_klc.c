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

                desc_to_str(fn->desc, &buf);
                KlcFunc *f = klc_add_func(&klc, fn->name, BUF_STR(buf), flags);

                // add argument info
                ArgInfo **item_p;
                ArgInfo *item;
                vector_foreach(item_p, fn->params) {
                    RESET_BUF(buf);
                    item = *item_p;
                    desc_to_str(item->desc, &buf);
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
                KlcKlass *klass = klc_add_klass(&klc, kls->name, 0);
                FuncSymbol **fn_p;
                FuncSymbol *fn;
                vector_foreach(fn_p, kls->funcs) {
                    fn = *fn_p;
                    printf("%s\n", fn->name);
                }
                break;
            }
            case SYM_TRAIT: {
                KlassSymbol *kls = (KlassSymbol *)sym;
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
