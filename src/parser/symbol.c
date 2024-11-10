/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2023 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "symbol.h"
#include "buffer.h"
#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif

void __symbol_free__(Symbol *sym, void *arg)
{
    switch (sym->kind) {
        case SYM_VAR: {
            VarSymbol *var = (VarSymbol *)sym;
            break;
        }
        case SYM_FUNC: {
            FuncSymbol *fn = (FuncSymbol *)sym;
            break;
        }
        case SYM_CLASS: {
            break;
        }
        case SYM_TRAIT: {
            break;
        }
        default: {
            UNREACHABLE();
            break;
        }
    }

    mm_free(sym);
}

Symbol *stbl_add_var(HashMap *stbl, char *name, TypeDesc *desc, int flags)
{
    VarSymbol *sym = mm_alloc_obj(sym);
    hashmap_entry_init(sym, str_hash(name));
    sym->kind = SYM_VAR;
    sym->name = name;

    if (hashmap_put_absent(stbl, sym) < 0) {
        mm_free(sym);
        sym = NULL;
    } else {
        sym->desc = desc;
        sym->flags = flags;
    }

#ifndef NOLOG
    BUF(buf);
    desc_print(desc, &buf);
    char *s = BUF_STR(buf);
    if (sym) {
        log_info("add var('%s' : '%s') OK", name, s ? s : "<NO-TYPE>");
    } else {
        log_info("add var('%s' : '%s') failed", name, s ? s : "<NO-TYPE>");
    }
    FINI_BUF(buf);
#endif

    return (Symbol *)sym;
}

Symbol *stbl_add_func(HashMap *stbl, char *name, Vector *tps, TypeDesc *ret,
                      Vector *params, int flags)
{
    FuncSymbol *sym = mm_alloc_obj(sym);
    hashmap_entry_init(sym, str_hash(name));
    sym->kind = SYM_FUNC;
    sym->name = name;

    if (hashmap_put_absent(stbl, sym) < 0) {
        mm_free(sym);
        sym = NULL;
    } else {
        sym->flags = flags;
        sym->ret = ret;
        sym->params = params;
        sym->tps = tps;
        sym->desc = desc_proto(ret, params);
        sym->stbl = stbl_new();
    }

#ifndef NOLOG
    BUF(buf);
    desc_print(sym->desc, &buf);
    char *s = BUF_STR(buf);
    if (sym) {
        log_info("add func('%s' : '%s') OK", name, s ? s : "<NO-TYPE>");
    } else {
        log_info("add func('%s' : '%s') failed", name, s ? s : "<NO-TYPE>");
    }
    FINI_BUF(buf);
#endif

    return (Symbol *)sym;
}

Symbol *stbl_add_klass(HashMap *stbl, char *name, Vector *tps, Vector *bases, int flags)
{
    KlassSymbol *sym = mm_alloc_obj(sym);
    hashmap_entry_init(sym, str_hash(name));
    sym->kind = SYM_CLASS;
    sym->name = name;

    if (hashmap_put_absent(stbl, sym) < 0) {
        mm_free(sym);
        sym = NULL;
    } else {
        sym->flags = flags;
        sym->bases = bases;
        sym->tps = tps;
        sym->stbl = stbl_new();
    }

#ifndef NOLOG
    BUF(buf);
    desc_print(sym->desc, &buf);
    char *s = BUF_STR(buf);
    if (sym) {
        log_info("add class('%s') OK", name);
    } else {
        log_info("add class('%s') failed", name);
    }
    FINI_BUF(buf);
#endif

    return (Symbol *)sym;
}

Symbol *stbl_add_trait(HashMap *stbl, char *name, Vector *tps, Vector *bases, int flags)
{
    KlassSymbol *sym = mm_alloc_obj(sym);
    hashmap_entry_init(sym, str_hash(name));
    sym->kind = SYM_TRAIT;
    sym->name = name;

    if (hashmap_put_absent(stbl, sym) < 0) {
        mm_free(sym);
        sym = NULL;
    } else {
        sym->flags = flags;
        sym->bases = bases;
        sym->tps = tps;
        sym->stbl = stbl_new();
    }

#ifndef NOLOG
    BUF(buf);
    desc_print(sym->desc, &buf);
    char *s = BUF_STR(buf);
    if (sym) {
        log_info("add trait('%s') OK", name);
    } else {
        log_info("add trait('%s') failed", name);
    }
    FINI_BUF(buf);
#endif

    return (Symbol *)sym;
}

Symbol *stbl_get(HashMap *stbl, char *name)
{
    if (stbl == NULL) return NULL;
    Symbol key = { .name = name };
    hashmap_entry_init(&key, str_hash(name));
    return hashmap_get(stbl, &key);
}

void stbl_show(HashMap *stbl)
{
    HashMapIter it = { 0 };
    while (hashmap_next(stbl, &it)) {
        Symbol *sym = (Symbol *)it.entry;
        switch (sym->kind) {
            case SYM_VAR: {
                VarSymbol *var = (VarSymbol *)sym;
                BUF(buf);
                desc_print(var->desc, &buf);
                log_info("variable symbol: '%s', type: '%s'", sym->name, BUF_STR(buf));
                FINI_BUF(buf);
                break;
            }
            case SYM_FUNC: {
                FuncSymbol *fn = (FuncSymbol *)sym;
                BUF(buf);
                desc_print(fn->ret, &buf);
                log_info("function symbol: '%s', ret-type: '%s'", sym->name,
                         BUF_STR(buf));
                FINI_BUF(buf);
                break;
            }
            case SYM_CLASS: {
                KlassSymbol *kls = (KlassSymbol *)sym;
                log_info("class symbol: '%s'", sym->name);
                break;
            }
            case SYM_TRAIT: {
                KlassSymbol *kls = (KlassSymbol *)sym;
                log_info("trait symbol: '%s'", sym->name);
                break;
            }
            default: {
                UNREACHABLE();
                break;
            }
        }
    }
}

#ifdef __cplusplus
}
#endif
