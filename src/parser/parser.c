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

void kl_error_detail(ParserState *ps, Loc *loc) {}

static ParserScope *new_scope(ScopeKind kind, BlockType block)
{
    ParserScope *scope = mm_alloc_obj(scope);
    scope->kind = kind;
    scope->block_type = block;
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
#define print_type_spec(ts) do {        \
    BUF(buf);                           \
    type_spec_print(ts, &buf);          \
    log_info("  '%s'", BUF_STR(buf));   \
    FINI_BUF(buf);                      \
} while (0)
/* clang-format on */
#else
#define print_type_spec(ts) ((void *)(ts))
#endif

#ifndef NOLOG
static const char *scopes[] = {
    "TOP", "CLASS", "TRAIT", "FUNC", "BLOCK", "ANONY",
};

static const char *blocks[] = {
    "UNK",       "BLOCK",       "IF-BLOCK",   "WHILE-BLOCK",
    "FOR-BLOCK", "MATCH-BLOCK", "MATCH-CASE", "MATCH-CLAUSE",
};
#endif

ParserScope *enter_scope(ParserState *ps, ScopeKind kind, BlockType block)
{
    ParserScope *scope = new_scope(kind, block);
    scope->next = ps->scope;
    ps->scope = scope;
    ++ps->depth;

#ifndef NOLOG
    const char *str;
    if (kind != SCOPE_BLOCK)
        str = scopes[kind];
    else
        str = blocks[block];
    log_info("====== Enter scope-%d(%s) ======", ps->depth, str);
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
    log_info("====== Exit scope-%d(%s) ======", ps->depth, str);
#endif

    ps->scope = scope->next;
    free_scope(scope);
    --ps->depth;
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
    vector_foreach_object(base, sym->bases)
    {
        if (is_subtype_of(base->id, parent_id)) return 1;
    }

    return 0;
}

/**
 * Checks if the 'src' type is compatible with the 'dst' type.
 *
 * Rules for High-Performance Type System:
 * 1. Top Type: TYPE_OBJECT is the root and accepts any type.
 * 2. Strict Kind Matching: Except for Object, kinds must match (e.g., no implicit
 * int-to-float).
 * 3. Numerical Widening: For INT/FLOAT, source width <= destination width is allowed.
 *    Note: Semantically compatible but requires explicit 'cast' instructions during
 * codegen due to binary representation (memory layout) mismatch.
 * 4. Generic Variance:
 *    - Reference Types (Classes): Covariant (e.g., List[Dog] -> List[Animal]).
 *    - Value Types (Primitives): Invariant (Generic parameters must be strictly
 * compatible).
 */
