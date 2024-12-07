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
        log_info("find symbol '%s' in builtin module\n", id->name);
        id->where = BLTIN_SCOPE;
        id->scope = NULL;
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
        if (!_add_var(ps, ps->scope->stbl, var)) return;
    }

    VarSymbol *sym = (VarSymbol *)id->sym;

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
    TypeDesc *ty = fn->ret ? fn->ret->desc : NULL;
    Symbol *sym;

    int flags = parse_flags(&fn->flags);

    sym = stbl_add_func(stbl, id->name, fn->tps, ty, fn->args, flags);

    if (!sym) {
        kl_error(id->loc, "redefinition of '%s'", id->name);
        return NULL;
    }

    fn->id.sym = sym;
    return sym;
}

static void parse_stmt(ParserState *ps, Stmt *stmt);

static void parse_body(ParserState *ps, Vector *stmts)
{
    int sz = vector_size(stmts);
    int index = 0;
    Stmt **s;
    vector_foreach(s, stmts) {
        parse_stmt(ps, *s);
        if (ps->errors >= MAX_ERRORS) break;
        ++index;

        if ((*s)->kind == STMT_RETURN_KIND) {
            if (index < sz) {
                kl_error((*s)->loc, "statements after this are unreachable.");
                return;
            }
        }

        if (index == sz && (*s)->kind == STMT_EXPR_KIND) {
            /*
             * If last statement is expression in func body,
             * the expr value can be func return value.
             */
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

    ParserScope *scope = enter_scope(ps, SCOPE_FUNC, 0);

    /* add parameters into function symbol table */
    ParamDecl *param;
    vector_foreach(param, fn->args) {
    }

    /* parse body */
    parse_body(ps, fn->body);

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
        parse_stmt(ps, *s);
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
        parse_stmt(ps, *s);
    }

    exit_scope(ps);
}

static void parse_stmt(ParserState *ps, Stmt *stmt)
{
    if (!stmt) return;

    /* if errors is greater than MAX_ERRORS, stop parsing */
    if (ps->errors >= MAX_ERRORS) return;

    /* clang-format off */
    static void (*handlers[])(ParserState *, Stmt *) = {
        NULL,                       /* INVALID          */
        NULL, // parse_import,               /* IMPORT_KIND      */
        parse_var_decl,             /* VAR_KIND         */
        parse_func_decl,            /* FUNC_KIND        */
        parse_class,                /* CLASS_KIND       */
        parse_trait,                /* TRAIT_KIND       */
        NULL, // parse_return,               /* RETURN_KIND      */
        NULL, // parse_assign,               /* ASSIGN_KIND      */
        NULL, // parse_break,                /* BREAK_KIND       */
        NULL, // parse_continue,             /* CONTINUE_KIND    */
        parse_expr,                 /* EXPR_KIND        */
        NULL, // parse_block,                /* BLOCK_KIND       */
        NULL, // parse_if,                   /* IF_KIND          */
        NULL, // parse_while,                /* WHILE_KIND       */
        NULL, // parse_for,                  /* FOR_KIND         */
        NULL, // parse_match,                /* MATCH_KIND       */
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

static void read_builtin_module(ParserState *ps)
{
    KlcFile klc;
    init_klc_file(&klc, "libs/builtin.klc");
    read_klc_file(&klc, 0);
    read_from_klc(&klc);
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
    if (!strstr(path, "builtin.kl")) {
        read_builtin_module(ps);
    }

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
        kl_code_gen(ps);
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
