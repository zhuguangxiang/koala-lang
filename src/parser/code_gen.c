/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "atom.h"
#include "ir.h"
#include "log.h"
#include "mm.h"
#include "parser.h"

/* codegen:
emit to IR
do optimization
remap to VM byte codes
*/

#ifdef __cplusplus
extern "C" {
#endif

static void cgen_var_decl(ParserState *ps, Stmt *stmt)
{
    VarDeclStmt *var = (VarDeclStmt *)stmt;
    Expr *exp = var->exp;

    if (exp->kind != EXPR_LITERAL_KIND) {
        // generate code in __init__()
        printf("generate code in __init__()\n");
    } else {
        printf("no need generate code for var decl\n");
    }
}

static void cgen_func_decl(ParserState *ps, Stmt *stmt)
{
    FuncDeclStmt *fn = (FuncDeclStmt *)stmt;
    if (!fn->body || vector_empty(fn->body)) {
        log_info("func '%s' no body", fn->id.name);
        return;
    }
}

static void cgen_class(ParserState *ps, Stmt *stmt) {}

static void cgen_trait(ParserState *ps, Stmt *stmt) {}

static void cgen_expr(ParserState *ps, Stmt *stmt) {}

void kl_code_gen(ParserState *ps)
{
    /* clang-format off */
    static void (*handlers[])(ParserState *, Stmt *) = {
        NULL,                               /* INVALID          */
        NULL,                               /* IMPORT_KIND      */
        cgen_var_decl,                      /* VAR_KIND         */
        cgen_func_decl,                     /* FUNC_KIND        */
        cgen_class,                         /* CLASS_KIND       */
        cgen_trait,                         /* TRAIT_KIND       */
        NULL, // parse_return,              /* RETURN_KIND      */
        NULL, // parse_assign,              /* ASSIGN_KIND      */
        NULL, // parse_break,               /* BREAK_KIND       */
        NULL, // parse_continue,            /* CONTINUE_KIND    */
        cgen_expr,                          /* EXPR_KIND        */
        NULL, // parse_block,               /* BLOCK_KIND       */
        NULL, // parse_if,                  /* IF_KIND          */
        NULL, // parse_while,               /* WHILE_KIND       */
        NULL, // parse_for,                 /* FOR_KIND         */
        NULL, // parse_match,               /* MATCH_KIND       */
    };
    /* clang-format on */

    KlrModule *m = klr_create_module(ps->filename);
    if (!m) return;
    ps->module = m;

    KlrValue *fn = klr_add_func(m, NULL, NULL, "__init__");
    m->init = (KlrFunc *)fn;

    Stmt **stmt;
    vector_foreach(stmt, &ps->stmts) {
        handlers[(*stmt)->kind](ps, *stmt);
    }

    klr_dump_module(m);
}

#ifdef __cplusplus
}
#endif
