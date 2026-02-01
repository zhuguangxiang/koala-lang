/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "parser.h"
#include "atom.h"
#include "klc.h"
#include "log.h"

/* clang-format off */
#include "koala_yacc.h"
#include "koala_lex.h"
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

/* saved in ps->imported */
typedef struct _Imported {
    HashMapEntry hnode;
    char *key;
    /* module or others */
    Symbol *sym;
} Imported;

/* saved symbols */
static HashMap *imported;
static HashMap *current;
static HashMap *builtin;

ModuleSymbol *import_module(char *path)
{
    Symbol *mod_sym = stbl_get(imported, path);
    if (mod_sym) {
        log_info("module '%s' already imported", path);
        return (ModuleSymbol *)mod_sym;
    }

    mod_sym = stbl_add_module(imported, path);
    load_module((ModuleSymbol *)mod_sym, path);
    log_info("imported module '%s' successfully", path);
    return (ModuleSymbol *)mod_sym;
}

static inline void load_builtin_module(void)
{
    ModuleSymbol *mod_sym = import_module("libs/builtin.klc");
    builtin = mod_sym->stbl;
    update_builtin_types(builtin);
}

void init_parser(void)
{
    imported = stbl_new();
    current = stbl_new();
    load_builtin_module();
}

void fini_parser(void) {}

void kl_error_detail(ParserState *ps, Loc *loc) {}

static ParserScope *new_scope(ScopeKind kind, BlockType block)
{
    ParserScope *scope = mm_alloc_obj(scope);
    scope->kind = kind;
    scope->block_type = block;
    if (kind == SCOPE_BLOCK) scope->stbl = stbl_new();
    return scope;
}

static void free_scope(ParserScope *scope)
{
    // only block scope needs to free symbol table
    if (scope->kind == SCOPE_BLOCK) stbl_free(scope->stbl);
    mm_free(scope);
}

#ifndef NOLOG
/* clang-format off */
#define log_type_spec(ts) do {        \
    BUF(buf);                           \
    type_spec_print(ts, &buf);          \
    log_info("  '%s'", BUF_STR(buf));   \
    FINI_BUF(buf);                      \
} while (0)
/* clang-format on */
#else
#define log_type_spec(ts) ((void *)(ts))
#endif

/* clang-format off */
#define print_type_spec(ts) do {    \
    BUF(buf);                       \
    type_spec_print(ts, &buf);      \
    printf(" %s", BUF_STR(buf));    \
    FINI_BUF(buf);                  \
} while (0)
/* clang-format on */

#ifndef NOLOG
static const char *scopes[] = {
    "TOP", "CLASS", "TRAIT", "FUNC", "BLOCK", "ANONY",
};

static const char *blocks[] = {
    "UNK",       "BLOCK",       "IF-BLOCK",   "ELSE-BLOCK",   "WHILE-BLOCK",
    "FOR-BLOCK", "MATCH-BLOCK", "MATCH-CASE", "MATCH-CLAUSE",
};
#endif

ParserScope *enter_scope(ParserState *ps, ScopeKind kind, BlockType block, char *name)
{
    ParserScope *scope = new_scope(kind, block);
    scope->next = ps->scope;
    scope->name = name;
    ps->scope = scope;
    ++ps->depth;

#ifndef NOLOG
    const char *str;
    if (kind != SCOPE_BLOCK)
        str = scopes[kind];
    else
        str = blocks[block];
    log_info("====== Enter scope-%d(%s, %s) ======", ps->depth, str, scope->name);
#endif
    return scope;
}

void exit_scope(ParserState *ps)
{
    ParserScope *scope = ps->scope;

#ifndef NOLOG
    const char *str;

    if (scope->kind != SCOPE_BLOCK)
        str = scopes[scope->kind];
    else
        str = blocks[scope->block_type];
    log_info("====== Exit scope-%d(%s, %s) ======", ps->depth, str, scope->name);
#endif

    ps->scope = scope->next;
    free_scope(scope);
    --ps->depth;
}

static FuncSymbol *get_current_function(ParserState *ps)
{
    ParserScope *sc = ps->scope;
    while (sc) {
        if (sc->kind == SCOPE_FUNC) {
            return (FuncSymbol *)sc->sym;
        }
        sc = sc->next;
    }
    return NULL;
}

static void update_ir_info(ParserState *ps, Symbol *sym, char *module)
{
    // KlrValue *val = NULL;
    // if (sym->kind == SYM_FUNC) {
    //     val = klr_add_ext_func(ps->module, DESC_INCREF_GET(sym->desc), module,
    //     sym->name);
    // } else {
    //     NYI();
    // }
    // sym->ir_val = val;
}

Symbol *find_symbol(ParserState *ps, Ident *id)
{
    ParserScope *sc = ps->scope;

    /* find id from current scope */
    Symbol *sym = stbl_get(sc->stbl, id->name);
    if (sym) {
        log_info("find symbol '%s' in scope-%d(%s)", id->name, ps->depth,
                 scopes[sc->kind]);
        id->where = CURRENT_SCOPE;
        id->scope = sc;

        if (sym->kind == SYM_SHADOW_VAR) {
            log_info("  note: symbol '%s' is a shadow variable.", id->name);
            ShadowVarSymbol *shadow_sym = (ShadowVarSymbol *)sym;
            log_info("  value is null: %s", shadow_sym->is_null ? "true" : "false");
        }

        return sym;
    }

    /* find ident from up scope */
    ParserScope *up = sc->next;
    int depth = ps->depth - 1;
    while (up) {
        sym = stbl_get(up->stbl, id->name);
        if (sym) {
            log_info("find symbol '%s' in up scope-%d(%s)", id->name, depth,
                     scopes[up->kind]);
            id->where = UP_SCOPE;
            id->scope = up;

            if (sym->kind == SYM_SHADOW_VAR) {
                log_info("  note: symbol '%s' is a shadow variable.", id->name);
                ShadowVarSymbol *shadow_sym = (ShadowVarSymbol *)sym;
                log_info("  value is null: %s", shadow_sym->is_null ? "true" : "false");
            }

            return sym;
        }
        up = up->next;
    }

    /* find ident from external scope (imported) */
    /* find ident from auto-imported(builtin) */
    sym = stbl_get(ps->builtin, id->name);
    if (sym) {
        log_info("find symbol '%s' in builtin module", id->name);
        id->where = BLTIN_SCOPE;
        id->scope = NULL;
        update_ir_info(ps, sym, "builtin");
        return sym;
    }

    return NULL;
}

Symbol *find_type_symbol(ParserState *ps, TypeIdent *pkg, TypeIdent *name)
{
    if (pkg->name == NULL) {
        // find in current module
        Ident id = { .name = name->name, .loc = name->loc };
        return find_symbol(ps, &id);
    }

    // TODO: find package
    return NULL;
}

/**
 * Determines if a TypeSpec is a value type.
 * Value types (primitive types like int, float, bool) require exact
 * memory layout matching and are strictly invariant as generic parameters.
 */
