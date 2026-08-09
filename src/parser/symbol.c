/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
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
    void *p = vector_get(&all_symbols, id);
    if (!p) return NULL;
    return p;
}

static void __free_inner_stbl(Symbol *sym)
{
    if (!sym) return;

    switch (sym->kind) {
        case SYM_VAR: {
            ASSERT(!sym->stbl);
            break;
        }
        case SYM_FUNC: {
            stbl_free(sym->stbl);
            sym->stbl = NULL;
            break;
        }
        case SYM_CLASS:
        case SYM_TRAIT: {
            stbl_free(sym->stbl);
            sym->stbl = NULL;
            break;
        }
        case SYM_TYPE_PARAM: {
            ASSERT(!sym->stbl);
            break;
        }
        case SYM_PACKAGE: {
            stbl_free(sym->stbl);
            sym->stbl = NULL;
            break;
        }
        case SYM_INSTANCE: {
            stbl_free(sym->stbl);
            sym->stbl = NULL;
            break;
        }
        case SYM_INSTANCE_FUNC: {
            stbl_free(sym->stbl);
            sym->stbl = NULL;
            break;
        }
        case SYM_SHADOW_VAR: {
            // nothing
            ASSERT(!sym->stbl);
            break;
        }
        case SYM_INHERITED: {
            // nothing
            ASSERT(!sym->stbl);
            break;
        }
        case SYM_IMPORTED: {
            // nothing
            ASSERT(!sym->stbl);
            break;
        }
        default: {
            UNREACHABLE();
            break;
        }
    }
}

static void __symbol_free(Symbol *sym)
{
    if (!sym) return;
    switch (sym->kind) {
        case SYM_VAR: {
            VarSymbol *var = (VarSymbol *)sym;
            if (var->lit) mm_free(var->lit);
            ASSERT(!sym->stbl);
            break;
        }
        case SYM_FUNC: {
            FuncSymbol *fn = (FuncSymbol *)sym;

            vector_fini(&fn->tps);

            ArgInfo *arg;
            vector_foreach(arg, fn->params) {
                if (!arg) continue;
                mm_free(arg);
            }
            vector_destroy(fn->params);

            ASSERT(!sym->stbl);
            break;
        }
        case SYM_CLASS:
        case SYM_TRAIT: {
            KlassSymbol *kls = (KlassSymbol *)sym;

            vector_fini(&kls->tps);

            vector_fini(&kls->bases);
            vector_destroy(kls->fields);
            vector_destroy(kls->funcs);

            vector_fini(&kls->pip);
            vector_fini(&kls->lro);
            vector_fini(&kls->scm);
            vector_fini(&kls->intf_table);

            ASSERT(!sym->stbl);
            break;
        }
        case SYM_TYPE_PARAM: {
            TypeParamSymbol *tp = (TypeParamSymbol *)sym;
            vector_fini(&tp->bound);
            break;
        }
        case SYM_PACKAGE: {
            ASSERT(!sym->stbl);
            break;
        }
        case SYM_INSTANCE: {
            InstanceSymbol *inst = (InstanceSymbol *)sym;
            vector_destroy(inst->tp_args);
            vector_destroy(inst->bases);
            ASSERT(!sym->stbl);
            break;
        }
        case SYM_INSTANCE_FUNC: {
            InstanceFuncSymbol *inst_fn = (InstanceFuncSymbol *)sym;
            vector_destroy(inst_fn->real_params);
            vector_destroy(inst_fn->real_args);
            ASSERT(!sym->stbl);
            break;
        }

        case SYM_SHADOW_VAR: {
            // nothing
            ASSERT(!sym->stbl);
            break;
        }
        case SYM_INHERITED: {
            // nothing
            ASSERT(!sym->stbl);
            break;
        }
        case SYM_IMPORTED: {
            // nothing
            ASSERT(!sym->stbl);
            break;
        }
        default: {
            UNREACHABLE();
            break;
        }
    }

    mm_free(sym);
}

void free_all_symbols(void)
{
    Symbol *s;

    vector_foreach(s, &all_symbols) {
        if (!s) continue;
        __free_inner_stbl(s);
    }

    vector_foreach(s, &all_symbols) {
        if (!s) continue;
        __symbol_free(s);
    }

    vector_fini(&all_symbols);
}

