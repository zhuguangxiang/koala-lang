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
#define print_desc(desc) do {           \
    BUF(buf);                           \
    desc_print(desc, &buf);            \
    log_info("  '%s'", BUF_STR(buf));   \
    FINI_BUF(buf);                      \
} while (0)
/* clang-format on */
#else
#define print_desc(desc) ((void *)(desc))
#endif

#ifndef NOLOG
static const char *scopes[] = {
    "TOP", "TYPE", "FUNC", "BLOCK", "ANONY",
};

static const char *blocks[] = {
    "UNK",       "BLOCK",       "IF-BLOCK",   "WHILE-BLOCK",
    "FOR-BLOCK", "MATCH-BLOCK", "MATCH-CASE", "MATCH-CLAUSE",
};
#endif

static ParserScope *enter_scope(ParserState *ps, ScopeKind kind, BlockType block)
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

static void exit_scope(ParserState *ps)
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

static void update_ir_info(ParserState *ps, Symbol *sym)
{
    KlrValue *val = NULL;
    if (sym->kind == SYM_FUNC) {
        val = klr_add_ext_func(ps->module, DESC_INCREF_GET(sym->desc), sym->name);
    } else {
        NYI();
    }
    sym->ir_val = val;
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
            log_info("find symbol '%s' in up scope-%d(%s)\n", id->name, depth,
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
        update_ir_info(ps, sym);
        return sym;
    }

    return NULL;
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
    TypeDesc *ty = var->type ? var->type->desc : NULL;
    Symbol *sym;

    int flags = parse_flags(&var->flags);
    if (!var->ro) flags |= SYM_FLAGS_MUTABLE;
    if (var->exp && var->exp->kind == EXPR_LITERAL_KIND) flags |= SYM_FLAGS_VAR_VALUE;

    sym = stbl_add_var(stbl, id->name, ty, flags);

    if (!sym) {
        kl_error(id->loc, "redefinition of '%s'", id->name);
        return NULL;
    }

    var->id.sym = sym;
    return sym;
}