static int is_value_type(TypeSpec *type)
{
    if (!type) return 0;
    switch (type->kind) {
        case TYPE_INT:
        case TYPE_FLOAT:
        case TYPE_BFLOAT16:
        case TYPE_BOOL:
            return 1;
        default:
            return 0;
    }
}

/**
 * Checks for strict identity between two types.
 * Required for value-type generic parameters to ensure binary compatibility.
 */
static int type_spec_equal_strict(TypeSpec *a, TypeSpec *b)
{
    if (a == b) return 1;
    if (a->kind != b->kind || a->sym_id != b->sym_id) return 0;

    // Numerical values must have identical representation (sign and width)
    if (a->kind == TYPE_INT || a->kind == TYPE_FLOAT || a->kind == TYPE_BFLOAT16) {
        return a->int_flt_info.width == b->int_flt_info.width &&
               a->int_flt_info.sign == b->int_flt_info.sign;
    }
    return 1;
}

static int is_subtype_of(int child_id, int parent_id)
{
    if (child_id == parent_id) return 1;

    Symbol *s = get_symbol_by_id(child_id);
    if (!s) {
        UNREACHABLE();
        return 0;
    }

    if (s->kind != SYM_CLASS && s->kind != SYM_TRAIT) {
        UNREACHABLE();
        return 0;
    }

    KlassSymbol *sym = (KlassSymbol *)s;
    if (!sym->bases) return 0;

    Symbol *base;
    vector_foreach(base, sym->bases) {
        if (!base) continue;
        if (is_subtype_of(base->id, parent_id)) return 1;
    }

    return 0;
}

/**
 * Checks if the 'src' type is compatible with the 'dst' type.
 *
 * Rules for High-Performance Type System:
 * 1. Top Type: TYPE_ANY is the root and accepts any type.
 * 2. Strict Kind Matching: Except for Object, kinds must match (e.g., no implicit
 * int-to-float).
 * 3. Numerical Widening: For INT/FLOAT, source width <= destination width is allowed.
 *    Note: Semantically compatible but requires explicit 'cast' instructions during
 * codegen due to binary representation (memory layout) mismatch.
 * 4. Generic Variance:
 *    - Reference Types (Classes): Invariant (e.g., List[Dog] -> List[Animal] not
 * allowed).
 *    - Value Types (Primitives): Invariant (Generic parameters must be strictly
 * compatible).
 */
int type_spec_compatible(TypeSpec *dst, TypeSpec *src)
{
    if (!dst || !src) return 0;

    if (dst == src) return 1;

    // Rule 1: TYPE_ANY is the Top Type (Root of the type hierarchy)
    if (dst->kind == TYPE_ANY) return 1;

    if (dst->kind == TYPE_OPTIONAL) {
        ASSERT(dst->opt.src != NULL);
        if (src->kind == TYPE_OPTIONAL) {
            if (src->opt.src == NULL) {
                // src is null type
                return 1;
            } else {
                // both dst and src are optional
                return type_spec_compatible(dst->opt.src, src->opt.src);
            }
        } else {
            // dst is optional, src is not optional
            return type_spec_compatible(dst->opt.src, src);
        }
    } else {
        // dst is not optional
        // fall through to normal type compatibility check
    }

    // Rule 2: Strict kind matching (Semantic barrier)
    if (dst->kind != src->kind) {
        if (src->kind == TYPE_GENERIC_VAR) {
            // check dst with src's upbound
            TypeParamSymbol *sym = get_symbol_by_id(src->sym_id);
            TypeSpec *bound;
            vector_foreach(bound, sym->bound) {
                if (!bound) continue;
                if (type_spec_compatible(dst, bound)) {
                    // Only one bound is compatible, T is compatible with dst.
                    return 1;
                }
            }
        }

        if (dst->kind == TYPE_SPECIALIZED) {
            if (src->kind == TYPE_KLASS) {
                if (dst->sym_id == src->sym_id) {
                    return 1;
                } else {
                    KlassSymbol *sym = get_symbol_by_id(src->sym_id);
                    TypeSpec *base;
                    vector_foreach(base, sym->bases) {
                        if (!base) continue;
                        if (type_spec_compatible(dst, base)) {
                            return 1;
                        }
                    }
                    return 0;
                }
            }

            UNREACHABLE();
        }

        if (dst->kind == TYPE_UNION) {
            TypeSpec *arg;
            vector_foreach(arg, dst->union_type.args) {
                if (!arg) continue;
                if (type_spec_compatible(arg, src)) {
                    return 1;
                }
            }
        }

        if (dst->kind == TYPE_KLASS) {
            if (src->kind == TYPE_INT) {
                // check Integer base class
                Symbol *sym = get_symbol_by_id(src->sym_id);
                if (sym->kind != SYM_CLASS) return 0;
                KlassSymbol *kls_sym = (KlassSymbol *)sym;
                TypeSpec *base;
                vector_foreach(base, kls_sym->bases) {
                    if (!base) continue;
                    if (base->kind == TYPE_KLASS) {
                        if (base == src) {
                            return 1;
                        }
                    }
                }
                return 0;
            }
        }

        return 0;
    }

    // Rule 3: Numeric Widening Logic (INT/FLOAT/BFLOAT16)
    // Semantically compatible, but memory layout is incompatible.
    // Insert 'cast' instructions during code generation.
    if (dst->kind == TYPE_INT || dst->kind == TYPE_FLOAT) {
        if (dst->int_flt_info.sign != src->int_flt_info.sign) return 0;
        return dst->int_flt_info.width >= src->int_flt_info.width;
    }

    if (dst->kind == TYPE_FLOAT || dst->kind == TYPE_BFLOAT16) {
        // float16/bfloat16 can be promoted to float32/64
        return dst->int_flt_info.width >= src->int_flt_info.width;
    }

    // Rule 4: Symbol ID and Inheritance Check

    if (dst->kind == TYPE_GENERIC_VAR) {
        return dst->generic_var.index == src->generic_var.index;
    }

    // Rule 5: Structural Recursion for Specialized Types (Generics)

    if (dst->kind == TYPE_SPECIALIZED) {
        int d_args_size = vector_size(dst->specialized.args);
        int s_args_size = vector_size(src->specialized.args);
        if (d_args_size != s_args_size) {
            Symbol *sym = get_symbol_by_id(src->sym_id);
            if (sym->kind == SYM_CLASS) {
                KlassSymbol *kls_sym = (KlassSymbol *)sym;
                TypeSpec *base;
                vector_foreach(base, kls_sym->bases) {
                    if (!base) continue;
                    if (type_spec_compatible(dst, base)) {
                        return 1;
                    }
                }
            } else if (sym->kind == SYM_INSTANCE) {
                // TODO: bases instance
                InstanceSymbol *inst_sym = (InstanceSymbol *)sym;
                Symbol *origin_sym = inst_sym->origin;
                if (origin_sym->kind == SYM_CLASS) {
                    KlassSymbol *kls_sym = (KlassSymbol *)origin_sym;
                    TypeSpec *base;
                    vector_foreach(base, kls_sym->bases) {
                        if (!base) continue;
                        if (type_spec_compatible(dst, base)) {
                            return 1;
                        }
                    }
                }
            }

            return 0;
        }

        // Handle Variance based on storage model

        for (int i = 0; i < d_args_size; i++) {
            TypeSpec *d_arg = vector_get_object(dst->specialized.args, i);
            TypeSpec *s_arg = vector_get_object(src->specialized.args, i);

            // generic parameters must be strictly compatible(invariant).
            // List[int32] and List[int64] are not compatible.
            // List[Dog] and List[Animal] are not compatible.
            if (!type_spec_equal_strict(d_arg, s_arg)) return 0;
        }

        return 1;
    }

    if (dst->kind == TYPE_KLASS) {
        Symbol *sym = get_symbol_by_id(src->sym_id);
        Vector *bases = NULL;
        if (sym->kind == SYM_CLASS || sym->kind == SYM_TRAIT) {
            KlassSymbol *kls_sym = (KlassSymbol *)sym;
            bases = kls_sym->bases;
        } else if (sym->kind == SYM_INSTANCE) {
            InstanceSymbol *inst_sym = (InstanceSymbol *)sym;
            bases = inst_sym->bases;
        } else {
            UNREACHABLE();
            return 0;
        }

        TypeSpec *base;
        vector_foreach(base, bases) {
            if (!base) continue;
            if (type_spec_compatible(dst, base)) {
                return 1;
            }
        }

        log_info("no compatible base found for klass type, compare directly by pointer");
        return dst == src;
    }

    if (dst->sym_id != src->sym_id) {
        // Check if 'src' is a subtype of 'dst' in the symbol table
        return is_subtype_of(src->sym_id, dst->sym_id);
    }

    UNREACHABLE();
    return 0;
}

