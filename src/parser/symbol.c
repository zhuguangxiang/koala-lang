/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2023 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "symbol.h"
#include "atom.h"
#include "buffer.h"
#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif

static Vector all_symbols = VECTOR_INIT_PTR;

static void add_to_global(void *_sym)
{
    Vector *vec = &all_symbols;
    Symbol *sym = _sym;
    sym->id = vector_size(vec);
    vector_push_back(vec, &sym);
}

void *get_symbol_by_id(int id)
{
    void **p = vector_get(&all_symbols, id);
    if (!p) return NULL;
    return *p;
}

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
        case SYM_TYPE_PARAM: {
            break;
        }
        case SYM_MODULE: {
            break;
        }
        case SYM_INSTANCE: {
            break;
        }
        default: {
            UNREACHABLE();
            break;
        }
    }

    mm_free(sym);
}

Symbol *stbl_add_var(HashMap *stbl, char *name, TypeSpec *ts, int flags)
{
    VarSymbol *sym = mm_alloc_obj(sym);
    hashmap_entry_init(sym, str_hash(name));
    sym->kind = SYM_VAR;
    sym->name = name;

    if (hashmap_put_absent(stbl, sym) < 0) {
        mm_free(sym);
        sym = NULL;
    } else {
        sym->ts = ts;
        sym->flags = flags;
        add_to_global(sym);
    }

    return (Symbol *)sym;
}

Symbol *stbl_add_func(HashMap *stbl, char *name, Vector *tps, TypeSpec *ret,
                      Vector *params, int flags, char *ann, char *ann_key)
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
        sym->params = params;
        sym->tps = tps;
        sym->ret = ret;
        sym->stbl = stbl_new();
        sym->ann = ann;
        sym->ann_key = ann_key;
        add_to_global(sym);
    }

    return (Symbol *)sym;
}

Symbol *stbl_add_klass(HashMap *stbl, char *name, int flags, int is_trait)
{
    KlassSymbol *sym = mm_alloc_obj(sym);
    hashmap_entry_init(sym, str_hash(name));
    sym->kind = is_trait ? SYM_TRAIT : SYM_CLASS;
    sym->name = name;

    if (hashmap_put_absent(stbl, sym) < 0) {
        mm_free(sym);
        sym = NULL;
    } else {
        sym->fields = vector_create_ptr();
        sym->funcs = vector_create_ptr();
        sym->protos = vector_create_ptr();
        sym->flags = flags;
        sym->stbl = stbl_new();
        add_to_global(sym);
    }

    return (Symbol *)sym;
}

Symbol *stbl_add_type_param(HashMap *stbl, char *name, Symbol *owner)
{
    TypeParamSymbol *sym = mm_alloc_obj(sym);
    hashmap_entry_init(sym, str_hash(name));
    sym->kind = SYM_TYPE_PARAM;
    sym->name = name;
    sym->owner = owner;

    if (hashmap_put_absent(stbl, sym) < 0) {
        mm_free(sym);
        sym = NULL;
    } else {
        sym->stbl = stbl_new();
        add_to_global(sym);
    }

#ifndef NOLOG
    if (sym) {
        log_info("add type_param('%s') OK", name);
    } else {
        log_info("add type_param('%s') failed", name);
    }
#endif

    return (Symbol *)sym;
}

Symbol *stbl_add_module(HashMap *stbl, char *path)
{
    ModuleSymbol *sym = mm_alloc_obj(sym);
    hashmap_entry_init(sym, str_hash(path));
    sym->kind = SYM_MODULE;
    sym->name = path;

    if (hashmap_put_absent(stbl, sym) < 0) {
        mm_free(sym);
        sym = NULL;
    } else {
        sym->stbl = stbl_new();
        add_to_global(sym);
    }
#ifndef NOLOG
    if (sym) {
        log_info("add module('%s') OK", path);
    } else {
        log_info("add module('%s') failed", path);
    }
#endif
    return (Symbol *)sym;
}

static char *mangle_type_name(char *base_name, Vector *tp_args)
{
    BUF(buf);
    buf_write_str(&buf, "_Z");
    buf_write_int64(&buf, strlen(base_name));
    buf_write_str(&buf, base_name);

    TypeSpec *ts;
    vector_foreach(ts, tp_args) {
        type_spec_to_str(ts, &buf);
    }

    char *mangled_name = atom_str(BUF_STR(buf));
    FINI_BUF(buf);
    return mangled_name;
}