Symbol *stbl_add(HashMap *stbl, Symbol *sym)
{
    if (hashmap_put_absent(stbl, sym) < 0) {
        mm_free(sym);
        sym = NULL;
    } else {
        add_to_global(sym);
    }
    return sym;
}

Symbol *stbl_remove(HashMap *stbl, char *name)
{
    uint64_t hash = str_hash(name);
    Symbol key = { .name = name };
    hashmap_entry_init(&key, hash);

    Symbol *sym = hashmap_remove(stbl, &key);
    return sym;
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

Symbol *stbl_add_shadow_var(HashMap *stbl, Symbol *origin, int is_null)
{
    ASSERT(origin->kind == SYM_VAR);
    ShadowVarSymbol *sym = mm_alloc_obj(sym);
    hashmap_entry_init(sym, str_hash(origin->name));
    sym->kind = SYM_SHADOW_VAR;
    sym->name = origin->name;
    sym->owner = stbl;

    if (hashmap_put_absent(stbl, sym) < 0) {
        mm_free(sym);
        sym = NULL;
    } else {
        TypeSpec *ts = ((VarSymbol *)origin)->ts;
        ASSERT(type_is_optional(ts));
        sym->ts = is_null ? ts : ts->opt.src;
        sym->origin = origin;
        sym->is_null = is_null;
        add_to_global(sym);
    }

    return (Symbol *)sym;
}

Symbol *stbl_add_func(HashMap *stbl, char *name, TypeSpec *ret, Vector *params, int flags)
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
        vector_init_ptr(&sym->tps);
        sym->ret = ret;
        // vector_init_ptr(&sym->locals);
        sym->stbl = stbl_new();
        add_to_global(sym);
    }

    return (Symbol *)sym;
}

Symbol *stbl_add_inherited_func(HashMap *stbl, Symbol *sym, KlassSymbol *origin_trait)
{
    ASSERT(sym->kind == SYM_FUNC);
    FuncSymbol *origin = (FuncSymbol *)sym;

    InheritedFunc *inherited = mm_alloc_obj(inherited);
    hashmap_entry_init(inherited, str_hash(origin->name));
    inherited->kind = SYM_INHERITED;
    inherited->name = atom(origin->name);

    if (hashmap_put_absent(stbl, inherited) < 0) {
        mm_free(inherited);
        inherited = NULL;
    } else {
        inherited->origin = origin;
        inherited->trait = origin_trait;
        // inherited->ts = origin->ts;
        add_to_global(inherited);
    }

    return (Symbol *)inherited;
}

KlassSymbol *stbl_add_klass(HashMap *stbl, char *name, int flags, int is_trait)
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
        sym->flags = flags;
        sym->stbl = stbl_new();
        vector_init_ptr(&sym->tps);
        vector_init_ptr(&sym->bases);
        vector_init_ptr(&sym->pip);
        vector_init_ptr(&sym->lro);
        vector_init_ptr(&sym->scm);
        vector_init_ptr(&sym->intf_table);
        add_to_global(sym);
    }

    return sym;
}

TypeParamSymbol *stbl_add_type_param(HashMap *stbl, char *name, Symbol *owner)
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
        vector_init_ptr(&sym->bound);
        add_to_global(sym);
    }

    return sym;
}

PkgSymbol *stbl_add_pkg(HashMap *stbl, char *path)
{
    PkgSymbol *sym = mm_alloc_obj(sym);
    hashmap_entry_init(sym, str_hash(path));
    sym->kind = SYM_PACKAGE;
    sym->name = path;

    if (hashmap_put_absent(stbl, sym) < 0) {
        mm_free(sym);
        sym = NULL;
    } else {
        add_to_global(sym);
    }

    return sym;
}

Symbol *stbl_add_imported(HashMap *stbl, Symbol *origin, char *name)
{
    ImportedSymbol *sym = mm_alloc_obj(sym);
    hashmap_entry_init(sym, str_hash(name));
    sym->kind = SYM_IMPORTED;
    sym->name = name;

    if (hashmap_put_absent(stbl, sym) < 0) {
        mm_free(sym);
        sym = NULL;
    } else {
        sym->origin = origin;
        add_to_global(sym);
    }

    return (Symbol *)sym;
}