TypeSpec *resolve_type(ParserState *ps, TypeSpec *_ts)
{
    if (!_ts) return NULL;

    if (_ts->kind == TYPE_UNION) {
        ASSERT(_ts->type_id < 0);
        TypeSpec *arg;
        Vector *vec = vector_create_ptr();
        vector_foreach(arg, _ts->union_type.args) {
            if (!arg) continue;
            TypeSpec *ret = resolve_type(ps, arg);
            vector_push_back(vec, &ret);
        }
        type_spec_free(_ts);
        return union_type_spec_intern(vec);
    }

    if (_ts->kind != TYPE_UNRESOLVED) return _ts;

    // parse arguments by bottom-to-up method
    int open = 0;
    Vector *vec = NULL;
    if (vector_size(_ts->unresolved.args) > 0) {
        vec = vector_create_ptr();
        TypeSpec *ts;
        TypeSpec *ret;
        vector_foreach(ts, _ts->unresolved.args) {
            if (!ts) continue;
            ret = resolve_type(ps, ts);
            if (ret->kind == TYPE_GENERIC_VAR) open = 1;
            vector_push_back(vec, &ret);
        }
        vector_clear(_ts->unresolved.args);
    }

    Symbol *sym = find_type_symbol(ps, &_ts->unresolved.pkg, &_ts->unresolved.name);
    if (!sym) {
        kl_error(_ts->loc, "'%s' is not found", _ts->unresolved.name.name);
        goto error;
    }

    if (sym->kind == SYM_TYPE_PARAM) {
        if (vec != NULL) {
            kl_error(_ts->loc, "'%s' is a type parameter, not a generic type",
                     _ts->unresolved.name.name);
            goto error;
        }
        log_info("resolve type-parameter '%s'", _ts->unresolved.name.name);
        TypeParamSymbol *ts_sym = (TypeParamSymbol *)sym;
        TypeSpec *ret = generic_var_type_spec(ts_sym->name, ts_sym->index, ts_sym->id,
                                              ts_sym->owner->name);
        type_spec_free(_ts);
        return ret;

    } else if (sym->kind == SYM_CLASS || sym->kind == SYM_TRAIT) {
        KlassSymbol *kls_sym = (KlassSymbol *)sym;
        if (vector_size(kls_sym->tps) != vector_size(vec)) {
            kl_error(_ts->loc,
                     "Type argument mismatch: '%s' expects %d argument(s), but %d were "
                     "provided",
                     _ts->unresolved.name.name, vector_size(kls_sym->tps),
                     vector_size(vec));
            goto error;
        }

        if (open) {
            // open specialized type
            log_info("resolve open specialized type '%s'", _ts->unresolved.name.name);
            TypeSpec *ret = specialized_type_spec(
                _ts->unresolved.pkg.name, _ts->unresolved.name.name, vec, kls_sym->id);
            type_spec_free(_ts);
            return ret;
        }

        log_info("resolve closed specialized type '%s'", _ts->unresolved.name.name);

        // closed specialized type
        if (vector_empty(vec)) {
            log_info("resolve type '%s' without type-args", _ts->unresolved.name.name);
            TypeSpec *ret =
                klass_type_spec(_ts->unresolved.pkg.name, _ts->unresolved.name.name);
            // sure this TypeSpec is already interned
            ASSERT(ret->sym_id == kls_sym->id);
            ASSERT(kls_sym->instance_ts == ret);
            type_spec_free(_ts);
            return ret;
        } else {
            // all args are concrete types
            // create instance symbol
            log_info("resolve type '%s' with type-args", _ts->unresolved.name.name);
            Symbol *inst_sym = find_or_add_instance(ps->stbl, sym, vec);
            if (!inst_sym) {
                kl_error(_ts->loc, "failed to get instance for specialized type");
                return NULL;
            }
            TypeSpec *ret = ((InstanceSymbol *)inst_sym)->instance_ts;
            type_spec_free(_ts);
            return ret;
        }
    } else {
        UNREACHABLE();
    }

error:
    // TODO: free memroy
    return NULL;
}

// check arg is satified by up-bounds.
int check_type_constraints(Vector *bounds, TypeSpec *arg)
{
    // no up-bounds
    if (!bounds || vector_size(bounds) == 0) return 1;

    for (int i = 0; i < vector_size(bounds); i++) {
        TypeSpec **bound_p = vector_get(bounds, i);
        TypeSpec *bound = *bound_p;
        if (!type_spec_compatible(bound, arg)) {
            return 0;
        }
    }
    return 1;
}

/**
 * Performs semantic validation on a resolved TypeSpec.
 * @return 1 (true) if valid, 0 (false) if any constraint is violated.
 *
 * Responsibilities:
 * 1. Verify generic argument counts.
 * 2. Validate bounds/constraints (e.g., T : Animal).
 * 3. Recursively check nested types (e.g., List[Map[K, V]]).
 */
