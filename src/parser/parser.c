/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "parser.h"
#include "atom.h"
#include "cmd.h"
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

typedef struct _InferredInfo {
    HashMapEntry hnode;
    /* klass_name.func_name */
    char *key;
    /* callback */
    Vector *(*infer)(FuncSymbol *fn, Vector *args, ParserState *ps);
} InferredInfo;

/* inferred tp of func */
static HashMap *inferred;

static Vector *infer_tuple___getitem__(FuncSymbol *fn, Vector *args, ParserState *ps)
{
    ASSERT(vector_size(args) == 1);
    ASSERT(vector_size(&fn->tps) == 1);

    Expr *e = vector_get(args, 0);
    if (!type_is_int(e->ts)) {
        kl_error(e->loc, "index of tuple must be int, but got '%s'", e->ts->signature);
        return NULL;
    }

    Symbol *parent = fn->parent;
    ASSERT(parent && parent->kind == SYM_INSTANCE);
    InstanceSymbol *inst_sym = (InstanceSymbol *)parent;

    if (e->kind == EXPR_LITERAL_KIND) {
        LitExpr *lit = (LitExpr *)e;
        ASSERT(lit->which == LIT_EXPR_INT);
        int index = (int)lit->ival;

        if (!(index >= 0 && index < vector_size(inst_sym->tp_args))) {
            kl_error(e->loc, "tuple index out of range, got %d but expected 0 <= index < %d",
                     index, vector_size(inst_sym->tp_args));
            return NULL;
        }

        TypeSpec *_ts = vector_get(inst_sym->tp_args, index);
        Vector *res = vector_create_ptr();
        vector_push_back(res, &_ts);
        return res;
    } else {
        // if index is not literal, we use infered tuple's tp type.
        TypeSpec *_ts = inst_sym->arg;
        ASSERT(_ts);
        Vector *res = vector_create_ptr();
        vector_push_back(res, &_ts);
        return res;
    }
}

static int __inferred_info_equal__(InferredInfo *a, InferredInfo *b)
{
    return !strcmp(a->key, b->key);
}

static HashMap *inferred_map(void)
{
    HashMap *map = mm_alloc_obj(map);
    hashmap_init(map, (HashMapEqualFunc)__inferred_info_equal__);

    InferredInfo *info = mm_alloc_obj(info);
    info->key = "tuple.__getitem__";
    info->infer = infer_tuple___getitem__;
    hashmap_put(map, info);
    return map;
}

static void _inferred_info_free_(void *entry, void *data)
{
    UNUSED(data);
    mm_free(entry);
}

static void free_inferred_map(void)
{
    hashmap_fini(inferred, _inferred_info_free_, NULL);
    mm_free(inferred);
}

Vector *infer_func_tp(FuncSymbol *fn, Vector *args, ParserState *ps)
{
    char name[256] = { 0 };
    Symbol *parent = fn->parent;
    ASSERT(parent && parent->kind == SYM_INSTANCE);
    InstanceSymbol *inst_sym = (InstanceSymbol *)parent;
    snprintf(name, sizeof(name), "%s.%s", inst_sym->origin->name, fn->name);
    InferredInfo key = { .key = name };
    InferredInfo *info = hashmap_get(inferred, &key);
    ASSERT(info);
    return info->infer(fn, args, ps);
}

// path without .klc suffix
static PkgSymbol *import_package(ParserModule *pm, char *path)
{
    Symbol *sym = stbl_get(pm->imported, path);
    if (sym) {
        log_info("module '%s' already imported", path);
        ASSERT(sym->kind == SYM_PACKAGE);
        return (PkgSymbol *)sym;
    }

    PkgSymbol *pkg_sym = stbl_add_pkg(pm->imported, path);
    int ret = load_module(path, pkg_sym);

    if (ret) {
        fprintf(stderr, "error: cannot import module '%s'\n", path);
        char *koala_path = getenv("KOALA_PATH");
        if (koala_path) {
            fprintf(stderr, "please check KOALA_PATH: %s\n", koala_path);
        } else {
            fprintf(stderr, "KOALA_PATH is not set\n");
        }
        abort();
    }

    ASSERT(pkg_sym->path);
    TypeSpec *ts = pkg_type_spec(pkg_sym->path);
    pkg_sym->ts = ts;
    ts->sym_id = pkg_sym->id;
    log_info("imported module '%s'(package-name: %s) successfully", path, pkg_sym->path);
    return pkg_sym;
}

static inline void load_builtin_module(ParserModule *pm)
{
    PkgSymbol *pkg_sym = import_package(pm, "std/builtin");
    if (!pkg_sym) return;
    pm->builtin = pkg_sym->stbl;
    install_builtin_types(pm->builtin);
}

static void mark_magic_func(HashMap *stbl)
{
    HashMapIter it = { 0 };
    while (hashmap_next(stbl, &it)) {
        Symbol *sym = (Symbol *)it.entry;
        if (sym->kind == SYM_FUNC) {
            if (str_equal(sym->name, "len")) {
                sym->flags |= SYM_FLAGS_MAGIC;
                log_info("marked magic function '%s'", sym->name);
            }
        }
    }
}

void init_parser(ParserModule *pm)
{
    vector_init_ptr(&pm->pss);
    pm->imported = stbl_new();
    pm->stbl = stbl_new();
    inferred = inferred_map();
    if (!is_build_stdlib()) {
        load_builtin_module(pm);
        mark_magic_func(pm->builtin);
    }
}

void fini_parser(ParserModule *pm)
{
    ParserState *ps;
    vector_foreach(ps, &pm->pss) {
        if (!ps) continue;
        free_parser_state(ps);
    }
    vector_fini(&pm->pss);

    stbl_free(pm->imported);
    stbl_free(pm->stbl);
    free_inferred_map();
    free_all_symbols();
}

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
static const char *scopes[] = {
    "TOP", "CLASS", "TRAIT", "FUNC", "BLOCK", "ANONY",
};

static const char *blocks[] = {
    "UNK",          "BLOCK",       "IF-BLOCK",        "ELSE-BLOCK",
    "IF-LET-BLOCK", "WHILE-BLOCK", "WHILE-LET-BLOCK", "FOR-BLOCK",
    "MATCH-BLOCK",  "MATCH-CASE",  "MATCH-CLAUSE",
};
#endif

ParserScope *enter_scope(ParserState *ps, ScopeKind kind, BlockType block, char *name)
{
    ParserScope *scope = new_scope(kind, block);
    ++ps->depth;
    scope->next = ps->scope;
    scope->name = name;
    scope->depth = ps->depth;
    ps->scope = scope;

#ifndef NOLOG
    const char *str;
    if (kind != SCOPE_BLOCK)
        str = scopes[kind];
    else
        str = blocks[block];
    log_info("====== Enter scope-%d(%s, %s) ======", scope->depth, str, scope->name);
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
    log_info("====== Exit scope-%d(%s, %s) ======", scope->depth, str, scope->name);
#endif

    ps->scope = scope->next;
    free_scope(scope);
    --ps->depth;
}

FuncSymbol *get_current_function(ParserState *ps)
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