static Symbol *stbl_add_instance(HashMap *stbl, Symbol *origin, Vector *tp_args)
{
    InstanceSymbol *sym = mm_alloc_obj(sym);
    char *mangled_name = mangle_type_name(origin->name, tp_args);
    hashmap_entry_init(sym, str_hash(mangled_name));
    sym->kind = SYM_INSTANCE;
    sym->name = mangled_name;

    if (hashmap_put_absent(stbl, sym) < 0) {
        mm_free(sym);
        sym = NULL;
    } else {
        add_to_global(sym); // get sym->id
        sym->ts = type_type_spec();
        sym->origin = origin;
        sym->tp_args = tp_args;
        sym->stbl = stbl_new();
        sym->instance_ts = klass_type_spec(NULL, mangled_name);
        sym->instance_ts->sym_id = sym->id;
        sym->instance_ts->checked = 1;
    }

    log_info("added instance symbol '%s' with id=%d", mangled_name, sym->id);

    return (Symbol *)sym;
}

static Symbol *instance_type_spec(HashMap *stbl, TypeSpec *ts, Vector *tp_args)
{
    ASSERT(ts->kind == TYPE_SPECIALIZED);

    Symbol *origin_sym = get_symbol_by_id(ts->sym_id);
    ASSERT(origin_sym->kind == SYM_CLASS || origin_sym->kind == SYM_TRAIT);

    Vector *base_tp_args = vector_create_ptr();
    TypeSpec *spec_arg_ts;
    TypeSpec *arg_ts;
    vector_foreach(arg_ts, ts->specialized.args) {
        if (!arg_ts) continue;
        if (arg_ts->kind == TYPE_GENERIC_VAR) {
            spec_arg_ts = vector_get_object(tp_args, arg_ts->generic_var.index);
        } else if (arg_ts->kind == TYPE_SPECIALIZED) {
            // nested specialized type
            Symbol *spec_arg_sym = instance_type_spec(stbl, arg_ts, tp_args);
            spec_arg_ts = ((InstanceSymbol *)spec_arg_sym)->instance_ts;
        } else {
            ASSERT(arg_ts->kind != TYPE_UNRESOLVED);
            spec_arg_ts = arg_ts;
        }
        vector_push_back(base_tp_args, &spec_arg_ts);
    }

    return find_or_add_instance(stbl, origin_sym, base_tp_args);
}

Symbol *find_or_add_instance(HashMap *stbl, Symbol *origin, Vector *tp_args)
{
    ASSERT(origin->kind == SYM_CLASS || origin->kind == SYM_TRAIT);

    char *mangled_name = mangle_type_name(origin->name, tp_args);
    Symbol *sym = stbl_get(stbl, mangled_name);
    if (sym) {
        log_info("found existing instance symbol '%s'", mangled_name);
        return sym;
    }

    sym = stbl_add_instance(stbl, origin, tp_args);

    InstanceSymbol *inst_sym = (InstanceSymbol *)sym;
    KlassSymbol *kls_sym = (KlassSymbol *)origin;

    // set instance bases
    if (!vector_empty(kls_sym->bases)) {
        log_info("updating instance bases for '%s'", mangled_name);
        inst_sym->bases = vector_create_ptr();
        TypeSpec *base_ts;
        vector_foreach(base_ts, kls_sym->bases) {
            if (!base_ts) continue;
            if (base_ts->kind == TYPE_KLASS) {
                vector_push_back(inst_sym->bases, &base_ts);
            } else {
                // handle specialized base class/trait
                ASSERT(base_ts->kind == TYPE_SPECIALIZED);
                // specialize base class/trait
                Symbol *base_sym = instance_type_spec(stbl, base_ts, tp_args);
                base_ts = ((InstanceSymbol *)base_sym)->instance_ts;
                vector_push_back(inst_sym->bases, &base_ts);
                ASSERT(base_ts->kind == TYPE_KLASS);
            }
            log_info("updated instance base '%s' for '%s'", base_ts->klass_type.name,
                     mangled_name);
        }
    }

    return sym;
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
                type_spec_print(var->ts, &buf);
                log_info("variable symbol: '%s', type: '%s'", sym->name, BUF_STR(buf));
                FINI_BUF(buf);
                break;
            }
            case SYM_FUNC: {
                FuncSymbol *fn = (FuncSymbol *)sym;
                BUF(buf);
                type_spec_print(fn->ts, &buf);
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
            case SYM_INSTANCE: {
                InstanceSymbol *inst = (InstanceSymbol *)sym;
                log_info("instance symbol: '%s'", sym->name);
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