int check_type(ParserState *ps, TypeSpec *type)
{
    if (!type) return 1;

    if (type->checked) return 1;

    if (type->kind == TYPE_UNION) {
        ASSERT(type->type_id >= 0);
        TypeSpec *arg;
        vector_foreach(arg, type->union_type.args) {
            if (!arg) continue;
            if (!check_type(ps, arg)) return 0;
        }
        return 1;
    }

    // Only TYPE_SPECIALIZED requires complex validation of its arguments.
    if (type->kind != TYPE_SPECIALIZED) {
        type->checked = 1;
        return 1;
    }

    // 1. Get the original symbol definition (e.g., the template for Foo[T])
    KlassSymbol *sym = get_symbol_by_id(type->sym_id);
    if (!sym) {
        UNREACHABLE();
        return 0; // Should not happen if resolve_type passed
    }

    if (sym->kind != SYM_CLASS && sym->kind != SYM_TRAIT) {
        // should not happen
        UNREACHABLE();
        return 0;
    }

    Vector *tps = sym->tps;
    int arg_count = vector_size(type->specialized.args);
    if (vector_size(tps) != arg_count) {
        kl_error(type->loc,
                 "Type argument count mismatch: '%s' expects %d argument(s), but %d were "
                 "provided",
                 sym->name, vector_size(tps), arg_count);
        return 0;
    }

    // 2. Validate each generic argument against its defined constraints
    for (int i = 0; i < arg_count; i++) {
        TypeSpec *arg = vector_get_object(type->specialized.args, i);

        // Get the required bounds for the i-th parameter (e.g., [Animal, Serializable])
        TypeParamSymbol *tp_sym = vector_get_object(tps, i);
        Vector *bounds = tp_sym->bound;

        if (bounds && vector_size(bounds) > 0) {
            // Check if the provided 'arg' satisfies all upper bounds.
            // Since this is a constraint check, we use is_type_compatible.
            if (!check_type_constraints(bounds, arg)) {
                kl_error(
                    arg->loc,
                    "Type constraint violation: argument #%d does not satisfy bounds",
                    i + 1);
                return 0;
            }
        }

        // 3. Core: Recursively validate the argument itself (for nested generics)
        // This ensures Map[String, List[InvalidType]] is caught.
        if (!check_type(ps, arg)) {
            return 0;
        }
    }

    type->checked = 1;
    return 1;
}

static int parse_flags(PrefixFlags *flags)
{
    int f = 0;

    if (flags->pub.flag) f |= SYM_FLAGS_PUBLIC;

    if (flags->at.assoc_ident)
        f |= SYM_FLAGS_TAG_VALUE;
    else if (flags->at.ident)
        f |= SYM_FLAGS_TAG_ONLY;

    return f;
}

static Symbol *_add_var(ParserState *ps, HashMap *stbl, VarDeclStmt *var)
{
    Ident *id = &var->id;
    Symbol *sym;

    int flags = parse_flags(&var->flags);
    if (!var->ro) flags |= SYM_FLAGS_MUTABLE;

    sym = stbl_add_var(stbl, id->name, NULL, flags);

    if (!sym) {
        kl_error(id->loc, "redefinition of '%s'", id->name);
        return NULL;
    }

    var->sym = sym;
    return sym;
}

static Symbol *_add_local(ParserState *ps, HashMap *stbl, VarDeclStmt *var)
{
    Ident *id = &var->id;
    Symbol *sym;

    int flags = parse_flags(&var->flags);
    if (!var->ro) flags |= SYM_FLAGS_MUTABLE;

    sym = stbl_add_var(stbl, id->name, var->type, flags);

    if (!sym) {
        kl_error(id->loc, "redefinition of '%s'", id->name);
        return NULL;
    }

    var->sym = sym;
    return sym;
}

static void check_type_in_first_chain(ParserState *ps, TypeSpec *src, TypeSpec *dst)
{
    if (src->kind != TYPE_KLASS || dst->kind != TYPE_KLASS) return;

    KlassSymbol *src_sym = get_symbol_by_id(src->sym_id);
    KlassSymbol *dst_sym = get_symbol_by_id(dst->sym_id);

    int index = 0;
    TypeSpec *ts;
    vector_foreach(ts, src_sym->bases) {
        if (!ts) continue;
        if (type_spec_compatible(dst, ts)) {
            log_info("variable base type matched at index %d:", index);
            log_type_spec(ts);
        }
        index++;
    }
}

static void parse_var_decl(ParserState *ps, Stmt *stmt)
{
    VarDeclStmt *var = (VarDeclStmt *)stmt;
    Ident *id = &var->id;
    TypeSpec *ts = var->type;
    Expr *exp = var->exp;

    /*
     * If var is global, it is already existed.
     * If var is local, it needs to be added into symbol table.
     */
    if (var->where != VAR_GLOBAL && var->where != VAR_FIELD) {
        ParserScope *sc = ps->scope;
        if (!_add_local(ps, sc->stbl, var)) return;
    }

    VarSymbol *sym = (VarSymbol *)var->sym;
    ASSERT(sym);

    if (sym->status != SYM_UNRESOLVED) {
        log_info("variable '%s' is resolving or resolved.", id->name);
        return;
    }

    sym->status = SYM_RESOLVING;

    if (ts) {
        ts = resolve_type(ps, ts);
        if (!check_type(ps, ts)) return;
        var->type = ts;
    }

    if (!exp) {
        if (!ts) {
            kl_error(id->loc, "variable '%s' needs a type or an initializer", id->name);
            return;
        }

        ((VarSymbol *)var->sym)->ts = ts;
        log_info("variable '%s' type set as:", id->name);
        log_type_spec(ts);
        return;
    }

    exp->ctx = EXPR_CTX_LOAD;
    exp->expected = ts;
    parser_visit_expr(ps, exp);
    if (!exp->ts) return;

    if (exp->kind == EXPR_LITERAL_KIND) {
        LitExpr *lit_exp = (LitExpr *)exp;
        sym->scope = VAR_SCOPE_GLOBAL;
        Literal *lit = mm_alloc_obj(lit);
        if (lit_exp->which == LIT_EXPR_INT) {
            lit->which = LIT_INT;
            lit->sign = lit_exp->sign;
            lit->len = lit_exp->len;
            lit->ival = lit_exp->ival;
        } else if (lit_exp->which == LIT_EXPR_FLT) {
            lit->which = LIT_FLT;
            lit->fval = lit_exp->fval;
        } else if (lit_exp->which == LIT_EXPR_BOOL) {
            lit->which = LIT_BOOL;
            lit->bval = lit_exp->bval;
        } else if (lit_exp->which == LIT_EXPR_STR) {
            lit->which = LIT_STR;
            lit->len = lit_exp->len;
            lit->sval = lit_exp->sval;
        } else if (lit_exp->which == LIT_EXPR_NONE) {
            lit->which = LIT_NONE;
        } else {
            UNREACHABLE();
        }
        sym->lit = lit;
    }

    if (!ts) {
        /* update symbol type */
        sym->ts = exp->ts;
        log_info("update symbol '%s' type as:", sym->name);
        log_type_spec(sym->ts);
    } else {
        if (!sym->ts) sym->ts = ts;
        if (!type_spec_compatible(ts, exp->ts)) {
            kl_error(id->loc, "Types of two sides are not matched.");
            printf("lhs:");
            print_type_spec(ts);
            printf(" =/= rhs:");
            print_type_spec(exp->ts);
            printf("\n");
        } else {
            log_info("variable '%s' type check passed.", id->name);
            log_info("  declared type:");
            log_type_spec(ts);
            log_info("  rhs type:");
            log_type_spec(exp->ts);
        }
    }

    sym->status = SYM_RESOLVED;
}