int type_spec_compatible(TypeSpec *dst, TypeSpec *src)
{
    if (!dst || !src) return 0;

    if (dst == src) return 1;

    // Rule 1: TYPE_OBJECT is the Top Type (Root of the type hierarchy)
    if (dst->kind == TYPE_OBJECT) return 1;

    // Rule 2: Strict kind matching (Semantic barrier)
    if (dst->kind != src->kind) {
        if (src->kind == TYPE_GENERIC_VAR) {
            // check dst with src's upbound
            TypeParamSymbol *sym = get_symbol_by_id(src->sym_id);
            TypeSpec *bound;
            vector_foreach_object(bound, sym->bound)
            {
                if (type_spec_compatible(dst, bound)) {
                    // Only one bound is compatible, T is compatible with dst.
                    return 1;
                }
            }
        }

        if (dst->kind == TYPE_UNION) {
            TypeSpec *arg;
            vector_foreach_object(arg, dst->union_type.args)
            {
                if (type_spec_compatible(arg, src)) {
                    return 1;
                }
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
    if (dst->sym_id != src->sym_id) {
        // Check if 'src' is a subtype of 'dst' in the symbol table
        return is_subtype_of(src->sym_id, dst->sym_id);
    }

    if (dst->kind == TYPE_GENERIC_VAR) {
        return dst->generic_var.index == src->generic_var.index;
    }

    // Rule 5: Structural Recursion for Specialized Types (Generics)

    if (dst->kind == TYPE_SPECIALIZED) {
        int d_args_size = vector_size(dst->specialized.args);
        int s_args_size = vector_size(src->specialized.args);
        if (d_args_size != s_args_size) return 0;

        // Handle Variance based on storage model

        for (int i = 0; i < d_args_size; i++) {
            TypeSpec *d_arg = vector_get_object(dst->specialized.args, i);
            TypeSpec *s_arg = vector_get_object(src->specialized.args, i);

            // generic parameters must be strictly compatible(invariant).
            // List[int32] and List[int64] are not compatible.
            // List[Dog] and List[Animal] are compatible.
            if (is_value_type(d_arg)) {
                if (!type_spec_equal_strict(d_arg, s_arg)) return 0;
            } else {
                // reference type
                if (!type_spec_compatible(d_arg, s_arg)) return 0;
            }
        }

        return 1;
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
        vector_foreach_object(arg, _ts->union_type.args)
        {
            TypeSpec *ret = resolve_type(ps, arg);
            vector_push_back(vec, &ret);
        }
        type_spec_free(_ts);
        return union_type_spec_intern(vec);
    }

    if (_ts->kind != TYPE_UNRESOLVED) return _ts;

    // parse arguments by bottom-to-up method

    Vector *vec = NULL;
    if (vector_size(_ts->unresolved.args) > 0) {
        vec = vector_create_ptr();
        TypeSpec *ts;
        TypeSpec *ret;
        vector_foreach_object(ts, _ts->unresolved.args)
        {
            ret = resolve_type(ps, ts);
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

        TypeSpec *ret = specialized_type_spec(
            _ts->unresolved.pkg.name, _ts->unresolved.name.name, vec, kls_sym->id);
        type_spec_free(_ts);
        return ret;
    } else {
        UNREACHABLE();
    }

error:
    // TODO: free memroy
    UNREACHABLE();
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
        vector_foreach_object(arg, type->union_type.args)
        {
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

    // 2. Validate each generic argument against its defined constraints
    for (int i = 0; i < arg_count; i++) {
        TypeSpec **arg_p = vector_get(type->specialized.args, i);
        TypeSpec *arg = *arg_p;

        // Get the required bounds for the i-th parameter (e.g., [Animal, Serializable])
        TypeParamSymbol **tp_sym_p = vector_get(tps, i);
        TypeParamSymbol *tp_sym = *tp_sym_p;
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
    if (flags->stat.flag) f |= SYM_FLAGS_STATIC;
    if (flags->final.flag) f |= SYM_FLAGS_FINAL;

    if (flags->at.assoc_ident)
        f |= SYM_FLAGS_TAG_VALUE;
    else if (flags->at.ident)
        f |= SYM_FLAGS_TAG_ONLY;

    return f;
}

static Symbol *_add_var(ParserState *ps, HashMap *stbl, VarDeclStmt *var)
{
    Ident *id = &var->id;
    TypeSpec *ty = var->type;
    Symbol *sym;

    int flags = parse_flags(&var->flags);
    if (!var->ro) flags |= SYM_FLAGS_MUTABLE;
    if (var->exp && var->exp->kind == EXPR_LITERAL_KIND) flags |= SYM_FLAGS_VAR_VALUE;

    sym = stbl_add_var(stbl, id->name, ty, flags);

    if (!sym) {
        kl_error(id->loc, "redefinition of '%s'", id->name);
        return NULL;
    }

    var->sym = sym;
    return sym;
}

static void parse_var_decl(ParserState *ps, Stmt *stmt)
{
    VarDeclStmt *var = (VarDeclStmt *)stmt;
    Ident *id = &var->id;
    TypeSpec *ts = var->type;
    Expr *exp = var->exp;

    exp->ctx = EXPR_CTX_LOAD;
    exp->expected = ts;
    parser_visit_expr(ps, exp);
    if (!exp->ts) return;

    /*
     * If var is global, it is already existed.
     * If var is local, it needs to be added into symbol table.
     */
    if (var->where != VAR_GLOBAL && var->where != VAR_FIELD) {
        ParserScope *sc = ps->scope;
        if (!_add_var(ps, sc->stbl, var)) return;
    }

    VarSymbol *sym = (VarSymbol *)var->sym;

    if (var->where == VAR_GLOBAL) {
        if (exp && exp->kind == EXPR_LITERAL_KIND) {
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
    }

    if (!ts) {
        /* update symbol type */
        sym->ts = exp->ts;
        log_info("update symbol '%s' type as:", sym->name);
        print_type_spec(sym->ts);
    } else {
        if (!type_spec_compatible(exp->ts, ts)) {
            kl_error(id->loc, "Types of two sides are not matched.");
            log_info("lhs:");
            print_type_spec(ts);
            log_info("rhs:");
            print_type_spec(exp->ts);
            return;
        }
    }

    // codegen
    ParserScope *sc = ps->scope;
    if (sc->kind == SCOPE_FUNC) {
        // KlrBuilder bldr;
        // klr_builder_end(&bldr, sc->bb);
        // KlrValue *ir_var = klr_add_local(&bldr, sym->desc, id->name);
        // sym->ir_val = ir_var;
        // klr_build_store(&bldr, ir_var, exp->ir_val);
    }
}

static void check_top_func_flags(ParserState *ps, FuncDeclStmt *fn)
{
    PrefixFlags *flags = &fn->flags;

    if (flags->stat.flag) {
        kl_error(flags->stat.loc, "'static' is not allowed in top func '%s'",
                 fn->id.name);
        return;
    }

    if (flags->final.flag) {
        kl_error(flags->final.loc, "'final' is not allowed in top func '%s'",
                 fn->id.name);
        return;
    }

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

static Symbol *_add_func(ParserState *ps, HashMap *stbl, FuncDeclStmt *fn)
{
    Ident *id = &fn->id;
    TypeSpec *ty = fn->ret ?: no_type_spec();
    Symbol *sym;

    int flags = parse_flags(&fn->flags);
    char *ann = fn->flags.at.ident;
    char *ann_key = fn->flags.at.assoc_ident;
    sym = stbl_add_func(stbl, id->name, fn->tps, ty, NULL, flags, ann, ann_key);

    if (!sym) {
        kl_error(id->loc, "redefinition of '%s'", id->name);
        return NULL;
    }

    fn->sym = sym;
    return sym;
}

static void parse_stmt(ParserState *ps, Stmt *stmt);

static void parse_body(ParserState *ps, FuncSymbol *sym, Vector *stmts)
{
    int sz = vector_size(stmts);
    Stmt **s_p;
    Stmt *s;
    vector_foreach(s_p, stmts) {
        s = *s_p;
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

    if (ps->errors) return;

    FuncSymbol *sym = (FuncSymbol *)fn->sym;

    sc = enter_scope(ps, SCOPE_FUNC, 0);
    sc->stbl = sym->stbl;
    sc->sym = (Symbol *)sym;

    /* add parameters into function symbol table */

    // parse return type
    if (sym->ts) {
        TypeSpec *_ts = sym->ts;
        sym->ts = resolve_type(ps, _ts);
        check_type(ps, sym->ts);
    }

    Vector *args = vector_create_ptr();

    int size = vector_size(fn->args);
    TypeSpec *params[(size + 1)];
    Symbol *arg_syms[size];

    ParamDecl **param_p;
    ParamDecl *param;
    vector_foreach(param_p, fn->args) {
        param = *param_p;

        ArgInfo *arg = mm_alloc_obj_fast(arg);
        arg->name = param->id.name;
        arg->dfl_val_idx = 0;

        TypeSpec *ts;
        if (param->type) {
            ts = param->type;
        } else {
            Expr *e = param->value;
            e->ctx = EXPR_CTX_LOAD;
            parser_visit_expr(ps, e);
            if (!e->ts) return;
            ts = e->ts;
        }

        if (ts) {
            ts = resolve_type(ps, ts);
            // TODO: memory
            assert(ts);
            check_type(ps, ts);
        }

        Symbol *s = stbl_add_var(sc->stbl, param->id.name, ts, 0);

        arg->ts = ts;
        // DESC_INCREF(desc);
        vector_push_back(args, &arg);

        // params[i__] = DESC_INCREF_GET(desc);
        params[i__] = ts;
        arg_syms[i__] = s;
    }

    params[size] = 0;

    sym->params = args;

    // KlrValue *fval = klr_add_func(ps->module, sym->desc, params, fn->id.name);
    // sym->ir_val = fval;
    // KlrBasicBlock *entry = klr_append_block(fval, "entry");
    // sc->bb = entry;

    // for (int i = 0; i < size; i++) {
    //     Symbol *s = arg_syms[i];
    //     s->ir_val = klr_get_param(fval, i);
    // }

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

static Symbol *_add_klass(ParserState *ps, HashMap *stbl, KlassDeclStmt *kls)
{
    Ident *id = &kls->id;
    Symbol *sym;

    int flags = parse_flags(&kls->flags);

    sym = stbl_add_klass(stbl, id->name, flags);

    if (!sym) {
        kl_error(id->loc, "redefinition of '%s'", id->name);
        return NULL;
    }

    KlassSymbol *kls_sym = (KlassSymbol *)sym;

    // add tps
    Vector *vec = NULL;
    if (vector_size(kls->tps) > 0) {
        vec = vector_create_ptr();
        kls_sym->tps = vec;
    }

    TypeParamDecl **tp_p;
    TypeParamDecl *tp;
    vector_foreach(tp_p, kls->tps) {
        tp = *tp_p;
        Symbol *tp_sym = stbl_add_type_param(sym->stbl, tp->id.name, sym);
        vector_push_back(vec, &tp_sym);
        ((TypeParamSymbol *)tp_sym)->index = i__;
    }

    // add fields & methods
    Stmt **stmt_p;
    Stmt *stmt;
    vector_foreach(stmt_p, kls->stmts) {
        stmt = *stmt_p;
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

    // add typespec to class symbol
    TypeSpec *ts = klass_type_spec(NULL, kls->id.name);
    kls_sym->ts = ts;
    ts->sym_id = kls_sym->id;

    kls->sym = sym;
    return sym;
}

static Symbol *_add_trait(ParserState *ps, HashMap *stbl, KlassDeclStmt *kls)
{
    Ident *id = &kls->id;
    Symbol *sym;

    int flags = parse_flags(&kls->flags);
    sym = stbl_add_trait(stbl, id->name, flags);

    if (!sym) {
        kl_error(id->loc, "redefinition of '%s'", id->name);
        return NULL;
    }

    KlassSymbol *kls_sym = (KlassSymbol *)sym;

    // add tps
    Vector *vec = NULL;
    if (vector_size(kls->tps) > 0) {
        vec = vector_create_ptr();
    }
    kls_sym->tps = vec;

    TypeParamDecl **tp_p;
    TypeParamDecl *tp;
    vector_foreach(tp_p, kls->tps) {
        tp = *tp_p;
        Symbol *tp_sym = stbl_add_type_param(sym->stbl, tp->id.name, sym);
        vector_push_back(vec, &tp_sym);
        ((TypeParamSymbol *)tp_sym)->index = i__;
    }

    // add method proto
    Stmt **stmt_p;
    Stmt *stmt;
    vector_foreach(stmt_p, kls->stmts) {
        stmt = *stmt_p;
        if (stmt->kind == STMT_FUNC_KIND) {
            Symbol *fn = _add_func(ps, sym->stbl, (FuncDeclStmt *)stmt);
            if (fn) vector_push_back(kls_sym->funcs, &fn);
        } else {
            UNREACHABLE();
        }
    }

    kls->sym = sym;
    return sym;
}

static void parse_class(ParserState *ps, Stmt *stmt)
{
    KlassDeclStmt *kls = (KlassDeclStmt *)stmt;
    KlassSymbol *sym = (KlassSymbol *)kls->sym;

    ParserScope *sc = enter_scope(ps, SCOPE_CLASS, 0);
    sc->stbl = sym->stbl;
    sc->sym = (Symbol *)sym;

    // parse type parameter's bounds
    TypeParamDecl *tp;
    vector_foreach_object(tp, kls->tps)
    {
        TypeParamSymbol *tp_sym = vector_get_object(sym->tps, i__);

        if (vector_size(tp->bound) > 0) {
            Vector *vec = vector_create_ptr();
            TypeSpec *_ts;
            TypeSpec *ts;
            vector_foreach_object(_ts, tp->bound)
            {
                ts = resolve_type(ps, _ts);
                assert(ts);
                int r = check_type(ps, ts);
                assert(r);
                vector_push_back(vec, &ts);
            }
            tp_sym->bound = vec;
        }
    }

    /* parse base class and traits */
    if (vector_size(kls->bases) > 0) {
        Vector *vec = vector_create_ptr();
        TypeSpec *ts;
        vector_foreach_object(ts, kls->bases)
        {
            TypeSpec *base_ts = resolve_type(ps, ts);
            ASSERT(base_ts);
            int r = check_type(ps, base_ts);
            ASSERT(r);

            Symbol *base_sym = get_symbol_by_id(base_ts->sym_id);
            if (!base_sym) {
                UNREACHABLE();
            }

            if (base_sym->kind == SYM_CLASS) {
                if (i__ != 0) {
                    kl_error(ts->loc, "class '%s' can have only first base class.",
                             sym->name);
                    continue;
                }
                vector_push_back(vec, &base_ts);
            } else if (base_sym->kind == SYM_TRAIT) {
                vector_push_back(vec, &base_ts);
            } else {
                kl_error(ts->loc,
                         "only class or trait can be used as base of class '%s'.",
                         sym->name);
            }
        }
        sym->bases = vec;
    }

    /* parse class body */
    Stmt **s;
    vector_foreach(s, kls->stmts) {
        parse_stmt(ps, *s);
    }

    exit_scope(ps);
}

static void parse_trait(ParserState *ps, Stmt *stmt)
{
    KlassDeclStmt *kls = (KlassDeclStmt *)stmt;
    KlassSymbol *sym = (KlassSymbol *)kls->sym;

    ParserScope *sc = enter_scope(ps, SCOPE_TRAIT, 0);
    sc->stbl = sym->stbl;
    sc->sym = (Symbol *)sym;

    // parse type parameter's bounds
    TypeParamDecl *tp;
    vector_foreach_object(tp, kls->tps)
    {
        TypeParamSymbol *tp_sym = vector_get_object(sym->tps, i__);

        if (vector_size(tp->bound) > 0) {
            Vector *vec = vector_create_ptr();
            TypeSpec *_ts;
            TypeSpec *ts;
            vector_foreach_object(_ts, tp->bound)
            {
                ts = resolve_type(ps, _ts);
                assert(ts);
                int r = check_type(ps, ts);
                assert(r);
                vector_push_back(vec, &ts);
            }
            tp_sym->bound = vec;
        }
    }

    /* parse base class and traits */
    if (vector_size(kls->bases) > 0) {
        Vector *vec = vector_create_ptr();
        TypeSpec *ts;
        vector_foreach_object(ts, kls->bases)
        {
            TypeSpec *base_ts = resolve_type(ps, ts);
            ASSERT(base_ts);
            int r = check_type(ps, base_ts);
            ASSERT(r);

            Symbol *base_sym = get_symbol_by_id(base_ts->sym_id);
            if (!base_sym) {
                UNREACHABLE();
            }

            if (base_sym->kind != SYM_TRAIT) {
                kl_error(ts->loc, "only trait can be used as base of trait '%s'.",
                         sym->name);
                continue;
            }
            vector_push_back(vec, &base_ts);
        }
        sym->bases = vec;
    }

    /* parse class body */
    Stmt **s;
    vector_foreach(s, kls->stmts) {
        parse_stmt(ps, *s);
    }

    exit_scope(ps);
}

static void parse_return(ParserState *ps, Stmt *stmt)
{
    RetStmt *ret = (RetStmt *)stmt;
    Expr *exp = ret->exp;
    KlrValue *ir_val = NULL;
    if (exp) {
        exp->ctx = EXPR_CTX_LOAD;
        parser_visit_expr(ps, exp);
        if (!exp->ts) return;
        ir_val = exp->ir_val;
    }

    // codegen
    ParserScope *sc = ps->scope;

    KlrBuilder bldr;
    klr_builder_end(&bldr, sc->bb);
    if (ir_val)
        klr_build_ret(&bldr, ir_val);
    else
        klr_build_ret_void(&bldr);
}

static void parse_stmt(ParserState *ps, Stmt *stmt)
{
    if (!stmt) return;

    /* if errors is greater than MAX_ERRORS, stop parsing */
    if (ps->errors >= MAX_ERRORS) return;

    /* clang-format off */
    static void (*handlers[])(ParserState *, Stmt *) = {
        NULL,                               /* INVALID          */
        NULL, // parse_import,              /* IMPORT_KIND      */
        parse_var_decl,                     /* VAR_KIND         */
        parse_func_decl,                    /* FUNC_KIND        */
        parse_class,                        /* CLASS_KIND       */
        parse_trait,                        /* TRAIT_KIND       */
        parse_return,                       /* RETURN_KIND      */
        NULL, // parse_assign,              /* ASSIGN_KIND      */
        NULL, // parse_break,               /* BREAK_KIND       */
        NULL, // parse_continue,            /* CONTINUE_KIND    */
        parse_expr,                         /* EXPR_KIND        */
        NULL, // parse_block,               /* BLOCK_KIND       */
        NULL, // parse_if,                  /* IF_KIND          */
        NULL, // parse_while,               /* WHILE_KIND       */
        NULL, // parse_for,                 /* FOR_KIND         */
        NULL, // parse_match,               /* MATCH_KIND       */
    };
    /* clang-format on */

    handlers[stmt->kind](ps, stmt);
}

static void parse_ast(ParserState *ps)
{
    ParserScope *scope = enter_scope(ps, SCOPE_TOP, 0);
    scope->stbl = ps->stbl;
    Stmt **stmt;
    vector_foreach(stmt, &ps->stmts) {
        parse_stmt(ps, *stmt);
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
    ps->stbl = stbl_new();
    ps->builtin = stbl_new();
}

static void init_builtin_module(ParserState *ps)
{
    kl_read_from_klc(ps->builtin, "libs/builtin.klc");
    update_builtin_type_specs(ps->builtin);
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

    init_builtin_module(ps);

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

    if (!ps->errors) {
        codegen_ast(ps);
    }

    if (!ps->errors) {
        kl_write_to_klc(ps);
    }

    free_parser(ps);

    return 0;
}

static void parse_top_stmt(ParserState *ps, Stmt *stmt)
{
    if (!stmt) return;

    Symbol *sym = NULL;

    switch (stmt->kind) {
        case STMT_VAR_KIND: {
            VarDeclStmt *var = (VarDeclStmt *)stmt;
            sym = _add_var(ps, ps->stbl, var);
            if (!sym) goto failed;
            var->where = VAR_GLOBAL;
            break;
        }
        case STMT_FUNC_KIND: {
            FuncDeclStmt *fn = (FuncDeclStmt *)stmt;
            sym = _add_func(ps, ps->stbl, fn);
            if (!sym) goto failed;
            break;
        }
        case STMT_CLASS_KIND: {
            KlassDeclStmt *kls = (KlassDeclStmt *)stmt;
            sym = _add_klass(ps, ps->stbl, kls);
            if (!sym) goto failed;
            break;
        }
        case STMT_TRAIT_KIND: {
            KlassDeclStmt *kls = (KlassDeclStmt *)stmt;
            sym = _add_trait(ps, ps->stbl, kls);
            if (!sym) goto failed;
            break;
        }
        default: {
            break;
        }
    }

    vector_push_back(&ps->stmts, &stmt);
    return;

failed:
    stmt_free(stmt);
}

void yyparse_module(ParserState *ps, Vector *imports, Vector *stmts)
{
    Stmt **stmt;
    vector_foreach(stmt, stmts) {
        parse_top_stmt(ps, *stmt);
    }
    vector_destroy(stmts);
}

#ifdef __cplusplus
}
#endif
