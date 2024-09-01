/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "parser.h"
#include "atom.h"
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
    desc_to_str(desc, &buf);            \
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

static Symbol *_add_var(ParserState *ps, HashMap *stbl, VarDeclStmt *var)
{
    Ident *id = &var->id;
    TypeDesc *ty = var->type ? var->type->desc : NULL;
    Symbol *sym;

    sym = stbl_add_var(stbl, id->name, ty);

    if (!sym) {
        kl_error(var->id.loc, "redefinition of '%s'", id->name);
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
    parse_expr(ps, exp);
    if (!exp->desc) return;

    /*
     * If var is global, it is already existed.
     * If var is local, it needs to be added into symbol table.
     */
    if (var->where != VAR_GLOBAL) {
        if (!_add_var(ps, ps->scope->stbl, var)) return;
    }

    if (!desc) {
        /* update symbol type */
        Symbol *sym = id->sym;
        sym->desc = exp->desc;
        log_info("update '%s' type to:", sym->name);
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
        // parse_tuple_var_decl,       /* TUPLE_VAR_KIND   */
        // parse_assign,               /* ASSIGN_KIND      */
        // parse_func_decl,            /* FUNC_KIND        */
        // parse_return,               /* RETURN_KIND      */
        // parse_expr,                 /* EXPR_KIND        */
        // parse_block,                /* BLOCK_KIND       */
        // parse_class,                /* CLASS_KIND       */
        // parse_trait,                /* TRAIT_KIND       */
        // parse_enum,                 /* ENUM_KIND        */
        // parse_break,                /* BREAK_KIND       */
        // parse_continue,             /* CONTINUE_KIND    */
        // parse_if,                   /* IF_KIND          */
        // parse_while,                /* WHILE_KIND       */
        // parse_for,                  /* FOR_KIND         */
        // parse_match,                /* MATCH_KIND       */
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
    stbl_show(scope->stbl);
    exit_scope(ps);

    /* If there are errors, stop doing codegen. */
    if (ps->errors) return;

#ifndef NOLOG
        /* dump AST */
        // dump_stmt(stmt);
#endif

    /* FIXME: codegen */
    // kl_codegen_stmt(ps, stmt);
}

static void init_parser_state(ParserState *ps, char *filename)
{
    ps->filename = filename;
    vector_init_ptr(&ps->stmts);
    ps->stbl = stbl_new();
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
    parse_ast(ps);
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
            var->where = VAR_GLOBAL;
            break;
        }
        default: {
            UNREACHABLE();
            break;
        }
    }

    if (sym) {
        vector_push_back(&ps->stmts, &stmt);
    } else {
        stmt_free(stmt);
    }
}

void yyparse_module(ParserState *ps, Vector *imports, Vector *stmts)
{
    Stmt **stmt;
    vector_foreach(stmt, stmts) {
        stmt_free(*stmt);
        // parse_top_stmt(ps, *stmt);
    }
    vector_destroy(stmts);
}

#ifdef __cplusplus
}
#endif