static void check_top_func_flags(ParserState *ps, FuncDeclStmt *fn)
{
    PrefixFlags *flags = &fn->flags;

    AtFlag *at = &flags->at;
    if (at->flag.flag) {
        ASSERT(at->ident);
        if (strcmp(at->ident, "native")) {
            kl_error(at->id_loc, "only 'native' annotation is allowed in top func '%s'",
                     fn->id.name);
            return;
        }

        if (!at->assoc_ident) {
            kl_error(at->id_loc,
                     "'native' annotation needs a native func name in top func '%s'",
                     fn->id.name);
            return;
        }

        if (!vector_empty(fn->body)) {
            kl_error(at->id_loc, "func '%s' with 'native' annotation needs empty body.",
                     fn->id.name);
            return;
        }
    }
}

// only add function symbol and don't add parameters and return type.
static Symbol *_add_func(ParserState *ps, HashMap *stbl, FuncDeclStmt *fn)
{
    Ident *id = &fn->id;
    Symbol *sym;

    int flags = parse_flags(&fn->flags);
    char *ann = fn->flags.at.ident;
    char *ann_key = fn->flags.at.assoc_ident;
    sym = stbl_add_func(stbl, id->name, fn->tps, NULL, NULL, flags, ann, ann_key);

    if (!sym) {
        kl_error(id->loc, "redefinition of '%s'", id->name);
        return NULL;
    }

    fn->sym = sym;
    return sym;
}

static void parse_body(ParserState *ps, FuncSymbol *sym, Vector *stmts)
{
    int sz = vector_size(stmts);
    Stmt *s;
    vector_foreach(s, stmts) {
        if (!s) continue;

        parse_stmt(ps, s);

        if (ps->errors >= MAX_ERRORS) break;

        if (i__ + 1 < sz) {
            if (s->kind == STMT_RETURN_KIND) {
                kl_error(s->loc, "statements after this are unreachable.");
                return;
            }
        }

        // last statement
        if (i__ + 1 == sz) {
            if (s->kind != STMT_RETURN_KIND) {
                if (s->kind == STMT_EXPR_KIND) {
                    /*
                     * If last statement is expression in func body,
                     * the expr value can be func return value.
                     */
                    ExprStmt *exp = (ExprStmt *)s;
                    // KlrBuilder bldr;
                    // ParserScope *sc = ps->scope;
                    // klr_builder_end(&bldr, sc->bb);
                    // if (desc_is_no_type(sym->desc)) {
                    //     klr_build_ret_void(&bldr);
                    // } else {
                    //     // check types
                    //     // code gen
                    //     klr_build_ret(&bldr, exp->exp->ir_val);
                    // }
                } else {
                    // UNREACHABLE();
                }
            } else {
                // last statement is return statement
                RetStmt *ret = (RetStmt *)s;
                // if (desc_is_no_type(sym->desc)) {
                //     if (ret->exp) {
                //         kl_error(s->loc, "func '%s' has not return value.", sym->name);
                //         return;
                //     }
                // } else {
                //     // check types
                // }
            }
        }
    }
}

static void parse_func_decl(ParserState *ps, Stmt *stmt)
{
    FuncDeclStmt *fn = (FuncDeclStmt *)stmt;
    ParserScope *sc = ps->scope;
    if (sc->kind == SCOPE_TOP) {
        check_top_func_flags(ps, fn);
    }

    FuncSymbol *sym = (FuncSymbol *)fn->sym;

    sc = enter_scope(ps, SCOPE_FUNC, 0, sym->name);
    sc->stbl = sym->stbl;
    sc->sym = (Symbol *)sym;

    // parse return type
    if (fn->ret) {
        TypeSpec *_ts = fn->ret;
        _ts = resolve_type(ps, _ts);
        check_type(ps, _ts);
        sym->ret = _ts;
    } else {
        // default return type is 'None'
        sym->ret = no_type_spec();
    }

    Vector *args = vector_create_ptr();

    int has_va_arg = 0;
    ParamDecl *param;
    vector_foreach(param, fn->args) {
        if (!param) continue;

        if (has_va_arg && !param->value) {
            kl_error(param->id.loc,
                     "after variadic parameter must be the kw parameters.");
            return;
        }

        if (param->va_arg) {
            if (has_va_arg) {
                kl_error(param->id.loc, "only one variadic parameter is allowed.");
                return;
            }
            has_va_arg = 1;
        }

        ArgInfo *arg = mm_alloc_obj(arg);
        arg->name = param->id.name;
        arg->dfl_val_idx = 0;

        TypeSpec *ts;
        if (param->type) {
            ts = param->type;
            Expr *e = param->value;
            if (e) {
                if (e->kind != EXPR_LITERAL_KIND) {
                    kl_error(param->id.loc,
                             "parameter '%s' needs a literal default value",
                             param->id.name);
                    return;
                }
                e->ctx = EXPR_CTX_LOAD;
                e->expected = ts;
                parser_visit_expr(ps, e);
                if (!e->ts) return;
            }
        } else {
            Expr *e = param->value;
            if (e->kind != EXPR_LITERAL_KIND) {
                kl_error(param->id.loc, "parameter '%s' needs a literal default value",
                         param->id.name);
                return;
            }
            e->ctx = EXPR_CTX_LOAD;
            parser_visit_expr(ps, e);
            if (!e->ts) return;
            ts = e->ts;
        }

        if (ts) {
            ts = resolve_type(ps, ts);
            ASSERT(ts);
            check_type(ps, ts);
        }

        Symbol *s = stbl_add_var(sc->stbl, param->id.name, ts, 0);
        if (!s) {
            kl_error(param->id.loc, "redefinition of parameter '%s' in function '%s'",
                     param->id.name, fn->id.name);
            return;
        }

        ((VarSymbol *)s)->scope = VAR_SCOPE_PARAM;
        Expr *e = param->value;
        if (e) {
            LitExpr *lit_exp = (LitExpr *)e;
            Literal *lit = mm_alloc_obj(lit);
            if (lit_exp->which == LIT_EXPR_INT) {
                lit->which = LIT_INT;
                lit->sign = lit_exp->sign;
                lit->len = lit_exp->len;
                lit->ival = lit_exp->ival;
            } else if (lit_exp->which == LIT_EXPR_FLT) {
                lit->which = LIT_FLT;
                lit->fval = lit_exp->fval;
            } else if (lit_exp->which == LIT_EXPR_BOOL) {
                lit->which = LIT_BOOL;
                lit->bval = lit_exp->bval;
            } else if (lit_exp->which == LIT_EXPR_STR) {
                lit->which = LIT_STR;
                lit->len = lit_exp->len;
                lit->sval = lit_exp->sval;
            } else if (lit_exp->which == LIT_EXPR_NONE) {
                lit->which = LIT_NONE;
            } else {
                UNREACHABLE();
            }
            ((VarSymbol *)s)->lit = lit;
            arg->dfl_val_idx = 1;
        }

        arg->sym = s;
        arg->ts = ts;
        vector_push_back(args, &arg);
    }

    sym->params = args;

    // update func's type
    TypeSpec *fn_ts = func_type_spec_from_arginfo(args, sym->ret);
    sym->ts = fn_ts;
    log_info("update function '%s' type as:", sym->name);
    log_type_spec(sym->ts);

    /* parse body */
    parse_body(ps, sym, fn->body);

    exit_scope(ps);
}