static char *mangle_type_name(char *base_name, Vector *tp_args)
{
    BUF(buf);
    buf_write_str(&buf, base_name);

    if (vector_size(tp_args) > 0) {
        buf_write_char(&buf, '<');
        TypeSpec *ts;
        vector_foreach(ts, tp_args) {
            type_spec_to_str(ts, &buf);
        }
        buf_write_char(&buf, '>');
    }

    char *mangled_name = atom_str(BUF_STR(buf));
    FINI_BUF(buf);
    return mangled_name;
}

static Symbol *stbl_add_instance(HashMap *stbl, Symbol *origin, char *mangled_name,
                                 Vector *tp_args, TypeSpec *instance_ts)
{
    InstanceSymbol *sym = mm_alloc_obj(sym);
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
        sym->tp_args = type_spec_vec_copy(tp_args);
        sym->stbl = stbl_new();
        sym->instance_ts = instance_ts;
        sym->instance_ts->sym_id = sym->id;
        sym->instance_ts->checked = 1;
    }

    return (Symbol *)sym;
}

static int get_generic_var_index(HashMap *stbl, TypeSpec *ts)
{
    ASSERT(ts->kind == TYPE_GENERIC_VAR);
    Symbol *sym = stbl_get(stbl, ts->generic_var.owner);
    ASSERT(sym);
    ASSERT(sym->kind == SYM_CLASS || sym->kind == SYM_TRAIT);

    Symbol *_sym = stbl_get(sym->stbl, ts->generic_var.name);
    ASSERT(_sym && _sym->kind == SYM_TYPE_PARAM);
    TypeParamSymbol *tp = (TypeParamSymbol *)_sym;
    ASSERT(tp->index >= 0);
    return tp->index;
}

static InstanceSymbol *__instance_type_spec(HashMap *stbl, TypeSpec *ts, Vector *tp_args)
{
    ASSERT(ts->kind == TYPE_GENERIC_REF);

    Symbol *origin_sym = get_symbol_by_id(ts->sym_id);
    if (!origin_sym) {
        origin_sym = stbl_get(stbl, ts->generic_ref.name);
    }
    ASSERT(origin_sym->kind == SYM_CLASS || origin_sym->kind == SYM_TRAIT ||
           origin_sym->kind == SYM_INSTANCE);

    Vector *base_tp_args = vector_create_ptr();
    TypeSpec *spec_arg_ts;
    TypeSpec *arg_ts;
    vector_foreach(arg_ts, ts->generic_ref.args) {
        if (!arg_ts) continue;
        if (arg_ts->kind == TYPE_GENERIC_VAR) {
            if (arg_ts->generic_var.index < 0) {
                arg_ts->generic_var.index = get_generic_var_index(stbl, arg_ts);
            }
            spec_arg_ts = vector_get(tp_args, arg_ts->generic_var.index);
        } else if (arg_ts->kind == TYPE_GENERIC_REF) {
            // nested generic_ref type
            InstanceSymbol *spec_arg_sym = __instance_type_spec(stbl, arg_ts, tp_args);
            spec_arg_ts = spec_arg_sym->instance_ts;
        } else {
            ASSERT(arg_ts->kind != TYPE_UNRESOLVED);
            spec_arg_ts = arg_ts;
        }
        vector_push_back(base_tp_args, &spec_arg_ts);
    }

    InstanceSymbol *inst_sym = find_or_add_instance(stbl, origin_sym, base_tp_args);
    vector_destroy(base_tp_args);
    return inst_sym;
}