Symbol *find_symbol(ParserState *ps, Ident *id)
{
    ParserScope *sc = ps->scope;

    /* find id from current scope */
    Symbol *sym = stbl_get(sc->stbl, id->name);
    if (sym) {
        log_info("find symbol '%s' in scope-%d(%s-%s)", id->name, sc->depth, scopes[sc->kind],
                 sc->name);
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
    while (up) {
        sym = stbl_get(up->stbl, id->name);
        if (sym) {
            log_info("find symbol '%s' in up scope-%d(%s-%s)", id->name, up->depth,
                     scopes[up->kind], up->name);
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
    sym = stbl_get(ps->pm->builtin, id->name);
    if (sym) {
        log_info("find symbol '%s' in 'std/builtin' module", id->name);
        id->where = BLTIN_SCOPE;
        id->scope = NULL;
        ASSERT(sym->flags & SYM_FLAGS_EXT);
        ASSERT(sym->path);
        return sym;
    }

    /* find ident from imported scope */
    sym = stbl_get(ps->imported, id->name);
    if (sym) {
        log_info("find symbol '%s' in imported scope", id->name);
        id->where = IMPORTED_SCOPE;
        id->scope = NULL;
        return ((ImportedSymbol *)sym)->origin;
    }

    return NULL;
}

Symbol *find_type_symbol(ParserState *ps, TypeIdent *pkg, TypeIdent *name)
{
    if (pkg->name == NULL || str_equal(pkg->name, "std/builtin") ||
        str_equal(pkg->name, ps->pm->pkg_path)) {
        // find in current module
        Ident id = { .name = name->name, .loc = name->loc };
        return find_symbol(ps, &id);
    }

    // find package
    Symbol *sym = stbl_get(ps->imported, pkg->name);
    if (!sym) return NULL;
    ASSERT(sym->kind == SYM_IMPORTED);
    Symbol *origin = ((ImportedSymbol *)sym)->origin;
    if (origin->kind != SYM_PACKAGE) {
        kl_error(pkg->loc, "symbol '%s' is not a package", origin->name);
        return NULL;
    }
    PkgSymbol *pkg_sym = (PkgSymbol *)origin;
    sym = stbl_get(pkg_sym->stbl, name->name);
    return sym;
}

static void parse_import(ParserState *ps, Stmt *stmt)
{
    // import is already resolved in parse_top_stmt
    // do nothing.
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
    if (vector_empty(&sym->bases)) return 0;

    Symbol *base;
    vector_foreach(base, &sym->bases) {
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
 * 2. Strict Kind Matching: Except for Object, kinds must match (e.g., no
 * implicit int-to-float).
 * 3. Numerical Widening: For INT/FLOAT, source width <= destination width is
 * allowed. Note: Semantically compatible but requires explicit 'cast'
 * instructions during codegen due to binary representation (memory layout)
 * mismatch.
 * 4. Generic Variance:
 *    - Reference Types (Classes): Invariant (e.g., List[Dog] -> List[Animal]
 * not allowed).
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
            vector_foreach(bound, &sym->bound) {
                if (!bound) continue;
                if (type_spec_compatible(dst, bound)) {
                    // Only one bound is compatible, T is compatible with dst.
                    return 1;
                }
            }
        }

        if (dst->kind == TYPE_GENERIC_REF) {
            if (src->kind == TYPE_KLASS) {
                if (dst->sym_id == src->sym_id) {
                    return 1;
                } else {
                    KlassSymbol *sym = get_symbol_by_id(src->sym_id);
                    TypeSpec *base;
                    vector_foreach(base, &sym->bases) {
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
                vector_foreach(base, &kls_sym->bases) {
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

    if (dst->kind == TYPE_GENERIC_REF) {
        int d_args_size = vector_size(dst->generic_ref.args);
        int s_args_size = vector_size(src->generic_ref.args);
        if (d_args_size != s_args_size) {
            Symbol *sym = get_symbol_by_id(src->sym_id);
            if (sym->kind == SYM_CLASS) {
                KlassSymbol *kls_sym = (KlassSymbol *)sym;
                TypeSpec *base;
                vector_foreach(base, &kls_sym->bases) {
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
                    vector_foreach(base, &kls_sym->bases) {
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
            TypeSpec *d_arg = vector_get(dst->generic_ref.args, i);
            TypeSpec *s_arg = vector_get(src->generic_ref.args, i);

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
            bases = &kls_sym->bases;
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

        log_info(
            "no compatible base found for klass type, compare directly by "
            "pointer");
        return dst == src;
    }

    if (dst->sym_id != src->sym_id) {
        UNREACHABLE();
        // Check if 'src' is a subtype of 'dst' in the symbol table
        return is_subtype_of(src->sym_id, dst->sym_id);
    }

    UNREACHABLE();
    return 0;
}

static void parse_klass_meta(ParserState *ps, KlassDeclStmt *kls);

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
        vector_destroy(_ts->union_type.args);
        _ts->union_type.args = NULL;
        type_spec_free(_ts);
        return union_type_spec_intern(vec);
    }

    if (_ts->kind == TYPE_OPTIONAL) {
        if (_ts->type_id >= 0) {
            return _ts;
        }

        TypeSpec *ret = resolve_type(ps, _ts->opt.src);
        _ts->opt.src = NULL;
        type_spec_free(_ts);
        return optional_type_spec_intern(ret);
    }

    if (_ts->kind == TYPE_VA_LIST) {
        if (_ts->type_id >= 0) {
            return _ts;
        }

        TypeSpec *ret = resolve_type(ps, _ts->va_list.src);
        _ts->va_list.src = NULL;
        type_spec_free(_ts);
        return va_list_type_spec_intern(ret);
    }

    if (_ts->kind != TYPE_UNRESOLVED) return _ts;

    // parse arguments by bottom-to-up method
    int open = 0;
    Vector *tp_args = NULL;
    if (vector_size(_ts->unresolved.args) > 0) {
        tp_args = vector_create_ptr();
        TypeSpec *ts;
        TypeSpec *ret;
        vector_foreach(ts, _ts->unresolved.args) {
            if (!ts) continue;
            ret = resolve_type(ps, ts);
            if (ret->kind == TYPE_GENERIC_VAR) open = 1;
            vector_push_back(tp_args, &ret);
        }
        vector_clear(_ts->unresolved.args);
    }

    Symbol *sym = find_type_symbol(ps, &_ts->unresolved.pkg, &_ts->unresolved.name);
    if (!sym) {
        kl_error(_ts->loc, "'%s' is not found", _ts->unresolved.name.name);
        vector_destroy(tp_args);
        return NULL;
    }

    if (sym->kind == SYM_TYPE_PARAM) {
        log_info("symbol '%s' is a type parameter", _ts->unresolved.name.name);
        if (tp_args != NULL) {
            kl_error(_ts->loc, "'%s' is a type parameter, not a generic type",
                     _ts->unresolved.name.name);
            vector_destroy(tp_args);
            return NULL;
        }
        log_info("resolve type-parameter '%s'", _ts->unresolved.name.name);
        TypeParamSymbol *tp_sym = (TypeParamSymbol *)sym;
        TypeSpec *ret = tp_sym->ts;
        if (ret == NULL) {
            if (tp_sym->which == TP_INFER) {
                log_info("type parameter '%s' is inferred", tp_sym->name);
            } else if (tp_sym->which == TP_CONST) {
                log_info("type parameter '%s' is const", tp_sym->name);
            } else {
                ASSERT(tp_sym->which == TP_NORMAL);
                log_info("type parameter '%s' is normal", tp_sym->name);
            }
            ret = generic_var_type_spec(tp_sym->name, tp_sym->index, tp_sym->id,
                                        tp_sym->owner->name);
        }
        type_spec_free(_ts);
        vector_destroy(tp_args);
        log_type_spec(ret);
        return ret;

    } else if (sym->kind == SYM_CLASS || sym->kind == SYM_TRAIT) {
        KlassSymbol *kls_sym = (KlassSymbol *)sym;
        if (vector_size(&kls_sym->tps) != vector_size(tp_args)) {
            if (strcmp(sym->name, "tuple")) {
                // don't check tuple type paramaters
                kl_error(_ts->loc,
                         "Type argument mismatch: '%s' expects %d argument(s), "
                         "but %d were "
                         "provided",
                         _ts->unresolved.name.name, vector_size(&kls_sym->tps),
                         vector_size(tp_args));
                vector_destroy(tp_args);
                return NULL;
            }
        }

        if (sym->status == SYM_UNRESOLVED) {
            log_info("symbol '%s' is not resolved yet, try to resolve it NOW", kls_sym->name);
            if (sym->ps) {
                ParserState *_ps = sym->ps;
                if (ps != _ps) {
                    log_info("resolve symbol '%s' in '%s'", kls_sym->name, _ps->filename);
                    kl_parse_ast(_ps);
                } else {
                    log_info("currently resolving symbol '%s' in '%s'", kls_sym->name,
                             ps->filename);
                    parse_klass_meta(ps, sym->arg);
                }
            } else {
                UNREACHABLE();
            }
        } else if (sym->status == SYM_RESOLVING) {
            kl_error(_ts->loc, "circular dependency detected when resolving '%s'", kls_sym->name);
            vector_destroy(tp_args);
            return NULL;
        } else {
            log_info("symbol '%s' is already resolved", kls_sym->name);
            // do nothing
        }

        if (open) {
            // open generic_ref type
            log_info("resolve open generic_ref type '%s'", _ts->unresolved.name.name);

            InstanceSymbol *inst_sym = find_or_add_instance(ps->pm->stbl, sym, tp_args);
            vector_destroy(tp_args);
            if (!inst_sym) {
                kl_error(_ts->loc, "failed to get instance for generic_ref type");
                return NULL;
            }
            TypeSpec *ret = inst_sym->instance_ts;
            type_spec_free(_ts);
            log_type_spec(ret);
            return ret;
        }

        log_info("resolve closed generic_ref type '%s'", _ts->unresolved.name.name);

        // closed generic_ref type
        if (vector_empty(tp_args)) {
            log_info("resolve type '%s' without type-args", _ts->unresolved.name.name);
            TypeSpec *ret = kls_sym->instance_ts;
            type_spec_free(_ts);
            vector_destroy(tp_args);
            log_type_spec(ret);
            return ret;
        } else {
            // all args are concrete types and create instance symbol
            log_info("resolve type '%s' with type-args", _ts->unresolved.name.name);
            InstanceSymbol *inst_sym = find_or_add_instance(ps->pm->stbl, sym, tp_args);
            vector_destroy(tp_args);
            if (!inst_sym) {
                kl_error(_ts->loc, "failed to get instance for generic_ref type");
                return NULL;
            }
            TypeSpec *ret = inst_sym->instance_ts;
            type_spec_free(_ts);
            log_type_spec(ret);
            return ret;
        }
    } else {
        UNREACHABLE();
    }
}

// check arg is satified by up-bounds.
int check_type_constraints(Vector *bounds, TypeSpec *arg)
{
    // no up-bounds
    if (!bounds || vector_size(bounds) == 0) return 1;

    for (int i = 0; i < vector_size(bounds); i++) {
        TypeSpec *bound = vector_get(bounds, i);
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

    // Only TYPE_GENERIC_REF requires complex validation of its arguments.
    if (type->kind != TYPE_GENERIC_REF) {
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

    Vector *tps = &sym->tps;
    int arg_count = vector_size(type->generic_ref.args);
    if (vector_size(tps) != arg_count) {
        kl_error(type->loc,
                 "Type argument count mismatch: '%s' expects %d argument(s), "
                 "but %d were "
                 "provided",
                 sym->name, vector_size(tps), arg_count);
        return 0;
    }

    // 2. Validate each generic argument against its defined constraints
    for (int i = 0; i < arg_count; i++) {
        TypeSpec *arg = vector_get(type->generic_ref.args, i);

        // Get the required bounds for the i-th parameter (e.g., [Animal,
        // Serializable])
        TypeParamSymbol *tp_sym = vector_get(tps, i);
        Vector *bounds = &tp_sym->bound;

        if (bounds && vector_size(bounds) > 0) {
            // Check if the provided 'arg' satisfies all upper bounds.
            // Since this is a constraint check, we use is_type_compatible.
            if (!check_type_constraints(bounds, arg)) {
                kl_error(arg->loc,
                         "Type constraint violation: argument #%d does not "
                         "satisfy bounds",
                         i + 1);
                return 0;
            }
        }

        // 3. Core: Recursively validate the argument itself (for nested
        // generics) This ensures Map[String, List[InvalidType]] is caught.
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

static Symbol *_add_field(ParserState *ps, HashMap *stbl, VarDeclStmt *var)
{
    Ident *id = &var->id;
    Symbol *sym;

    int flags = parse_flags(&var->flags);
    if (var->which == VAR_DECL_VAR) flags |= SYM_FLAGS_MUTABLE;
    if (var->which == VAR_DECL_CONST) flags |= SYM_FLAGS_CONST;

    sym = stbl_add_var(stbl, id->name, NULL, flags);

    if (!sym) {
        kl_error(id->loc, "redefinition of '%s'", id->name);
        return NULL;
    }

    var->sym = sym;
    ((VarSymbol *)sym)->scope = VAR_SCOPE_FIELD;
    return sym;
}

static Symbol *_add_local(ParserState *ps, HashMap *stbl, VarDeclStmt *var)
{
    Ident *id = &var->id;
    Symbol *sym;

    int flags = parse_flags(&var->flags);
    if (var->which == VAR_DECL_VAR) flags |= SYM_FLAGS_MUTABLE;
    if (var->which == VAR_DECL_CONST) flags |= SYM_FLAGS_CONST;

    sym = stbl_add_var(stbl, id->name, var->type, flags);

    if (!sym) {
        kl_error(id->loc, "redefinition of '%s'", id->name);
        return NULL;
    }

    var->sym = sym;
    ((VarSymbol *)sym)->scope = VAR_SCOPE_LOCAL;
    return sym;
}

static void check_type_in_first_chain(ParserState *ps, TypeSpec *src, TypeSpec *dst)
{
    if (src->kind != TYPE_KLASS || dst->kind != TYPE_KLASS) return;

    KlassSymbol *src_sym = get_symbol_by_id(src->sym_id);
    KlassSymbol *dst_sym = get_symbol_by_id(dst->sym_id);

    int index = 0;
    TypeSpec *ts;
    vector_foreach(ts, &src_sym->bases) {
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

    log_info("parse variable declaration '%s'", id->name);

    if (sym->status != SYM_UNRESOLVED) {
        log_info("variable '%s' is resolving or resolved.", sym->name);
        return;
    }

    sym->status = SYM_RESOLVING;

    if (ts) {
        ts = resolve_type(ps, ts);
        if (!check_type(ps, ts)) return;
        var->type = ts;
        sym->ts = ts;
    }

    if (!exp) {
        if (!ts) {
            kl_error(id->loc, "variable '%s' needs a type or an initializer", id->name);
            sym->status = SYM_RESOLVED;
            return;
        }

        ((VarSymbol *)var->sym)->ts = ts;
        log_info("variable '%s' type set as:", id->name);
        log_type_spec(ts);
        sym->status = SYM_RESOLVED;
        return;
    }

    exp->ctx = EXPR_CTX_LOAD;
    exp->expected = ts;
    parser_visit_expr(ps, exp);
    if (!exp->ts) return;

    if (exp->kind == EXPR_LITERAL_KIND) {
        Literal *lit = expr_to_literal(exp);
        sym->lit = lit;
    }

    if (!ts) {
        /* update symbol type */
        if (exp->ts->kind == TYPE_NO_TYPE) {
            kl_error(exp->loc, "expr has not value");
            sym->status = SYM_RESOLVED;
            return;
        }

        sym->ts = exp->ts;
        log_info("update symbol '%s' type as:", sym->name);
        log_type_spec(sym->ts);

        if (type_is_optional(sym->ts) && sym->lit && sym->lit->which == LIT_NONE) {
            // the literal is none, the subtype of optional is null, report error.
            kl_error(exp->loc, "cannot assign 'null' to variable '%s' without explicit type",
                     sym->name);
        }
    } else {
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

static void check_func_prefix(ParserState *ps, FuncDeclStmt *fn)
{
    PrefixFlags *flags = &fn->flags;

    AtFlag *at = &flags->at;
    if (!at->ident) return;

    if (str_equal(at->ident, "intrinsic")) {
        if (at->assoc_ident) {
            kl_error(at->id_loc,
                     "'intrinsic' annotation should not have a native func name in top func '%s'",
                     fn->id.name);
        }

        if (!vector_empty(fn->body)) {
            kl_error(at->id_loc, "func '%s' with 'intrinsic' annotation needs empty body.",
                     fn->id.name);
        }

        return;
    }

    if (str_equal(at->ident, "native")) {
        if (!vector_empty(fn->body)) {
            kl_error(at->id_loc, "func '%s' with 'native' annotation needs empty body.",
                     fn->id.name);
        }

        return;
    }
}

static char *func_native_name(FuncDeclStmt *fn)
{
    PrefixFlags *flags = &fn->flags;
    AtFlag *at = &flags->at;
    if (!at->ident) return NULL;
    if (!str_equal(at->ident, "native")) return NULL;
    return at->assoc_ident;
}

// only add function symbol and don't add parameters and return type.
static Symbol *_add_func(ParserState *ps, HashMap *stbl, FuncDeclStmt *fn)
{
    Ident *id = &fn->id;
    Symbol *sym;

    int flags = parse_flags(&fn->flags);
    char *ann = fn->flags.at.ident;
    char *ann_key = fn->flags.at.assoc_ident;
    sym = stbl_add_func(stbl, id->name, NULL, NULL, flags);

    if (!sym) {
        kl_error(id->loc, "redefinition of '%s'", id->name);
        return NULL;
    }

    FuncSymbol *fn_sym = (FuncSymbol *)sym;

    // add tps
    TypeParamDecl *tp;
    vector_foreach(tp, fn->tps) {
        if (!tp) continue;
        TypeParamSymbol *tp_sym = stbl_add_type_param(sym->stbl, tp->id.name, sym);
        tp_sym->index = vector_size(&fn_sym->tps);
        if (tp->which == TP_DECL_INFER) {
            tp_sym->which = TP_INFER;
        } else {
            ASSERT(tp->which == TP_DECL_NORMAL);
            tp_sym->which = TP_NORMAL;
        }
        vector_push_back(&fn_sym->tps, &tp_sym);
    }

    fn->sym = sym;
    return sym;
}

// TODO: Don't remove unreachable statements, opt will handle it.
static void remove_unreachable(Vector *stmts, int from_index)
{
    int size = vector_size(stmts);
    for (int i = from_index; i < size; i++) {
        Stmt *stmt = vector_get(stmts, i);
        if (!stmt) continue;
        stmt_free(stmt);
        log_warn("remove unreachable statement at index %d", i);
    }
    vector_clear_to_end(stmts, from_index);
}

static int is_terminal(Stmt *stmt)
{
    if (!stmt) return 0;

    if (stmt->kind == STMT_RETURN_KIND) return 1;
    if (stmt->kind == STMT_BREAK_KIND) return 1;
    if (stmt->kind == STMT_CONTINUE_KIND) return 1;

    if (stmt->kind == STMT_BLOCK_KIND) {
        BlockStmt *block = (BlockStmt *)stmt;
        return block->has_terminal;
    }

    return 0;
}

static void parse_block(ParserState *ps, Vector *stmts, int *has_terminal)
{
    int depth = 0;
    int index = 0;
    Stmt *s;
    vector_foreach(s, stmts) {
        if (!s) continue;

        parse_stmt(ps, s);

        if (ps->errors >= MAX_ERRORS) break;

        // this is only for 'Early Return' case
        ParserScope *sc = ps->scope;
        if (!vector_empty(&ps->shadows)) {
            log_trace(
                "scope-%d-'%s' has %d shadow variables, move them to "
                "shadow_scope(depth=%d).",
                sc->depth, sc->name, vector_size(&ps->shadows), depth);
            ParserScope *_sc = enter_scope(ps, SCOPE_BLOCK, ONLY_BLOCK, "shadow_scope");
            depth++;
            ShadowVarSymbol *_sym;
            vector_foreach(_sym, &ps->shadows) {
                if (!_sym) continue;
                if (_sym->is_null) {
                    stbl_add_shadow_var(_sc->stbl, _sym->origin, 0);
                    log_trace("add shadow variable '%s' as non-null.", _sym->name);
                } else {
                    stbl_add_shadow_var(_sc->stbl, _sym->origin, 1);
                    log_trace("add shadow variable '%s' as null.", _sym->name);
                }
            }
            vector_clear(&ps->shadows);
        }

        index++;

        if (is_terminal(s)) {
            if (has_terminal) *has_terminal = 1;
            if (index < vector_size(stmts)) {
                log_trace("there are more statements after a terminal statement");
                // TODO: Don't remove unreachable statements, opt will handle
                // it.
                remove_unreachable(stmts, index);
                goto exit;
            }
        }

        if (s->kind == STMT_BLOCK_KIND) {
            BlockStmt *b = (BlockStmt *)s;
            if (b->has_terminal) {
                if (has_terminal) *has_terminal = 1;
                if (index < vector_size(stmts)) {
                    log_trace(
                        "there are more statements after a block with a "
                        "terminal "
                        "statement");
                    // TODO: Don't remove unreachable statements, opt will
                    // handle it.
                    remove_unreachable(stmts, index);
                    goto exit;
                }
            }
        }
    }

exit:
    while (depth > 0) {
        exit_scope(ps);
        log_trace("exit shadow_scope(depth=%d) for early return handling.", depth);
        depth--;
    }
}

static void check_param_name_with_field_name(ParserState *ps, Vector *params)
{
    if (!params || vector_size(params) == 0) return;

    ParserScope *sc = ps->scope;
    if (!sc || sc->kind != SCOPE_CLASS) return;

    Symbol *sym = sc->sym;
    if (!sym || sym->kind != SYM_CLASS) return;

    KlassSymbol *kls_sym = (KlassSymbol *)sym;

    VarSymbol *field;
    vector_foreach(field, kls_sym->fields) {
        if (!field) continue;
        ParamDecl *param;
        vector_foreach(param, params) {
            if (!param) continue;
            if (strcmp(field->name, param->id.name) == 0) {
                kl_error(param->id.loc, "parameter '%s' conflicts with field name in class '%s'",
                         param->id.name, kls_sym->name);
            }
        }
    }
}

static void parse_func_decl(ParserState *ps, Stmt *stmt)
{
    FuncDeclStmt *fn = (FuncDeclStmt *)stmt;
    ParserScope *sc = ps->scope;

    check_func_prefix(ps, fn);

    FuncSymbol *sym = (FuncSymbol *)fn->sym;
    sym->native_name = func_native_name(fn);

    log_info("parse func '%s' body", sym->name);

    check_param_name_with_field_name(ps, fn->args);

    sc = enter_scope(ps, SCOPE_FUNC, 0, sym->name);
    sc->stbl = sym->stbl;
    sc->sym = (Symbol *)sym;

    /* parse body */
    parse_block(ps, fn->body, NULL);

    ASSERT(vector_empty(&ps->shadows));

    exit_scope(ps);
}

static void parse_expr(ParserState *ps, Stmt *stmt)
{
    ExprStmt *s = (ExprStmt *)stmt;
    Expr *exp = s->exp;
    if (exp) {
        exp->ctx = EXPR_CTX_LOAD;
        parser_visit_expr(ps, exp);
    }
}

static void parse_block_stmt(ParserState *ps, Stmt *stmt)
{
    BlockStmt *s = (BlockStmt *)stmt;
    enter_scope(ps, SCOPE_BLOCK, ONLY_BLOCK, "inner-block");
    parse_block(ps, s->stmts, &s->has_terminal);
    ASSERT(vector_empty(&ps->shadows));
    exit_scope(ps);
}

static void unwrap_optional(ParserState *ps, Expr *exp, Vector *shadows)
{
    if (!exp) return;

    Expr *e = exp;

    while (e->kind == EXPR_UNARY_KIND) {
        UnaryExpr *unary = (UnaryExpr *)e;
        if (unary->op != UNARY_NOT) {
            // restore original expr
            e = exp;
            break;
        }
        e = unary->exp;
    }

    if (!expr_is_binary(e)) return;

    BinaryExpr *bexp = (BinaryExpr *)e;
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
        log_trace("lhs is var(optional) and rhs is null literal");
        if (op == BINARY_NEQ) {
            log_trace("'%s' is optional, add shadow variable to stbl", lhs_sym->name);
            log_type_spec(lhs->ts->opt.src);
            sym = stbl_add_shadow_var(sc->stbl, lhs->sym, 0);
        } else if (op == BINARY_EQ) {
            log_trace("'%s' is optional, add shadow variable(null) to stbl", lhs_sym->name);
            log_type_spec(lhs->ts);
            sym = stbl_add_shadow_var(sc->stbl, lhs->sym, 1);
        } else {
            UNREACHABLE();
        }
    } else if (expr_is_literal_null(lhs) && rhs_sym && (rhs_sym->kind == SYM_VAR) &&
               type_is_optional(rhs->ts)) {
        log_trace("lhs is null literal and rhs is var(optional)");
        if (op == BINARY_NEQ) {
            log_trace("'%s' is optional, add shadow variable to stbl", rhs_sym->name);
            log_type_spec(rhs->ts->opt.src);
            sym = stbl_add_shadow_var(sc->stbl, rhs->sym, 0);
        } else if (op == BINARY_EQ) {
            log_trace("'%s' is optional, add shadow variable(null) to stbl", rhs_sym->name);
            log_type_spec(rhs->ts);
            sym = stbl_add_shadow_var(sc->stbl, rhs->sym, 1);
        } else {
            UNREACHABLE();
        }
    } else {
        // do nothing
    }

    if (sym) {
        // for merging to parent-scope
        if (shadows) {
            log_trace("added shadow variable '%s' in scope '%s'(vector)", sym->name, sc->name);
            vector_push_back(shadows, &sym);
        }
    }
}

static void parse_if(ParserState *ps, Stmt *stmt)
{
    IfStmt *s = (IfStmt *)stmt;
    Expr *cond = s->cond;

    cond->ctx = EXPR_CTX_LOAD;
    parser_visit_expr(ps, cond);
    if (!cond->ts) return;

    if (cond->ts->kind != TYPE_BOOL) {
        kl_error(cond->loc, "if condition must be boolean type.");
    }

    ParserScope *sc = enter_scope(ps, SCOPE_BLOCK, IF_BLOCK, "if-block");
    Vector shadows = VECTOR_INIT_PTR;
    unwrap_optional(ps, cond, &shadows);
    int has_terminal = 0;
    parse_block(ps, s->block, &has_terminal);
    ASSERT(vector_empty(&ps->shadows));
    exit_scope(ps);

    if (s->_else) {
        sc = enter_scope(ps, SCOPE_BLOCK, ELSE_BLOCK, "else-block");

        // inherit shadow vars from if-block
        if (!vector_empty(&shadows)) {
            log_trace("inherit shadow vars from if-block to else-block");
            ShadowVarSymbol *sym;
            vector_foreach(sym, &shadows) {
                ASSERT(sym->kind == SYM_SHADOW_VAR);
                stbl_add_shadow_var(sc->stbl, (Symbol *)sym->origin, !sym->is_null);
                log_trace("added shadow variable '%s'(%s) in scope '%s'", sym->name,
                          sym->is_null ? "null" : "non-null", sc->name);
            }
            // finalize vector only
            vector_fini(&shadows);
        } else {
            log_trace("no shadow vars in if-block to inherit");
        }

        ASSERT(s->_else->kind == STMT_BLOCK_KIND || s->_else->kind == STMT_IF_KIND);

        if (s->_else->kind == STMT_BLOCK_KIND) {
            parse_block(ps, ((BlockStmt *)s->_else)->stmts, NULL);
        } else {
            parse_if(ps, s->_else);
        }

        exit_scope(ps);
    } else {
        // no else block
        // only for 'Early Return' case
        /*
        func fn1() {
            let v int? = 0
            if v == null { return }
            v + 100
        }
        */
        log_trace("NO ELSE BLOCK");
        if (!vector_empty(&shadows)) {
            if (has_terminal) {
                ParserScope *_sc = ps->scope;
                log_trace("if-block is terminal, saved shadows to parent scope(%s)", _sc->name);
                Symbol *sym;
                vector_foreach(sym, &shadows) {
                    log_trace("move shadow variable '%s' to parent scope", sym->name);
                    vector_push_back(&ps->shadows, &sym);
                }
                // finalize vector only
                vector_fini(&shadows);

            } else {
                log_trace(
                    "if-block does not have terminal, discard shadow vars from "
                    "if-block");
#ifndef NDEBUG
                Symbol *sym;
                vector_foreach(sym, &shadows) {
                    log_trace("discard shadow variable '%s'", sym->name);
                }
#endif
                // finalize vector only
                vector_fini(&shadows);
            }
        }
    }
}

static void parse_for(ParserState *ps, Stmt *stmt)
{
    ForStmt *s = (ForStmt *)stmt;
    Vector *ids = s->ids;
    Expr *it = s->iterable;

    it->ctx = EXPR_CTX_LOAD;
    parser_visit_expr(ps, it);
    if (!it->ts) return;

    TypeSpec *elem_ts = NULL;

    if (type_is_tuple(it->ts)) {
        // special handling for tuple unpacking
        Symbol *sym = get_symbol_by_id(it->ts->sym_id);
        ASSERT(sym->kind == SYM_INSTANCE);
        InstanceSymbol *inst_sym = (InstanceSymbol *)sym;
        ASSERT(str_equal(inst_sym->origin->name, "tuple"));
        elem_ts = sym->arg;
        ASSERT(elem_ts);
    } else {
        match_iterable(it->ts, NULL, &elem_ts);
        if (!elem_ts) {
            kl_error(it->loc, "type '%s' is not iterable", it->ts->signature);
            return;
        }
    }

    Vector locals;
    vector_init(&locals, sizeof(IdentType));

    if (vector_size(ids) > 1) {
        Symbol *elem_sym = get_symbol_by_id(elem_ts->sym_id);

        if (elem_sym->kind == SYM_INSTANCE) {
            InstanceSymbol *inst_sym = (InstanceSymbol *)elem_sym;
            Symbol *origin_sym = inst_sym->origin;
            if (str_equal(origin_sym->name, "tuple")) {
                // special handling for tuple unpacking
                if (vector_size(ids) != vector_size(inst_sym->tp_args)) {
                    kl_error(it->loc,
                             "num of vars in for does not match num of "
                             "elements in tuple");
                } else {
                    for (int i = 0; i < vector_size(ids); i++) {
                        Ident *id = vector_get_ptr(ids, i);
                        TypeSpec *ts = vector_get(inst_sym->tp_args, i);
                        IdentType id_type = { *id, ts };
                        vector_push_back(&locals, &id_type);
                    }
                }
                goto __do_for_body;
            }
        }

        kl_error(it->loc,
                 "iterable element type '%s' for '%s' is not tuple for multiple vars of "
                 "for loop",
                 elem_ts->signature, it->ts->signature);
    } else {
        Ident *id = vector_get_ptr(ids, 0);
        IdentType id_type = { *id, elem_ts };
        vector_push_back(&locals, &id_type);
    }

__do_for_body:
    ParserScope *sc = enter_scope(ps, SCOPE_BLOCK, FOR_BLOCK, "for-block");

    IdentType *id_type;
    vector_foreach_ptr(id_type, &locals) {
        Symbol *sym = stbl_add_var(sc->stbl, id_type->id.name, id_type->ts, SYM_FLAGS_MUTABLE);
        ASSERT(sym);
        ((VarSymbol *)sym)->scope = VAR_SCOPE_LOCAL;
        log_info("added loop variable '%s' with type '%s'", sym->name, id_type->ts->signature);
        vector_push_back(&s->sym_ids, &sym->id);
    }
    vector_fini(&locals);

    parse_block(ps, s->block, NULL);

    exit_scope(ps);
}

static void parse_if_let(ParserState *ps, Stmt *stmt)
{
    IfLetStmt *s = (IfLetStmt *)stmt;
    Ident *id = &s->id;
    Expr *cond = s->cond;

    cond->ctx = EXPR_CTX_LOAD;
    parser_visit_expr(ps, cond);
    if (!cond->ts) return;

    if (!type_is_optional(cond->ts)) {
        kl_error(cond->loc, "if-let condition must be optional type.");
    }

    ParserScope *sc = enter_scope(ps, SCOPE_BLOCK, IF_LET_BLOCK, "if-let-block");
    Symbol *sym = stbl_add_var(sc->stbl, id->name, cond->ts->opt.src, 0);
    ASSERT(sym);
    ((VarSymbol *)sym)->scope = VAR_SCOPE_LOCAL;
    s->sym = sym;

    parse_block(ps, s->block, NULL);
    ASSERT(vector_empty(&ps->shadows));

    exit_scope(ps);

    // desugar to if stmt with shadow variable
    /*
    if let v = opt {
        // body can use 'v' which is non-optional type
    }
    -->
    if opt != null {
        shadow var v = opt // v is non-null
        // body
    }
    else {
        // there is no 'v' in else body
    }
    */

    if (s->_else) {
        sc = enter_scope(ps, SCOPE_BLOCK, ELSE_BLOCK, "else-block");
        ASSERT(s->_else->kind == STMT_BLOCK_KIND || s->_else->kind == STMT_IF_KIND ||
               s->_else->kind == STMT_IF_LET_KIND);

        if (s->_else->kind == STMT_BLOCK_KIND) {
            parse_block(ps, ((BlockStmt *)s->_else)->stmts, NULL);
        } else if (s->_else->kind == STMT_IF_KIND) {
            parse_if(ps, s->_else);
        } else {
            parse_if_let(ps, s->_else);
        }

        exit_scope(ps);
    }
}

static void parse_while(ParserState *ps, Stmt *stmt)
{
    WhileStmt *s = (WhileStmt *)stmt;
    Expr *cond = s->cond;

    if (cond != NULL) {
        cond->ctx = EXPR_CTX_LOAD;
        parser_visit_expr(ps, cond);
        if (!cond->ts) return;

        if (!type_is_bool(cond->ts)) {
            kl_error(cond->loc, "while condition must be boolean type.");
        }
    }

    enter_scope(ps, SCOPE_BLOCK, WHILE_BLOCK, "while-block");
    unwrap_optional(ps, cond, NULL);
    parse_block(ps, s->block, NULL);
    ASSERT(vector_empty(&ps->shadows));
    exit_scope(ps);
}

static void parse_while_let(ParserState *ps, Stmt *stmt)
{
    WhileLetStmt *s = (WhileLetStmt *)stmt;
    Ident *id = &s->id;
    Expr *cond = s->cond;

    cond->ctx = EXPR_CTX_LOAD;
    parser_visit_expr(ps, cond);
    if (!cond->ts) return;

    if (!type_is_optional(cond->ts)) {
        kl_error(cond->loc, "while-let condition must be optional type.");
    }

    ParserScope *sc = enter_scope(ps, SCOPE_BLOCK, WHILE_LET_BLOCK, "while-let-block");
    Symbol *sym = stbl_add_var(sc->stbl, id->name, cond->ts->opt.src, 0);
    ASSERT(sym);
    ((VarSymbol *)sym)->scope = VAR_SCOPE_LOCAL;
    s->sym = sym;

    parse_block(ps, s->block, NULL);
    ASSERT(vector_empty(&ps->shadows));

    exit_scope(ps);
}

// only add klass/trait symbol and add tp, fields and methods
static Symbol *_add_klass(ParserState *ps, HashMap *stbl, KlassDeclStmt *kls, int is_trait)
{
    Ident *id = &kls->id;
    Symbol *sym;

    int flags = parse_flags(&kls->flags);

    sym = (Symbol *)stbl_add_klass(stbl, id->name, flags, is_trait);

    if (!sym) {
        kl_error(id->loc, "redefinition of '%s'", id->name);
        return NULL;
    }

    sym->path = ps->pm->pkg_path;
    KlassSymbol *kls_sym = (KlassSymbol *)sym;

    // add tps
    TypeParamDecl *tp;
    vector_foreach(tp, kls->tps) {
        if (!tp) continue;
        TypeParamSymbol *tp_sym = stbl_add_type_param(sym->stbl, tp->id.name, sym);
        tp_sym->index = vector_size(&kls_sym->tps);
        if (tp->which == TP_DECL_INFER) {
            tp_sym->which = TP_INFER;
        } else if (tp->which == TP_DECL_CONST) {
            tp_sym->which = TP_CONST;
            tp_sym->const_type = tp->const_type;
        } else {
            ASSERT(tp->which == TP_DECL_NORMAL);
            tp_sym->which = TP_NORMAL;
        }
        vector_push_back(&kls_sym->tps, &tp_sym);
    }

    // add __init__ function for class, if there is not __init__ in class
    if (!is_trait) {
        int has_init = 0;

        Stmt *stmt;
        vector_foreach(stmt, kls->stmts) {
            if (!stmt) continue;
            if (stmt->kind == STMT_FUNC_KIND) {
                FuncDeclStmt *fn = (FuncDeclStmt *)stmt;
                if (str_equal(fn->id.name, "__init__")) {
                    has_init = 1;
                    break;
                }
            }
        }

        if (!has_init) {
            log_info("add default __init__ for class '%s'", id->name);
            Ident id = { .name = "__init__", .loc = kls->id.loc };
            Stmt *init_fn = stmt_from_func_decl(id, NULL, NULL, NULL);
            if (kls->stmts == NULL) {
                kls->stmts = vector_create_ptr();
            }
            vector_push_back(kls->stmts, &init_fn);
        }
    }

    Stmt *stmt;
    vector_foreach(stmt, kls->stmts) {
        if (!stmt) continue;
        if (stmt->kind == STMT_VAR_KIND) {
            Symbol *var = _add_field(ps, sym->stbl, (VarDeclStmt *)stmt);
            if (var) vector_push_back(kls_sym->fields, &var);
        } else if (stmt->kind == STMT_FUNC_KIND) {
            Symbol *fn = _add_func(ps, sym->stbl, (FuncDeclStmt *)stmt);
            if (fn) {
                vector_push_back(kls_sym->funcs, &fn);
                if (str_equal(fn->name, "__init__")) {
                    kls_sym->__init__ = fn;
                }
            }
        } else {
            UNREACHABLE();
        }
    }

    // add typespec to klass symbol
    kls_sym->instance_ts = kls->ts;
    ASSERT(kls->ts->sym_id < 0);
    kls->ts->sym_id = kls_sym->id;

    TypeSpec *ts = type_type_spec();
    kls_sym->ts = ts;

    kls->sym = sym;
    return sym;
}

static void parse_type_params(ParserState *ps, Vector *tps, Symbol *sym)
{
    Vector *sym_tps = NULL;

    if (sym->kind == SYM_CLASS || sym->kind == SYM_TRAIT) {
        sym_tps = &((KlassSymbol *)sym)->tps;
    } else if (sym->kind == SYM_FUNC) {
        sym_tps = &((FuncSymbol *)sym)->tps;
    } else {
        UNREACHABLE();
    }

    // parse type parameter's bounds
    int index = 0;
    TypeParamSymbol *tp_sym;

    TypeParamDecl *tp;
    vector_foreach(tp, tps) {
        if (!tp) continue;

        tp_sym = vector_get(sym_tps, index);
        index++;

        if (tp->which == TP_DECL_INFER) {
            continue;
        }

        if (tp->which == TP_DECL_CONST) {
            TypeSpec *const_ts = resolve_type(ps, tp->const_type);
            if (!const_ts) continue;
            int r = check_type(ps, const_ts);
            if (!r) continue;
            tp_sym->const_type = const_ts;
            continue;
        }

        if (vector_empty(tp->bound)) continue;

        Vector *vec = &tp_sym->bound;
        TypeSpec *ts;
        vector_foreach(ts, tp->bound) {
            if (!ts) continue;
            ts = resolve_type(ps, ts);
            assert(ts);
            int r = check_type(ps, ts);
            assert(r);
            vector_push_back(vec, &ts);
        }
    }
}

static void parse_bases(ParserState *ps, KlassDeclStmt *kls)
{
    KlassSymbol *sym = (KlassSymbol *)kls->sym;

    /* parse base class and traits */
    if (vector_empty(kls->bases)) return;

    Vector *vec = &sym->bases;
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
                kl_error(ts->loc,
                         "origin symbol '%s' is not trait, only trait can be "
                         "used as base",
                         origin_sym->name);
            } else {
                vector_push_back(vec, &base_ts);
            }
        } else {
            kl_error(ts->loc, "'%s' is not trait, only trait can be used as base", base_sym->name);
        }
    }
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

static int type_exists(Vector *vec, TypeSpec *ts)
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

/* compute primary inheritance path */
static void compute_pip(KlassSymbol *sym)
{
    Vector *pip = &sym->pip;
    if (vector_size(pip) > 0) return;

    TypeSpec *any_ts;

    TypeSpec *base_ts = vector_get(&sym->bases, 0);
    if (!base_ts) {
        any_ts = any_type_spec();
        if (!type_exists(pip, any_ts)) {
            vector_push_back(pip, &any_ts);
        }
        // add itself
        if (!type_exists(pip, sym->instance_ts)) {
            vector_push_back(pip, &sym->instance_ts);
        }
        return;
    }

    KlassSymbol *base_sym = _get_base_sym(base_ts);

    // compute base's pip first
    compute_pip(base_sym);

    any_ts = any_type_spec();
    if (!type_exists(pip, any_ts)) {
        vector_push_back(pip, &any_ts);
    }

    // inherit from base's pip
    TypeSpec *ts;
    vector_foreach(ts, &base_sym->pip) {
        if (!ts) continue;
        // skip any type in pip
        if (type_is_any(ts)) continue;

        if (!type_exists(pip, ts)) {
            vector_push_back(pip, &ts);
        }
    }

    // add itself
    if (!type_exists(pip, sym->instance_ts)) {
        vector_push_back(pip, &sym->instance_ts);
    }
}

static void compute_lro(KlassSymbol *sym)
{
    Vector *lro = &sym->lro;
    if (vector_size(lro) > 0) return;

    TypeSpec *base_ts;
    vector_foreach(base_ts, &sym->bases) {
        if (!base_ts) continue;

        KlassSymbol *base_sym = _get_base_sym(base_ts);

        // compute base's lro first
        compute_lro(base_sym);
    }

    TypeSpec *any_ts = any_type_spec();
    vector_push_back(lro, &any_ts);

    vector_foreach(base_ts, &sym->bases) {
        if (!base_ts) continue;

        KlassSymbol *base_sym = _get_base_sym(base_ts);

        // inherit from base's lro
        TypeSpec *ts;
        vector_foreach(ts, &base_sym->lro) {
            if (!ts) continue;

            // check duplication
            if (!type_exists(lro, ts)) {
                vector_push_back(lro, &ts);
            }
        }
    }

    // add self
    if (!type_exists(lro, sym->instance_ts)) {
        vector_push_back(lro, &sym->instance_ts);
    }
}

static void compute_scm(KlassSymbol *sym)
{
    Vector *scm = &sym->scm;
    if (vector_size(scm) > 0) return;

    TypeSpec *base_ts;
    vector_foreach(base_ts, &sym->lro) {
        if (!base_ts) continue;

        if (!type_exists(&sym->pip, base_ts)) {
            vector_push_back(scm, &base_ts);
        }
    }
}

#ifndef NOLOG
void print_vtbl_info(KlassSymbol *sym)
{
    printf("vtbl info for class/trait '%s':", sym->name);

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

static void compute_vtbl_info(KlassSymbol *sym)
{
    compute_pip(sym);
    compute_lro(sym);
    compute_scm(sym);
    print_vtbl_info(sym);
}

static void parse_klass(ParserState *ps, Stmt *stmt)
{
    KlassDeclStmt *kls = (KlassDeclStmt *)stmt;
    Symbol *sym = kls->sym;

    log_info("parse klass '%s' body", sym->name);

    ScopeKind scope_kind = (kls->kind == STMT_CLASS_KIND) ? SCOPE_CLASS : SCOPE_TRAIT;

    ParserScope *sc = enter_scope(ps, scope_kind, 0, sym->name);
    sc->stbl = sym->stbl;
    sc->sym = sym;

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
        exp->expected = fn_ret;
        exp->ctx = EXPR_CTX_LOAD;
        parser_visit_expr(ps, exp);
        if (!exp->ts) return;

        if (!fn_ret || fn_ret->kind == TYPE_NO_TYPE) {
            kl_error(ret->loc, "function '%s' has no return value.", fn_sym->name);
            return;
        }

        if (!type_spec_compatible(fn_sym->ret, exp->ts)) {
            kl_error(ret->loc, "return type mismatch in function '%s'", fn_sym->name);
        } else {
            log_info("return type check passed in function '%s'", fn_sym->name);
        }
        log_info("  expected type:");
        log_type_spec(fn_sym->ret);
        log_info("  actual type:");
        log_type_spec(exp->ts);
    } else {
        if (fn_ret && fn_ret->kind != TYPE_NO_TYPE) {
            kl_error(ret->loc, "function '%s' needs a return value.", fn_sym->name);
            return;
        }
    }
}

static int parse_simple_assign(ParserState *ps, AssignStmt *assign)
{
    Expr *lhs = assign->lhs;
    Expr *rhs = assign->rhs;
    Symbol *lhs_sym = lhs->sym;

    if (lhs->kind == EXPR_INDEX_KIND) {
        log_info("the lhs of assignment is an index expr, skip type check.");
        return 0;
    }

    if (lhs_sym->kind == SYM_VAR) {
        VarSymbol *var_sym = (VarSymbol *)lhs_sym;
        FuncSymbol *fn_sym = get_current_function(ps);
        if ((var_sym->scope != VAR_SCOPE_FIELD) || strcmp(fn_sym->name, "__init__")) {
            if (!(lhs_sym->flags & SYM_FLAGS_MUTABLE)) {
                kl_error(assign->loc, "cannot assign to immutable variable '%s'", lhs_sym->name);
                return -1;
            }
        }
        goto check_compatiable;
    }

    if (lhs_sym->kind == SYM_SHADOW_VAR) {
        ShadowVarSymbol *shadow_sym = (ShadowVarSymbol *)lhs_sym;
        Symbol *origin = shadow_sym->origin;
        if (!(origin->flags & SYM_FLAGS_MUTABLE)) {
            kl_error(assign->loc, "cannot assign to immutable variable '%s'", lhs_sym->name);
            return -1;
        }

        if (type_is_optional(rhs->ts)) {
            log_warn("type of shadow variable '%s' is changed to optional.", shadow_sym->name);
            log_info("from:");
            log_type_spec(lhs->ts);
            log_info("to:");
            log_type_spec(origin->ts);
            lhs->ts = origin->ts;
            lhs->sym = origin;
            remove_shadow_var(shadow_sym);
        } else {
            log_info("type of shadow variable '%s' is not changed.", shadow_sym->name);
        }

        goto check_compatiable;
    }

    kl_error(assign->loc, "Cannot assign to non-variable.");
    return -1;

check_compatiable:
    if (!type_spec_compatible(lhs->ts, rhs->ts)) {
        if (!type_is_optional(lhs->ts) && expr_is_literal_null(rhs)) {
            kl_error(assign->loc, "Cannot assign null to non-nullable type.");
        } else {
            kl_error(assign->loc, "Types of two sides are not matched.");
        }
        printf("lhs:");
        print_type_spec(lhs->ts);
        printf(" =/= rhs:");
        print_type_spec(rhs->ts);
        printf("\n");
        return -1;
    } else {
        log_info("assignment type check passed.");
        log_info("  declared type:");
        log_type_spec(lhs->ts);
        log_info("  rhs type:");
        log_type_spec(rhs->ts);
        return 0;
    }
}

static char *get_inplace_op_str(AssignOpKind op)
{
    switch (op) {
        case OP_PLUS_ASSIGN:
            return "__iadd__";
        case OP_MINUS_ASSIGN:
            return "__isub__";
        case OP_MULT_ASSIGN:
            return "__imul__";
        case OP_DIV_ASSIGN:
            return "__idiv__";
        case OP_MOD_ASSIGN:
            return "__imod__";
        /* bit operator */
        case OP_AND_ASSIGN:
            return "__iand__";
        case OP_OR_ASSIGN:
            return "__ior__";
        case OP_XOR_ASSIGN:
            return "__ixor__";
        case OP_SHL_ASSIGN:
            return "__ishl__";
        case OP_SHR_ASSIGN:
            return "__ishr__";
        default:
            UNREACHABLE();
            return NULL;
    }
}

static char *get_inplace_binary_op_str(AssignOpKind op)
{
    switch (op) {
        case OP_PLUS_ASSIGN:
            return "__add__";
        case OP_MINUS_ASSIGN:
            return "__sub__";
        case OP_MULT_ASSIGN:
            return "__mul__";
        case OP_DIV_ASSIGN:
            return "__div__";
        case OP_MOD_ASSIGN:
            return "__mod__";
        /* bit operator */
        case OP_AND_ASSIGN:
            return "__and__";
        case OP_OR_ASSIGN:
            return "__or__";
        case OP_XOR_ASSIGN:
            return "__xor__";
        case OP_SHL_ASSIGN:
            return "__shl__";
        case OP_SHR_ASSIGN:
            return "__shr__";
        default:
            UNREACHABLE();
            return NULL;
    }
}

static BiOpKind get_inplace_binary_op(AssignOpKind op)
{
    switch (op) {
        case OP_PLUS_ASSIGN:
            return BINARY_ADD;
        case OP_MINUS_ASSIGN:
            return BINARY_SUB;
        case OP_MULT_ASSIGN:
            return BINARY_MUL;
        case OP_DIV_ASSIGN:
            return BINARY_DIV;
        case OP_MOD_ASSIGN:
            return BINARY_MOD;
        case OP_SHL_ASSIGN:
            return BINARY_SHL;
        case OP_SHR_ASSIGN:
            return BINARY_SHR;
        case OP_AND_ASSIGN:
            return BINARY_BIT_AND;
        case OP_OR_ASSIGN:
            return BINARY_BIT_OR;
        case OP_XOR_ASSIGN:
            return BINARY_BIT_XOR;
        default:
            UNREACHABLE();
            return 0;
    }
}

static int parse_inplace_assign(ParserState *ps, AssignStmt *assign)
{
    Expr *lhs = assign->lhs;
    Expr *rhs = assign->rhs;
    AssignOpKind op = assign->op;

    Symbol *kls_sym = get_symbol_by_id(lhs->ts->sym_id);
    if (kls_sym->kind != SYM_CLASS) {
        kl_error(assign->loc, "inplace assignment is not supported for '%s' type.", kls_sym->name);
        return -1;
    }

    Symbol *op_sym = stbl_get(((KlassSymbol *)kls_sym)->stbl, get_inplace_op_str(op));
    if (!op_sym) {
        kl_error(assign->loc, "inplace operator '%s' is not defined for class '%s'.",
                 get_inplace_op_str(op), kls_sym->name);
        return -1;
    }

    if (op_sym->kind != SYM_FUNC) {
        kl_error(assign->loc, "'%s' in class '%s' is not a function.", get_inplace_op_str(op),
                 kls_sym->name);
        return -1;
    }

    FuncSymbol *fn_sym = (FuncSymbol *)op_sym;
    if (vector_size(fn_sym->params) != 1) {
        kl_error(assign->loc,
                 "inplace operator '%s' in class '%s' must have exactly one "
                 "parameter.",
                 get_inplace_op_str(op), kls_sym->name);
        return -1;
    }

    ArgInfo *arg_info = vector_get(fn_sym->params, 0);
    TypeSpec *param_ts = arg_info->ts;

    if (!type_spec_compatible(param_ts, rhs->ts)) {
        kl_error(assign->loc, "Types of two sides are not matched in inplace assignment.");
        printf("lhs:");
        print_type_spec(param_ts);
        printf(" =/= rhs:");
        print_type_spec(rhs->ts);
        printf("\n");
        return -1;
    }

    log_info("inplace assignment type check passed.");
    log_info("  lhs type:");
    log_type_spec(param_ts);
    log_info("  rhs type:");
    log_type_spec(rhs->ts);

    Symbol *bi_op_sym = stbl_get(((KlassSymbol *)kls_sym)->stbl, get_inplace_binary_op_str(op));
    if (bi_op_sym) {
        log_info(
            "binary operator '%s' is also defined for class '%s', inplace assignment can be "
            "desugared to binary operation.",
            get_inplace_binary_op_str(op), kls_sym->name);
        assign->bin_exp = expr_from_binary(get_inplace_binary_op(op), assign->op_loc, lhs, rhs);
        expr_set_loc(assign->bin_exp, assign->loc);
    } else {
        log_info(
            "binary operator '%s' is not defined for class '%s', inplace assignment cannot be "
            "desugared to binary operation.",
            get_inplace_binary_op_str(op), kls_sym->name);
        assign->bin_exp = NULL;
    }

    assign->fn_sym = fn_sym;

    return 0;
}

static void parse_assign(ParserState *ps, Stmt *stmt)
{
    AssignStmt *assign = (AssignStmt *)stmt;
    AssignOpKind op = assign->op;

    Expr *lhs = assign->lhs;
    Expr *rhs = assign->rhs;

    rhs->ctx = EXPR_CTX_LOAD;
    parser_visit_expr(ps, rhs);

    if (op == OP_ASSIGN) {
        log_info("simple assignment detected.");
        lhs->ctx = EXPR_CTX_STORE;
        lhs->arg = rhs;
        parser_visit_expr(ps, lhs);
        if (!lhs->ts || !rhs->ts) return;
        parse_simple_assign(ps, assign);
    } else {
        log_info("compound assignment detected.");
        lhs->ctx = EXPR_CTX_LOAD_STORE;
        lhs->arg = rhs;
        parser_visit_expr(ps, lhs);
        if (!lhs->ts || !rhs->ts) return;
        Symbol *lhs_sym = lhs->sym;
        if (!(lhs_sym->flags & SYM_FLAGS_MUTABLE)) {
            kl_error(assign->loc, "cannot assign to immutable variable '%s'", lhs_sym->name);
            return;
        }
        parse_inplace_assign(ps, assign);
    }
}

ParserScope *find_loop_scope(ParserState *ps)
{
    ParserScope *sc = ps->scope;
    while (sc) {
        if (sc->block_type == WHILE_BLOCK || sc->block_type == WHILE_LET_BLOCK ||
            sc->block_type == FOR_BLOCK)
            return sc;
        sc = sc->next;
    }
    return NULL;
}

static void parse_break(ParserState *ps, Stmt *s)
{
    ParserScope *sc = find_loop_scope(ps);
    if (!sc) {
        kl_error(s->loc, "break statement must be inside a loop.");
    } else {
        log_info("scope-%d('%s') has break statement", sc->depth, sc->name);
    }
}

static void parse_continue(ParserState *ps, Stmt *s)
{
    ParserScope *sc = find_loop_scope(ps);
    if (!sc) {
        kl_error(s->loc, "continue statement must be inside a loop.");
    } else {
        log_info("scope-%d('%s') has continue statement", sc->depth, sc->name);
    }
}

void parse_stmt(ParserState *ps, Stmt *stmt)
{
    if (!stmt) return;

    /* if errors is greater than MAX_ERRORS, stop parsing */
    if (ps->errors >= MAX_ERRORS) return;

    /* clang-format off */
    static void (*handlers[STMT_MAX_KIND])(ParserState *, Stmt *) = {
        [STMT_IMPORT_KIND]    = parse_import,
        [STMT_VAR_KIND]       = parse_var_decl,
        [STMT_FUNC_KIND]      = parse_func_decl,
        [STMT_CLASS_KIND]     = parse_klass,
        [STMT_TRAIT_KIND]     = parse_klass,
        [STMT_RETURN_KIND]    = parse_return,
        [STMT_ASSIGN_KIND]    = parse_assign,
        [STMT_BREAK_KIND]     = parse_break,
        [STMT_CONTINUE_KIND]  = parse_continue,
        [STMT_EXPR_KIND]      = parse_expr,
        [STMT_BLOCK_KIND]     = parse_block_stmt,
        [STMT_IF_KIND]        = parse_if,
        [STMT_WHILE_KIND]     = parse_while,
        [STMT_FOR_KIND]       = parse_for,
        [STMT_IF_LET_KIND]    = parse_if_let,
        [STMT_WHILE_LET_KIND] = parse_while_let,
    };
    /* clang-format on */

    handlers[stmt->kind](ps, stmt);
}

static void inherit_trait_methods(KlassSymbol *sym, Loc loc, ParserState *ps)
{
    Vector *funcs = vector_create_ptr();

    TypeSpec *base_ts;
    vector_foreach(base_ts, &sym->lro) {
        if (!base_ts) continue;

        Symbol *base_sym = get_symbol_by_id(base_ts->sym_id);
        if (!base_sym) {
            UNREACHABLE();
            continue;
        }

        if (base_sym == (Symbol *)sym) {
            // self type, skip
            continue;
        }

        ASSERT(base_sym->kind == SYM_TRAIT);
        KlassSymbol *trait_sym = (KlassSymbol *)base_sym;

        Symbol *fn_sym;
        vector_foreach(fn_sym, trait_sym->funcs) {
            if (fn_sym->kind != SYM_FUNC) continue;
            // check method name conflict
            Symbol *existing_fn = stbl_get(sym->stbl, fn_sym->name);
            if (existing_fn) {
                kl_error(loc,
                         "method '%s' inherited from trait '%s' conflicts with existing symbol in "
                         "trait '%s'.",
                         fn_sym->name, trait_sym->name, sym->name);
                continue;
            }

            // inherit method
            Symbol *inherited_fn = stbl_add_inherited_func(sym->stbl, fn_sym);
            vector_push_back(funcs, &inherited_fn);
            log_info("inherited method '%s' from trait '%s'", fn_sym->name, trait_sym->name);
        }
    }

    vector_concat(funcs, sym->funcs);
    vector_destroy(sym->funcs);
    sym->funcs = funcs;
}

static void parse_klass_meta(ParserState *ps, KlassDeclStmt *kls)
{
    KlassSymbol *sym = (KlassSymbol *)kls->sym;
    ScopeKind scope_kind = (kls->kind == STMT_CLASS_KIND) ? SCOPE_CLASS : SCOPE_TRAIT;

    log_info("parsing metadata for klass/trait '%s'", sym->name);

    if (sym->status != SYM_UNRESOLVED) {
        log_info("klass/trait '%s' is resolving or resolved.", sym->name);
        return;
    }

    sym->status = SYM_RESOLVING;

    ParserScope *sc = enter_scope(ps, scope_kind, 0, sym->name);
    sc->stbl = sym->stbl;
    sc->sym = (Symbol *)sym;

    // parse type parameter's bounds
    parse_type_params(ps, kls->tps, (Symbol *)sym);

    /* parse base class and traits */
    parse_bases(ps, kls);

    /* compute vtbl info */
    compute_vtbl_info(sym);

    if (sym->kind == SYM_TRAIT) {
        // for trait, inherit methods from base traits
        inherit_trait_methods(sym, kls->loc, ps);
    }

    exit_scope(ps);

    sym->status = SYM_RESOLVED;
}

static void parse_func_meta(ParserState *ps, FuncDeclStmt *fn)
{
    FuncSymbol *sym = (FuncSymbol *)fn->sym;

    log_info("parsing function '%s' meta info", sym->name);

    if (sym->status != SYM_UNRESOLVED) {
        log_info("function '%s' is resolving or resolved.", sym->name);
        return;
    }

    sym->status = SYM_RESOLVING;

    ParserScope *sc = enter_scope(ps, SCOPE_FUNC, 0, sym->name);
    sc->stbl = sym->stbl;
    sc->sym = (Symbol *)sym;

    // parse type parameters
    parse_type_params(ps, fn->tps, (Symbol *)sym);

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
            kl_error(param->id.loc, "after variadic parameter must be the kw parameters.");
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
        arg->has_dfl_val = 0;

        TypeSpec *ts;
        if (param->type) {
            ts = param->type;
            Expr *e = param->value;
            if (e) {
                if (e->kind != EXPR_LITERAL_KIND) {
                    kl_error(param->id.loc, "parameter '%s' needs a literal default value",
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
            ASSERT(e->kind == EXPR_LITERAL_KIND);
            Literal *lit = expr_to_literal(e);
            ((VarSymbol *)s)->lit = lit;
            arg->has_dfl_val = 1;
            arg->dfl_val = lit;
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

    exit_scope(ps);

    sym->status = SYM_RESOLVED;
}

static void parse_klass_func_meta(ParserState *ps, KlassDeclStmt *kls)
{
    KlassSymbol *sym = (KlassSymbol *)kls->sym;

    log_info("parsing klass/trait '%s' methods' meta info", sym->name);

    ScopeKind scope_kind = (kls->kind == STMT_CLASS_KIND) ? SCOPE_CLASS : SCOPE_TRAIT;

    ParserScope *sc = enter_scope(ps, scope_kind, 0, sym->name);
    sc->stbl = sym->stbl;
    sc->sym = (Symbol *)sym;

    Stmt *stmt;
    vector_foreach(stmt, kls->stmts) {
        if (!stmt) continue;
        if (stmt->kind == STMT_FUNC_KIND) {
            parse_func_meta(ps, (FuncDeclStmt *)stmt);
        }
    }

    exit_scope(ps);
}

void kl_parse_ast(ParserState *ps)
{
    if (ps->status != PS_STATUS_UNRESOLVED) {
        log_info("AST '%s' is already resolving or resolved.", ps->filename);
        return;
    }

    ps->status = PS_STATUS_RESOLVING;

    ParserScope *scope = enter_scope(ps, SCOPE_TOP, 0, "top");
    scope->stbl = ps->pm->stbl;

    KlassDeclStmt *kls;
    vector_foreach(kls, &ps->kls_stmts) {
        if (!kls) continue;
        parse_klass_meta(ps, kls);
    }

    vector_foreach(kls, &ps->kls_stmts) {
        if (!kls) continue;
        parse_klass_func_meta(ps, kls);
    }

    FuncDeclStmt *fn;
    vector_foreach(fn, &ps->fn_stmts) {
        if (!fn) continue;
        parse_func_meta(ps, fn);
    }

    Stmt *stmt;
    vector_foreach(stmt, &ps->stmts) {
        if (!stmt) continue;
        parse_stmt(ps, stmt);
    }

    ps->status = PS_STATUS_RESOLVED;

    exit_scope(ps);

    /* If there are errors, stop doing codegen. */
    if (ps->errors) return;

#ifndef NOLOG
    /* dump symbol tables */
    stbl_show(ps->pm->stbl);
#endif
}

static void init_parser_state(ParserState *ps, char *filename)
{
    ps->filename = str_dup(filename);
    vector_init_ptr(&ps->stmts);
    vector_init_ptr(&ps->fn_stmts);
    vector_init_ptr(&ps->kls_stmts);
    vector_init_ptr(&ps->shadows);
    ps->imported = stbl_new();
    INIT_BUF(ps->sbuf);
}

ParserState *new_parser_state(ParserModule *pm, char *path)
{
    FILE *in = fopen(path, "r");
    if (in == NULL) {
        kl_printf_error("%s: No such file or directory\n", path);
        return NULL;
    }

    ParserState *ps = mm_alloc_obj(ps);
    init_parser_state(ps, path);
    ps->pm = pm;
    vector_push_back(&pm->pss, &ps);

    yyscan_t scanner;
    yylex_init_extra(ps, &scanner);
    yyset_in(in, scanner);
    yyparse(ps, scanner);
    yylex_destroy(scanner);

    fclose(in);

    return ps;
}

void free_parser_state(ParserState *ps)
{
    mm_free(ps->filename);

    vector_fini(&ps->kls_stmts);
    vector_fini(&ps->fn_stmts);

    Stmt *s;
    vector_foreach(s, &ps->stmts) {
        stmt_free(s);
    }
    vector_fini(&ps->stmts);

    vector_fini(&ps->shadows);
    FINI_BUF(ps->sbuf);
    mm_free(ps);
}

// only add symbol and do not add its type
static Symbol *_add_global(ParserState *ps, HashMap *stbl, VarDeclStmt *var)
{
    Ident *id = &var->id;
    TypeSpec *ts = var->type;
    Symbol *sym;

    int flags = parse_flags(&var->flags);
    if (var->which == VAR_DECL_VAR) flags |= SYM_FLAGS_MUTABLE;
    if (var->which == VAR_DECL_CONST) flags |= SYM_FLAGS_CONST;

    // don't add unresolved type
    if (ts && ts->kind == TYPE_UNRESOLVED) ts = NULL;
    sym = stbl_add_var(stbl, id->name, ts, flags);

    if (!sym) {
        kl_error(id->loc, "redefinition of '%s'", id->name);
        return NULL;
    }

    var->sym = sym;
    sym->arg = var;
    ((VarSymbol *)sym)->scope = VAR_SCOPE_GLOBAL;
    return sym;
}

void parse_top_stmt(ParserState *ps, Stmt *stmt)
{
    if (!stmt) return;

    Symbol *sym = NULL;

    switch (stmt->kind) {
        case STMT_VAR_KIND: {
            VarDeclStmt *var = (VarDeclStmt *)stmt;
            sym = _add_global(ps, ps->pm->stbl, var);
            if (!sym) return;
            var->where = VAR_GLOBAL;
            sym->ps = ps;
            break;
        }
        case STMT_FUNC_KIND: {
            FuncDeclStmt *fn = (FuncDeclStmt *)stmt;
            sym = _add_func(ps, ps->pm->stbl, fn);
            if (!sym) return;
            vector_push_back(&ps->fn_stmts, &stmt);
            sym->ps = ps;
            sym->arg = fn;
            break;
        }
        case STMT_CLASS_KIND: {
            KlassDeclStmt *kls = (KlassDeclStmt *)stmt;
            sym = _add_klass(ps, ps->pm->stbl, kls, 0);
            if (!sym) return;
            vector_push_back(&ps->kls_stmts, &stmt);
            sym->ps = ps;
            sym->arg = kls;
            break;
        }
        case STMT_TRAIT_KIND: {
            KlassDeclStmt *kls = (KlassDeclStmt *)stmt;
            sym = _add_klass(ps, ps->pm->stbl, kls, 1);
            if (!sym) return;
            vector_push_back(&ps->kls_stmts, &stmt);
            sym->ps = ps;
            sym->arg = kls;
            break;
        }
        case STMT_IMPORT_KIND: {
            ImportStmt *s = (ImportStmt *)stmt;
            ASSERT(s->path);
            PkgSymbol *pkg = import_package(ps->pm, s->path);

            if (s->alias) {
                ASSERT(!s->names);
                stbl_add_imported(ps->imported, (Symbol *)pkg, s->alias);
            } else if (s->names) {
                ASSERT(!s->alias);
                char *pkg_name;
                Vector *names = s->names;
                IdentAsIdent *item;
                vector_foreach_ptr(item, names) {
                    Symbol *origin = stbl_get(pkg->stbl, item->id.name);
                    if (!origin || !(origin->flags & SYM_FLAGS_PUBLIC)) {
                        kl_error(item->id.loc, "cannot import symbol '%s' from package '%s'",
                                 item->id.name, pkg->name);
                        continue;
                    }
                    pkg_name = item->id.name;
                    if (item->alias_id.name) {
                        pkg_name = item->alias_id.name;
                    }
                    stbl_add_imported(ps->imported, origin, pkg_name);
                }
            } else {
                char *pkg_name = strrchr(s->path, '/');
                pkg_name = pkg_name ? pkg_name + 1 : s->path;
                stbl_add_imported(ps->imported, (Symbol *)pkg, pkg_name);
            }
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