static void parse_expr(ParserState *ps, Stmt *stmt)
{
    ExprStmt *s = (ExprStmt *)stmt;
    Expr *exp = s->exp;
    exp->ctx = EXPR_CTX_LOAD;
    parser_visit_expr(ps, exp);
}

static void parse_block(ParserState *ps, Stmt *stmt)
{
    BlockStmt *s = (BlockStmt *)stmt;

    ParserScope *sc = ps->scope;
    if (sc->block_type == ELSE_BLOCK) {
        // no need to create inner-block for else-block
        Stmt *_s;
        vector_foreach(_s, s->stmts) {
            parse_stmt(ps, _s);
        }
        return;
    }

    sc = enter_scope(ps, SCOPE_BLOCK, ONLY_BLOCK, "inner-block");

    Stmt *_s;
    vector_foreach(_s, s->stmts) {
        parse_stmt(ps, _s);
    }

    exit_scope(ps);
}

static void unbox_optional(ParserState *ps, Expr *exp)
{
    if (!expr_is_binary(exp)) return;

    BinaryExpr *bexp = (BinaryExpr *)exp;
    BiOpKind op = bexp->op;

    if (op != BINARY_EQ && op != BINARY_NEQ) return;

    Expr *lhs = bexp->lhs;
    Expr *rhs = bexp->rhs;

    ParserScope *sc = ps->scope;
    Symbol *lhs_sym = lhs->sym;
    Symbol *rhs_sym = rhs->sym;
    Symbol *sym = NULL;

    if (lhs_sym && (lhs_sym->kind == SYM_VAR) && type_is_optional(lhs->ts) &&
        expr_is_literal_null(rhs)) {
        log_info("lhs is var(optional) and rhs is null literal");
        if (op == BINARY_NEQ) {
            log_info("'%s' is optional, add shadow variable", lhs_sym->name);
            log_type_spec(lhs->ts->opt.src);
            sym = stbl_add_shadow_var(sc->stbl, lhs->sym, 0);
        } else if (op == BINARY_EQ) {
            log_info("'%s' is optional, add shadow variable(null)", lhs_sym->name);
            log_type_spec(lhs->ts);
            sym = stbl_add_shadow_var(sc->stbl, lhs->sym, 1);
        } else {
            UNREACHABLE();
        }
    } else if (expr_is_literal_null(lhs) && rhs_sym && (rhs_sym->kind == SYM_VAR) &&
               type_is_optional(rhs->ts)) {
        log_info("lhs is null literal and rhs is var(optional)");
        if (op == BINARY_NEQ) {
            log_info("'%s' is optional, add shadow variable", rhs_sym->name);
            log_type_spec(rhs->ts->opt.src);
            sym = stbl_add_shadow_var(sc->stbl, rhs->sym, 0);
        } else if (op == BINARY_EQ) {
            log_info("'%s' is optional, add shadow variable(null)", rhs_sym->name);
            log_type_spec(rhs->ts);
            sym = stbl_add_shadow_var(sc->stbl, rhs->sym, 1);
        } else {
            UNREACHABLE();
        }
    } else {
        log_info("none side is var and null literal");
    }

    if (sym) {
        log_info("added shadow variable '%s' in scope '%s'", sym->name, sc->name);
        if (!sc->shadows) {
            sc->shadows = vector_create_ptr();
        }
        vector_push_back(sc->shadows, &sym);
    }
}

static void parse_if(ParserState *ps, Stmt *stmt)
{
    ParserScope *sc = enter_scope(ps, SCOPE_BLOCK, IF_BLOCK, "if-block");
    IfStmt *s = (IfStmt *)stmt;
    Expr *cond = s->cond;
    Vector *shadows = NULL;

    cond->ctx = EXPR_CTX_LOAD;
    parser_visit_expr(ps, cond);
    if (!cond->ts) return;

    if (cond->ts->kind != TYPE_BOOL) {
        kl_error(cond->loc, "if condition must be boolean type.");
    }

    unbox_optional(ps, cond);

    shadows = sc->shadows;
    sc->shadows = NULL;

    Stmt *_s;
    vector_foreach(_s, s->block) {
        parse_stmt(ps, _s);
    }

    exit_scope(ps);

    if (s->_else) {
        sc = enter_scope(ps, SCOPE_BLOCK, ELSE_BLOCK, "else-block");
        // inherit shadow vars from if-block
        if (shadows) {
            log_info("inherit shadow vars from if-block to else-block");
            ShadowVarSymbol *sym;
            vector_foreach(sym, shadows) {
                ASSERT(sym->kind == SYM_SHADOW_VAR);
                stbl_add_shadow_var(sc->stbl, (Symbol *)sym->origin, !sym->is_null);
                log_info("added shadow variable '%s'(%s) in scope '%s'", sym->name,
                         sym->is_null ? "null" : "non-null", sc->name);
            }
            // destroy vector only
            vector_destroy(shadows);
        } else {
            log_info("no shadow vars in if-block to inherit");
        }
        parse_stmt(ps, s->_else);
        exit_scope(ps);
    } else {
        // no else block, how to inherit shadow vars?
        log_info("no else block, inherit shadow vars from if-block");
        if (shadows) {
            // destroy vector only
            vector_destroy(shadows);
        }
    }
}

// only add klass/trait symbol and add tp, fields and methods
static Symbol *_add_klass(ParserState *ps, HashMap *stbl, KlassDeclStmt *kls,
                          int is_trait)
{
    Ident *id = &kls->id;
    Symbol *sym;

    int flags = parse_flags(&kls->flags);

    sym = stbl_add_klass(stbl, id->name, flags, is_trait);

    if (!sym) {
        kl_error(id->loc, "redefinition of '%s'", id->name);
        return NULL;
    }

    KlassSymbol *kls_sym = (KlassSymbol *)sym;

    // add tps
    if (vector_size(kls->tps) > 0) {
        Vector *vec = vector_create_ptr();
        kls_sym->tps = vec;

        TypeParamDecl *tp;
        vector_foreach(tp, kls->tps) {
            if (!tp) continue;
            Symbol *tp_sym = stbl_add_type_param(sym->stbl, tp->id.name, sym);
            ((TypeParamSymbol *)tp_sym)->index = vector_size(vec);
            vector_push_back(vec, &tp_sym);
        }
    }

    // add fields & methods
    Stmt *stmt;
    vector_foreach(stmt, kls->stmts) {
        if (!stmt) continue;
        if (stmt->kind == STMT_VAR_KIND) {
            Symbol *var = _add_var(ps, sym->stbl, (VarDeclStmt *)stmt);
            if (var) vector_push_back(kls_sym->fields, &var);
        } else if (stmt->kind == STMT_FUNC_KIND) {
            Symbol *fn = _add_func(ps, sym->stbl, (FuncDeclStmt *)stmt);
            if (fn) vector_push_back(kls_sym->funcs, &fn);
        } else {
            UNREACHABLE();
        }
    }

    // add typespec to klass symbol
    TypeSpec *ts = klass_type_spec(NULL, kls->id.name);
    kls_sym->instance_ts = ts;
    ts->sym_id = kls_sym->id;

    ts = type_type_spec();
    kls_sym->ts = ts;

    kls->sym = sym;
    return sym;
}