/*
static TypeSpec *find_two_lub(TypeSpec *a, TypeSpec *b)
{
    if (a == b) return a;

    Symbol *a_sym = get_symbol_by_id(a->sym_id);
    Symbol *b_sym = get_symbol_by_id(b->sym_id);

    Vector *a_lro = NULL;
    Vector *b_lro = NULL;

    if (a_sym->kind == SYM_CLASS || a_sym->kind == SYM_TRAIT) {
        a_lro = ((KlassSymbol *)a_sym)->lro;
    } else if (a_sym->kind == SYM_INSTANCE) {
        a_lro = ((InstanceSymbol *)a_sym)->origin->lro;
    } else {
        UNREACHABLE();
    }

    if (b_sym->kind == SYM_CLASS || b_sym->kind == SYM_TRAIT) {
        b_lro = ((KlassSymbol *)b_sym)->lro;
    } else if (b_sym->kind == SYM_INSTANCE) {
        b_lro = ((InstanceSymbol *)b_sym)->origin->lro;
    } else {
        UNREACHABLE();
    }

    for (int i = 0; i < vector_size(a_lro); i++) {
        TypeSpec *a_base_ts = vector_get(a_lro, i);
        for (int j = 0; j < vector_size(b_lro); j++) {
            TypeSpec *b_base_ts = vector_get(b_lro, j);
            if (a_base_ts == b_base_ts) {
                log_info("found lub '%s' for '%s' and '%s'", a_base_ts->signature,
                         a->signature, b->signature);
                return a_base_ts;
            }
        }
    }

    UNREACHABLE();
}
*/

TypeSpec *find_lub(Vector *types)
{
    if (vector_empty(types)) return any_type_spec();

    TypeSpec *ts = vector_get(types, 0);

    for (int i = 1; i < vector_size(types); i++) {
        TypeSpec *_ts = vector_get(types, i);
        if (_ts != ts) {
            log_info("types has different arg types, lub is 'any'");
            return any_type_spec();
        }
        /*
        if (type_is_any(ts)) break;
        ts = find_two_lub(ts, _ts);
        ASSERT(ts);
        */
    }

    log_info("find lub: '%s'", ts->signature);
    return ts;
}

static inline int ts_is_generic(TypeSpec *ts)
{
    ASSERT(ts);
    if (ts->kind == TYPE_GENERIC_VAR) return 1;
    if (ts->kind == TYPE_GENERIC_REF) return 1;
    return 0;
}

static inline int tps_are_generic(Vector *tp_args)
{
    TypeSpec *ts;
    vector_foreach(ts, tp_args) {
        if (!ts) continue;
        if (ts_is_generic(ts)) return 1;
    }
    return 0;
}

InstanceSymbol *find_or_add_instance(HashMap *stbl, Symbol *origin, Vector *tp_args)
{
    ASSERT(origin->kind == SYM_CLASS || origin->kind == SYM_TRAIT || origin->kind == SYM_INSTANCE);

    if (origin->kind == SYM_INSTANCE) {
        // inherited instance, e.g. List[int] is an inherited instance of List[T]
        InstanceSymbol *origin_inst = (InstanceSymbol *)origin;
        origin = origin_inst->origin;
    }

    char *mangled_name = mangle_type_name(origin->name, tp_args);
    Symbol *sym = stbl_get(stbl, mangled_name);
    if (sym) {
        log_info("found existing instance symbol '%s'(generic=%d)", mangled_name,
                 sym->flags & SYM_FLAGS_GENERIC ? 1 : 0);
        ASSERT(sym->kind == SYM_INSTANCE);
        return (InstanceSymbol *)sym;
    }

    int generic = tps_are_generic(tp_args);

    if (generic) {
        log_info("added generic instance symbol '%s' for '%s'", mangled_name, origin->name);
        TypeSpec *instance_ts = generic_ref_type_spec(origin->path, origin->name, tp_args, -1);
        sym = stbl_add_instance(stbl, origin, mangled_name, tp_args, instance_ts);
    } else {
        log_info("added instance symbol '%s' for '%s'", mangled_name, origin->name);
        TypeSpec *instance_ts = klass_type_spec(origin->path, mangled_name);
        sym = stbl_add_instance(stbl, origin, mangled_name, tp_args, instance_ts);
    }

    InstanceSymbol *inst_sym = (InstanceSymbol *)sym;
    KlassSymbol *kls_sym = (KlassSymbol *)origin;

    // set instance bases
    if (!vector_empty(&kls_sym->bases)) {
        log_info("updating instance bases for '%s'", mangled_name);

        Vector *_tp_args = tp_args;

        if (!strcmp(origin->name, "tuple")) {
            // tuple instance
            // compute ...T for bases, methods parameters or return type
            log_info("handling tuple instance '%s'", mangled_name);
            TypeSpec *infer_ts = find_lub(tp_args);
            _tp_args = vector_create_ptr();
            inst_sym->arg = infer_ts; // pass infer type to instance symbol for later use
            vector_push_back(_tp_args, &infer_ts);
            log_info("tuple instance inferred type: '%s'", infer_ts->signature);
        }

        inst_sym->bases = vector_create_ptr();
        TypeSpec *base_ts;
        vector_foreach(base_ts, &kls_sym->bases) {
            if (!base_ts) continue;
            if (base_ts->kind == TYPE_KLASS) {
                vector_push_back(inst_sym->bases, &base_ts);
            } else {
                // handle generic_ref base class/trait
                ASSERT(base_ts->kind == TYPE_GENERIC_REF);
                // specialize base class/trait
                InstanceSymbol *base_sym = __instance_type_spec(stbl, base_ts, _tp_args);
                base_ts = base_sym->instance_ts;
                vector_push_back(inst_sym->bases, &base_ts);
                ASSERT(base_ts->kind == TYPE_KLASS || base_ts->kind == TYPE_GENERIC_REF);
            }
            log_info("updated instance base '%s' for '%s'", base_ts->klass_type.name,
                     mangled_name);
        }

        if (_tp_args != tp_args) {
            vector_destroy(_tp_args);
        }
    }

    return inst_sym;
}

