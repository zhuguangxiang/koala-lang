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
                        klc_add_int(&klc, lit->ival);
                    } else if (lit->which == LIT_FLT) {
                        klc_add_float(&klc, lit->fval);
                    } else if (lit->which == LIT_BOOL) {
                        klc_add_int(&klc, lit->bval);
                    } else if (lit->which == LIT_STR) {
                        klc_add_str(&klc, lit->sval, lit->len);
                    } else if (lit->which == LIT_NONE) {
                        klc_add_none(&klc);
                    } else {
                        UNREACHABLE();
                    }
                }
                BUF(buf);
                desc_to_str(var->desc, &buf);
                klc_add_var(&klc, var->name, BUF_STR(buf), def_val_idx, 0);
                FINI_BUF(buf);
                break;
            }
            case SYM_FUNC: {
                FuncSymbol *fn = (FuncSymbol *)sym;
                BUF(buf);
                desc_to_str(fn->ret, &buf);
                klc_add_func(&klc, fn->name, BUF_STR(buf), 0);
                FINI_BUF(buf);
                break;
            }
            case SYM_CLASS: {
                KlassSymbol *kls = (KlassSymbol *)sym;
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

    fini_klc_file(&klc);
    FINI_BUF(output);
}

#ifdef __cplusplus
}
#endif