static void parse_type_params(ParserState *ps, KlassDeclStmt *kls)
{
    KlassSymbol *sym = (KlassSymbol *)kls->sym;

    // parse type parameter's bounds
    int index = 0;
    TypeParamSymbol *tp_sym;

    TypeParamDecl *tp;
    vector_foreach(tp, kls->tps) {
        if (!tp) continue;

        tp_sym = vector_get_object(sym->tps, index);
        index++;

        if (vector_empty(tp->bound)) continue;

        Vector *vec = vector_create_ptr();
        TypeSpec *ts;
        vector_foreach(ts, tp->bound) {
            if (!ts) continue;
            ts = resolve_type(ps, ts);
            assert(ts);
            int r = check_type(ps, ts);
            assert(r);
            vector_push_back(vec, &ts);
        }
        tp_sym->bound = vec;
    }
}

static void parse_bases(ParserState *ps, KlassDeclStmt *kls)
{
    KlassSymbol *sym = (KlassSymbol *)kls->sym;

    /* parse base class and traits */
    if (vector_empty(kls->bases)) return;

    Vector *vec = vector_create_ptr();
    TypeSpec *ts;
    vector_foreach(ts, kls->bases) {
        if (!ts) continue;
        TypeSpec *base_ts = resolve_type(ps, ts);
        if (!base_ts) continue;
        int r = check_type(ps, base_ts);
        if (!r) continue;

        Symbol *base_sym = get_symbol_by_id(base_ts->sym_id);
        if (!base_sym) {
            UNREACHABLE();
        }

        if (base_sym->kind == SYM_TRAIT) {
            vector_push_back(vec, &base_ts);
            log_info("base is trait symbol: %s", base_sym->name);
        } else if (base_sym->kind == SYM_INSTANCE) {
            log_info("base is instance symbol: %s", base_sym->name);
            Symbol *origin_sym = ((InstanceSymbol *)base_sym)->origin;
            if (origin_sym->kind != SYM_TRAIT) {
                kl_error(
                    ts->loc,
                    "origin symbol '%s' is not trait, only trait can be used as base",
                    origin_sym->name);
            } else {
                vector_push_back(vec, &base_ts);
            }
        } else {
            kl_error(ts->loc, "'%s' is not trait, only trait can be used as base",
                     base_sym->name);
        }
    }
    sym->bases = vec;
}

static KlassSymbol *_get_base_sym(TypeSpec *base_ts)
{
    Symbol *_sym = get_symbol_by_id(base_ts->sym_id);
    if (!_sym) {
        UNREACHABLE();
        return NULL;
    }

    KlassSymbol *base_sym = NULL;

    if (_sym->kind == SYM_TRAIT) {
        base_sym = (KlassSymbol *)_sym;
    } else if (_sym->kind == SYM_INSTANCE) {
        InstanceSymbol *inst_sym = (InstanceSymbol *)_sym;
        Symbol *origin_sym = inst_sym->origin;
        if (origin_sym->kind == SYM_TRAIT) {
            base_sym = (KlassSymbol *)origin_sym;
        } else {
            UNREACHABLE();
        }
    } else {
        UNREACHABLE();
    }

    return base_sym;
}

/* compute primary inheritance path */
static void compute_pip(ParserState *ps, KlassSymbol *sym)
{
    Vector *pip = &sym->pip;
    if (vector_size(pip) > 0) return;

    TypeSpec *base_ts = vector_get_object(sym->bases, 0);
    if (!base_ts) {
        // add itself
        vector_push_back(pip, &sym->instance_ts);
        return;
    }

    KlassSymbol *base_sym = _get_base_sym(base_ts);

    // compute base's pip first
    compute_pip(ps, base_sym);

    // inherit from base's pip
    TypeSpec *ts;
    vector_foreach(ts, &base_sym->pip) {
        if (!ts) continue;
        vector_push_back(pip, &ts);
    }

    // add itself
    vector_push_back(pip, &sym->instance_ts);
}

static int type_in_vec(Vector *vec, TypeSpec *ts)
{
    TypeSpec *existing_ts;
    vector_foreach(existing_ts, vec) {
        if (!existing_ts) continue;
        if (existing_ts == ts) {
            return 1;
        }
    }
    return 0;
}

static void compute_lro(ParserState *ps, KlassSymbol *sym)
{
    Vector *lro = &sym->lro;
    if (vector_size(lro) > 0) return;

    TypeSpec *base_ts;
    vector_foreach(base_ts, sym->bases) {
        if (!base_ts) continue;

        KlassSymbol *base_sym = _get_base_sym(base_ts);

        // compute base's lro first
        compute_lro(ps, base_sym);
    }

    vector_foreach(base_ts, sym->bases) {
        if (!base_ts) continue;

        KlassSymbol *base_sym = _get_base_sym(base_ts);

        // inherit from base's lro
        TypeSpec *ts;
        vector_foreach(ts, &base_sym->lro) {
            if (!ts) continue;

            // check duplication
            if (!type_in_vec(lro, ts)) {
                vector_push_back(lro, &ts);
            }
        }
    }

    // add self
    vector_push_back(lro, &sym->instance_ts);
}

static void compute_scm(ParserState *ps, KlassSymbol *sym)
{
    Vector *scm = &sym->scm;
    if (vector_size(scm) > 0) return;

    TypeSpec *base_ts;
    vector_foreach(base_ts, &sym->lro) {
        if (!base_ts) continue;

        if (!type_in_vec(&sym->pip, base_ts)) {
            vector_push_back(scm, &base_ts);
        }
    }
}

#ifndef NOLOG
static void print_vtbl_info(KlassSymbol *sym)
{
    printf("vtbl info for klass/trait '%s':", sym->name);

    printf("\n  pip:");
    TypeSpec *ts;
    vector_foreach(ts, &sym->pip) {
        if (!ts) continue;
        if (i__ != 0) printf(" -> ");
        print_type_spec(ts);
    }

    printf("\n  lro:");
    vector_foreach(ts, &sym->lro) {
        if (!ts) continue;
        if (i__ != 0) printf(" -> ");
        print_type_spec(ts);
    }

    printf("\n  scm:");
    vector_foreach(ts, &sym->scm) {
        if (!ts) continue;
        if (i__ != 0) printf(" -> ");
        print_type_spec(ts);
    }
    printf("\n");
}
#else
#define print_vtbl_info(sym) \
    do { \
    } while (0)