Symbol *stbl_add_func_instance(HashMap *stbl, FuncSymbol *origin, char *mangled_name,
                               Vector *real_arg_types, TypeSpec *ret_type)
{
    InstanceFuncSymbol *sym = mm_alloc_obj(sym);
    hashmap_entry_init(sym, str_hash(mangled_name));
    sym->kind = SYM_INSTANCE_FUNC;
    sym->name = mangled_name;

    if (hashmap_put_absent(stbl, sym) < 0) {
        mm_free(sym);
        sym = NULL;
    } else {
        add_to_global(sym); // get sym->id
        sym->origin = origin;
        sym->real_args = type_spec_vec_copy(real_arg_types);
        sym->flags = origin->flags;
        sym->ret_ts = ret_type;

        Vector *real_params = vector_create_ptr();
        ArgInfo *arg;
        vector_foreach(arg, origin->params) {
            if (!arg) continue;
            ArgInfo *_real_param = mm_alloc_obj(_real_param);
            _real_param->name = arg->name;
            _real_param->ts = vector_get(real_arg_types, i__);
            vector_push_back(real_params, &_real_param);
        }
        sym->real_params = real_params;

        sym->ts = func_type_spec(real_arg_types, ret_type);
    }

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
                type_spec_print(var->ts, &buf);
                log_info("variable symbol: '%s', type: '%s'", sym->name, BUF_STR(buf));
                FINI_BUF(buf);
                break;
            }
            case SYM_FUNC: {
                FuncSymbol *fn = (FuncSymbol *)sym;
                BUF(buf);
                type_spec_print(fn->ts, &buf);
                log_info("function symbol: '%s', proto: '%s'", sym->name, BUF_STR(buf));
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
            case SYM_INSTANCE_FUNC: {
                InstanceFuncSymbol *inst_fn = (InstanceFuncSymbol *)sym;
                log_info("function instance symbol: '%s'", sym->name);
                break;
            }
            default: {
                UNREACHABLE();
                break;
            }
        }
    }
}

static IntfEntry *get_intf_entry(Vector *intfs, Symbol *trait_sym)
{
    IntfEntry *entry;
    vector_foreach(entry, intfs) {
        if (entry->trait == trait_sym) return entry;
    }
    return NULL;
}