static void parse_var_decl(ParserState *ps, Stmt *stmt)
{
    VarDeclStmt *var = (VarDeclStmt *)stmt;
    Ident *id = &var->id;
    TypeDesc *desc = var->type ? var->type->desc : NULL;
    Expr *exp = var->exp;

    exp->ctx = EXPR_CTX_LOAD;
    exp->expected = desc;
    parser_visit_expr(ps, exp);
    if (!exp->desc) return;

    /*
     * If var is global, it is already existed.
     * If var is local, it needs to be added into symbol table.
     */
    if (var->where != VAR_GLOBAL) {
        ParserScope *sc = ps->scope;
        if (!_add_var(ps, sc->stbl, var)) return;
    }

    VarSymbol *sym = (VarSymbol *)id->sym;

    if (var->where == VAR_GLOBAL) {
        if (exp && exp->kind == EXPR_LITERAL_KIND) {
            LitExpr *lit_exp = (LitExpr *)exp;
            sym->scope = VAR_SCOPE_GLOBAL;
            Literal *lit = mm_alloc_obj_fast(lit);
            if (lit_exp->which == LIT_EXPR_INT) {
                lit->which = LIT_INT;
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

    if (!desc) {
        /* update symbol type */
        sym->desc = exp->desc;
        log_info("update symbol '%s' type as:", sym->name);
        print_desc(sym->desc);
    } else {
        if (!desc_equal(desc, exp->desc)) {
            kl_error(id->loc, "Types of two sides are not matched.");
            log_info("lhs:");
            print_desc(desc);
            log_info("rhs:");
            print_desc(exp->desc);
        }
    }

    // codegen
    ParserScope *sc = ps->scope;
    if (sc->kind == SCOPE_FUNC) {
        KlrBuilder bldr;
        klr_builder_end(&bldr, sc->bb);
        KlrValue *ir_var = klr_add_local(&bldr, sym->desc, id->name);
        sym->ir_val = ir_var;
        klr_build_store(&bldr, ir_var, exp->ir_val);
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
    TypeDesc *ty = fn->ret ? fn->ret->desc : desc_no_type();
    Symbol *sym;

    int flags = parse_flags(&fn->flags);
    char *ann = fn->flags.at.ident;
    char *ann_key = fn->flags.at.assoc_ident;
    sym = stbl_add_func(stbl, id->name, fn->tps, ty, NULL, flags, ann, ann_key);

    if (!sym) {
        kl_error(id->loc, "redefinition of '%s'", id->name);
        return NULL;
    }

    fn->id.sym = sym;
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
                    KlrBuilder bldr;
                    ParserScope *sc = ps->scope;
                    klr_builder_end(&bldr, sc->bb);
                    if (desc_is_no_type(sym->desc)) {
                        klr_build_ret_void(&bldr);
                    } else {
                        // check types
                        // code gen
                        klr_build_ret(&bldr, exp->exp->ir_val);
                    }
                } else {
                    UNREACHABLE();
                }
            } else {
                // last statement is return statement
                RetStmt *ret = (RetStmt *)s;
                if (desc_is_no_type(sym->desc)) {
                    if (ret->exp) {
                        kl_error(s->loc, "func '%s' has not return value.", sym->name);
                        return;
                    }
                } else {
                    // check types
                }
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

    FuncSymbol *sym = (FuncSymbol *)fn->id.sym;

    sc = enter_scope(ps, SCOPE_FUNC, 0);
    sc->stbl = sym->stbl;
    sc->sym = (Symbol *)sym;

    /* add parameters into function symbol table */

    Vector *args = vector_create_ptr();

    int size = vector_size(fn->args);
    TypeDesc *params[(size + 1)];
    Symbol *arg_syms[size];

    ParamDecl **param_p;
    ParamDecl *param;
    vector_foreach(param_p, fn->args) {
        param = *param_p;

        ArgInfo *arg = mm_alloc_obj_fast(arg);
        arg->name = param->id.name;
        arg->dfl_val_idx = 0;

        TypeDesc *desc;
        if (param->type) {
            desc = param->type->desc;
            DESC_INCREF(desc);
        } else {
            Expr *e = param->value;
            e->ctx = EXPR_CTX_LOAD;
            parser_visit_expr(ps, e);
            if (!e->desc) return;
            desc = e->desc;
            DESC_INCREF(desc);
        }
        Symbol *s = stbl_add_var(sc->stbl, param->id.name, desc, 0);

        arg->desc = desc;
        DESC_INCREF(desc);
        vector_push_back(args, &arg);

        params[i__] = DESC_INCREF_GET(desc);
        arg_syms[i__] = s;
    }

    params[size] = 0;

    sym->params = args;

    KlrValue *fval = klr_add_func(ps->module, sym->desc, params, fn->id.name);
    sym->ir_val = fval;
    KlrBasicBlock *entry = klr_append_block(fval, "entry");
    sc->bb = entry;

    for (int i = 0; i < size; i++) {
        Symbol *s = arg_syms[i];
        s->ir_val = klr_get_param(fval, i);
    }

    /* parse body */
    parse_body(ps, sym, fn->body);

    klr_print_func((KlrFunc *)fval, stdout);
    klr_alloc_registers((KlrFunc *)fval);
    klr_print_func((KlrFunc *)fval, stdout);

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
    // TypeDesc *ty = fn->ret ? fn->ret->desc : NULL;
    Symbol *sym;

    int flags = parse_flags(&kls->flags);

    sym = stbl_add_klass(stbl, id->name, NULL, NULL, flags);

    if (!sym) {
        kl_error(id->loc, "redefinition of '%s'", id->name);
        return NULL;
    }

    // fn->id.sym = sym;
    return sym;
}

static Symbol *_add_trait(ParserState *ps, HashMap *stbl, KlassDeclStmt *kls)
{
    Ident *id = &kls->id;
    // TypeDesc *ty = fn->ret ? fn->ret->desc : NULL;
    Symbol *sym;

    int flags = parse_flags(&kls->flags);
    sym = stbl_add_trait(stbl, id->name, NULL, NULL, flags);

    if (!sym) {
        kl_error(id->loc, "redefinition of '%s'", id->name);
        return NULL;
    }

    // fn->id.sym = sym;
    return sym;
}

static void parse_class(ParserState *ps, Stmt *stmt)
{
    KlassDeclStmt *kls = (KlassDeclStmt *)stmt;

    ParserScope *scope = enter_scope(ps, SCOPE_TYPE, 0);

    /* parse class body */
    Stmt **s;
    vector_foreach(s, kls->stmts) {
        // parse_stmt(ps, *s);
    }

    exit_scope(ps);
}

static void parse_trait(ParserState *ps, Stmt *stmt)
{
    KlassDeclStmt *kls = (KlassDeclStmt *)stmt;

    ParserScope *scope = enter_scope(ps, SCOPE_TYPE, 0);

    /* parse class body */
    Stmt **s;
    vector_foreach(s, kls->stmts) {
        // parse_stmt(ps, *s);
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
        if (!exp->desc) return;
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

static ParserState *build_ast(char *path)
{
    FILE *in = fopen(path, "r");
    if (in == NULL) {
        kl_printf_error("%s: No such file or directory\n", path);
        return NULL;
    }

    ParserState *ps = mm_alloc_obj(ps);
    init_parser_state(ps, path);
    kl_read_from_klc(ps->builtin, "libs/builtin.klc");

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

    KlrModule *m = klr_create_module(ps->filename);
    if (!m) return -1;
    ps->module = m;

    KlrValue *fn = klr_add_func(m, NULL, NULL, "__init__");
    m->init = (KlrFunc *)fn;

    parse_ast(ps);

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