#endif

static void compute_vtbl_info(ParserState *ps, KlassSymbol *sym)
{
    compute_pip(ps, sym);
    compute_lro(ps, sym);
    compute_scm(ps, sym);
    print_vtbl_info(sym);
}

static void parse_klass(ParserState *ps, Stmt *stmt)
{
    KlassDeclStmt *kls = (KlassDeclStmt *)stmt;
    KlassSymbol *sym = (KlassSymbol *)kls->sym;

    ScopeKind scope_kind = (kls->kind == STMT_CLASS_KIND) ? SCOPE_CLASS : SCOPE_TRAIT;

    ParserScope *sc = enter_scope(ps, scope_kind, 0, sym->name);
    sc->stbl = sym->stbl;
    sc->sym = (Symbol *)sym;

    // parse type parameter's bounds
    parse_type_params(ps, kls);

    /* parse base class and traits */
    parse_bases(ps, kls);

    /* compute vtbl info */
    compute_vtbl_info(ps, sym);

    /* parse class body */
    Stmt *s;
    vector_foreach(s, kls->stmts) {
        if (!s) continue;
        parse_stmt(ps, s);
    }

    exit_scope(ps);
}

static void parse_return(ParserState *ps, Stmt *stmt)
{
    RetStmt *ret = (RetStmt *)stmt;
    Expr *exp = ret->exp;
    FuncSymbol *fn_sym = get_current_function(ps);
    TypeSpec *fn_ret = fn_sym->ret;

    if (exp) {
        exp->ctx = EXPR_CTX_LOAD;
        parser_visit_expr(ps, exp);
        if (!exp->ts) return;

        if (!fn_ret || fn_ret->kind == TYPE_NO_TYPE) {
            kl_error(ret->loc, "function '%s' has no return value.", fn_sym->name);
            return;
        }

        if (!type_spec_compatible(fn_sym->ret, exp->ts)) {
            kl_error(ret->loc, "return type mismatch in function '%s'", fn_sym->name);
            log_info("  expected type:");
            log_type_spec(fn_sym->ret);
            log_info("  actual type:");
            log_type_spec(exp->ts);
            return;
        }
    } else {
        if (fn_ret && fn_ret->kind != TYPE_NO_TYPE) {
            kl_error(ret->loc, "function '%s' needs a return value.", fn_sym->name);
            return;
        }
    }
}

void parse_stmt(ParserState *ps, Stmt *stmt)
{
    if (!stmt) return;

    /* if errors is greater than MAX_ERRORS, stop parsing */
    if (ps->errors >= MAX_ERRORS) return;

    /* clang-format off */
    static void (*handlers[STMT_MAX_KIND])(ParserState *, Stmt *) = {
        [STMT_VAR_KIND]    = parse_var_decl,
        [STMT_FUNC_KIND]   = parse_func_decl,
        [STMT_CLASS_KIND]  = parse_klass,
        [STMT_TRAIT_KIND]  = parse_klass,
        [STMT_RETURN_KIND] = parse_return,
        [STMT_EXPR_KIND]   = parse_expr,
        [STMT_BLOCK_KIND]  = parse_block,
        [STMT_IF_KIND]     = parse_if,
        // [STMT_IF_LET_KIND] = parse_if_let,
    };
    /* clang-format on */

    handlers[stmt->kind](ps, stmt);
}

static void parse_ast(ParserState *ps)
{
    ParserScope *scope = enter_scope(ps, SCOPE_TOP, 0, "top");
    scope->stbl = ps->stbl;
    Stmt *stmt;
    vector_foreach(stmt, &ps->stmts) {
        if (!stmt) continue;
        parse_stmt(ps, stmt);
    }
    exit_scope(ps);

    /* If there are errors, stop doing codegen. */
    if (ps->errors) return;

#ifndef NOLOG
    /* dump symbol tables */
    stbl_show(ps->stbl);
#endif
}

static void init_parser_state(ParserState *ps, char *filename)
{
    ps->filename = filename;
    vector_init_ptr(&ps->stmts);
    ps->stbl = current;
    ps->builtin = builtin;
}

static ParserState *build_ast(char *path)
{
    FILE *in = fopen(path, "r");
    if (in == NULL) {
        kl_printf_error("%s: No such file or directory\n", path);
        return NULL;
    }

    ParserState *ps = mm_alloc_obj(ps);
    init_parser_state(ps, path);

    yyscan_t scanner;
    yylex_init_extra(ps, &scanner);
    yyset_in(in, scanner);
    yyparse(ps, scanner);
    yylex_destroy(scanner);

    fclose(in);

    return ps;
}

static void free_parser(ParserState *ps)
{
    FINI_BUF(ps->sbuf);
    stbl_free(ps->stbl);
    mm_free(ps);
}

int compile(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: koalac <source files>\n");
        return 0;
    }

    ParserState *ps = build_ast(argv[1]);
    if (!ps) return -1;

    parse_ast(ps);

    // if (!ps->errors) {
    //     codegen_ast(ps);
    // }

    if (!ps->errors) {
        kl_write_to_klc(ps);
    }

    free_parser(ps);

    return 0;
}

// only add symbol and do not add its type
static Symbol *_add_global(ParserState *ps, HashMap *stbl, VarDeclStmt *var)
{
    Ident *id = &var->id;
    TypeSpec *ts = var->type;
    Symbol *sym;

    int flags = parse_flags(&var->flags);
    if (!var->ro) flags |= SYM_FLAGS_MUTABLE;

    // don't add unresolved type
    if (ts && ts->kind == TYPE_UNRESOLVED) ts = NULL;
    sym = stbl_add_var(stbl, id->name, ts, flags);

    if (!sym) {
        kl_error(id->loc, "redefinition of '%s'", id->name);
        return NULL;
    }

    var->sym = sym;
    sym->arg = var;
    return sym;
}

void parse_top_stmt(ParserState *ps, Stmt *stmt)
{
    if (!stmt) return;

    Symbol *sym = NULL;

    switch (stmt->kind) {
        case STMT_VAR_KIND: {
            VarDeclStmt *var = (VarDeclStmt *)stmt;
            sym = _add_global(ps, ps->stbl, var);
            if (!sym) return;
            var->where = VAR_GLOBAL;
            break;
        }
        case STMT_FUNC_KIND: {
            FuncDeclStmt *fn = (FuncDeclStmt *)stmt;
            sym = _add_func(ps, ps->stbl, fn);
            if (!sym) return;
            break;
        }
        case STMT_CLASS_KIND: {
            KlassDeclStmt *kls = (KlassDeclStmt *)stmt;
            sym = _add_klass(ps, ps->stbl, kls, 0);
            if (!sym) return;
            break;
        }
        case STMT_TRAIT_KIND: {
            KlassDeclStmt *kls = (KlassDeclStmt *)stmt;
            sym = _add_klass(ps, ps->stbl, kls, 1);
            if (!sym) return;
            break;
        }
        default: {
            break;
        }
    }

    vector_push_back(&ps->stmts, &stmt);
}

#ifdef __cplusplus
}
#endif