static void build_class_intf_table(Symbol *sym)
{
    ASSERT(sym->kind == SYM_CLASS);

    KlassSymbol *kls = (KlassSymbol *)sym;
    if (vector_empty(&kls->bases)) return;

    log_info("building interface table for class '%s'", sym->name);

    Vector *intfs = &kls->intf_table;
    TypeSpec *ts;
    vector_foreach(ts, &kls->lro) {
        Symbol *_sym = get_symbol_by_id(ts->sym_id);
        if (_sym->kind != SYM_TRAIT) continue;
        log_info("add interface '%s' to class '%s'", _sym->name, sym->name);
        IntfEntry *entry = mm_alloc_obj(entry);
        entry->trait = _sym;
        vector_init_ptr(&entry->methods);
        entry->index = vector_size(intfs);
        vector_init_ptr(&entry->parents);
        vector_push_back(intfs, &entry);

        KlassSymbol *trait_kls = (KlassSymbol *)_sym;
        Symbol *fn;
        vector_foreach(fn, trait_kls->funcs) {
            Symbol *kls_fn = stbl_get(kls->stbl, fn->name);
            if (!kls_fn) {
                void *empty_fn = NULL;
                vector_push_back(&entry->methods, &empty_fn);
                log_warn("class '%s' does not implement method '%s' of interface '%s'", sym->name,
                         fn->name, _sym->name);
                continue;
            }
            log_info("add method '%s'%s for interface '%s' in class '%s'", fn->name,
                     fn->kind == SYM_INHERITED ? " (inherited)" : "", _sym->name, sym->name);
            vector_push_back(&entry->methods, &kls_fn);
        }
    }

    IntfEntry *entry;
    vector_foreach(entry, &kls->intf_table) {
        Symbol *sym = entry->trait;
        KlassSymbol *trait_kls = (KlassSymbol *)sym;
        TypeSpec *ts;
        vector_foreach(ts, &trait_kls->lro) {
            Symbol *_sym = get_symbol_by_id(ts->sym_id);
            if (_sym == sym) continue;
            log_info("add parent trait '%s' for trait '%s' in class '%s'", _sym->name,
                     entry->trait->name, sym->name);
            ASSERT(_sym && _sym->kind == SYM_TRAIT);
            IntfEntry *e = get_intf_entry(intfs, _sym);
            ASSERT(e);
            vector_push_back(&entry->parents, &e);
        }
    }
}

void build_intf_table(HashMap *stbl)
{
    log_info("building interface table for classes...");

    HashMapIter it = { 0 };
    while (hashmap_next(stbl, &it)) {
        Symbol *sym = (Symbol *)it.entry;
        if (sym->kind != SYM_CLASS) continue;
        build_class_intf_table(sym);
    }
}

int get_intf_index(Symbol *sym, TypeSpec *trait_ts)
{
    ASSERT(sym->kind == SYM_CLASS || sym->kind == SYM_TRAIT);
    KlassSymbol *kls = (KlassSymbol *)sym;

    int index = 0;

    TypeSpec *ts;
    vector_foreach(ts, &kls->lro) {
        if (!ts) continue;

        if (ts == trait_ts) {
            return index;
        }

        ++index;
    }

    UNREACHABLE();
}

static void dump_class_intf_table(Symbol *sym)
{
    ASSERT(sym->kind == SYM_CLASS);
    KlassSymbol *kls = (KlassSymbol *)sym;

    printf("--- Intf-Table of class %s ---\n", sym->name);

    IntfEntry *entry;
    vector_foreach(entry, &kls->intf_table) {
        printf("\n  [%d] %s\n", entry->index, entry->trait->name);

        printf("        methods:\n");

        Symbol *meth;
        vector_foreach(meth, &entry->methods) {
            if (!meth) {
                printf("          [%d] <empty>\n", i__);
                continue;
            }
            ASSERT(meth->kind == SYM_FUNC);
            FuncSymbol *fn = (FuncSymbol *)meth;
            printf("          [%d] %s, code_index=%d\n", i__, fn->name, fn->code_index);
        }

        printf("        parents:\n");
        IntfEntry *parent;
        vector_foreach(parent, &entry->parents) {
            printf("          [%d] %s\n", i__, parent->trait->name);
        }
    }
    printf("\n");
}

void dump_intf_table(HashMap *stbl)
{
    printf("\n");

    HashMapIter it = { 0 };
    while (hashmap_next(stbl, &it)) {
        Symbol *sym = (Symbol *)it.entry;
        if (sym->kind != SYM_CLASS) continue;
        KlassSymbol *kls = (KlassSymbol *)sym;
        dump_class_intf_table(sym);
    }
}

#ifdef __cplusplus
}
#endif
