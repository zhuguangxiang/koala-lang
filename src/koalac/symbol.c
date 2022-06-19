/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2018-2022 James <zhuguangxiang@gmail.com>
 */

#include "symbol.h"

#ifdef __cplusplus
extern "C" {
#endif

static int symbol_equal(void *k1, void *k2)
{
    Symbol *s1 = k1;
    Symbol *s2 = k2;
    return !strcmp(s1->name, s2->name);
}

HashMap *stbl_new(void)
{
    HashMap *stbl = mm_alloc_obj(stbl);
    hashmap_init(stbl, symbol_equal);
    return stbl;
}

static void symbol_free(void *e, void *arg)
{
    UNUSED(arg);
    Symbol *sym = e;
    mm_free(sym);
}

void stbl_free(HashMap *stbl)
{
    if (!stbl) return;
    hashmap_fini(stbl, symbol_free, NULL);
    mm_free(stbl);
}

void free_symbol(Symbol *sym)
{
    Symbol key = { .name = sym->name };
    hashmap_entry_init(&key, str_hash(sym->name));
    hashmap_remove(sym->parent, &key);
    symbol_free(sym, NULL);
}

Symbol *stbl_add_let(HashMap *stbl, char *name, TypeDesc *desc)
{
    VarSymbol *sym = mm_alloc(sizeof(*sym));
    hashmap_entry_init(sym, str_hash(name));
    sym->kind = SYM_LET;
    sym->name = name;
    sym->type = desc;
    sym->parent = stbl;
    if (hashmap_put_absent(stbl, sym) < 0) {
        mm_free(sym);
        sym = NULL;
    }
    return (Symbol *)sym;
}

Symbol *stbl_add_var(HashMap *stbl, char *name, TypeDesc *desc)
{
    VarSymbol *sym = mm_alloc(sizeof(*sym));
    hashmap_entry_init(sym, str_hash(name));
    sym->kind = SYM_VAR;
    sym->name = name;
    sym->type = desc;
    sym->parent = stbl;
    if (hashmap_put_absent(stbl, sym) < 0) {
        mm_free(sym);
        sym = NULL;
    }
    return (Symbol *)sym;
}

Symbol *stbl_add_func(HashMap *stbl, char *name)
{
    FuncSymbol *sym = mm_alloc(sizeof(*sym));
    hashmap_entry_init(sym, str_hash(name));
    sym->kind = SYM_FUNC;
    sym->name = name;
    sym->parent = stbl;
    if (hashmap_put_absent(stbl, sym) < 0) {
        mm_free(sym);
        sym = NULL;
    } else {
        sym->stbl = stbl_new();
    }
    return (Symbol *)sym;
}

Symbol *stbl_add_param_type(HashMap *stbl, char *name)
{
    ParamTypeSymbol *sym = mm_alloc(sizeof(*sym));
    hashmap_entry_init(sym, str_hash(name));
    sym->kind = SYM_PARAM_TYPE;
    sym->name = name;
    sym->parent = stbl;
    if (hashmap_put_absent(stbl, sym) < 0) {
        mm_free(sym);
        sym = NULL;
    }
    return (Symbol *)sym;
}

Symbol *stbl_add_package(HashMap *stbl, char *path, char *name)
{
    PackSymbol *sym = mm_alloc(sizeof(*sym));
    hashmap_entry_init(sym, str_hash(name));
    sym->kind = SYM_PACKAGE;
    sym->path = path;
    sym->name = name;
    sym->parent = stbl;
    if (hashmap_put_absent(stbl, sym) < 0) {
        mm_free(sym);
        sym = NULL;
    } else {
        sym->stbl = stbl_new();
    }
    return (Symbol *)sym;
}

Symbol *stbl_get(HashMap *stbl, char *name)
{
    if (!stbl) return NULL;
    Symbol key = { .name = name };
    hashmap_entry_init(&key, str_hash(name));
    return hashmap_get(stbl, &key);
}

#ifdef __cplusplus
}
#endif
