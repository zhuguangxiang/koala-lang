/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "atom.h"
#include "ir.h"
#include "klc.h"
#include "mm.h"
#include "parser.h"

/* codegen:
emit to IR
do optimization
remap to VM byte codes
write to klc file
*/

#ifdef __cplusplus
extern "C" {
#endif

static void codegen_var_decl(ParserState *ps, Stmt *stmt, KlcFile *klc)
{
    VarDeclStmt *var = (VarDeclStmt *)stmt;
    Ident *id = &var->id;
    Expr *exp = var->exp;
    VarSymbol *sym = (VarSymbol *)id->sym;

    int has_value = 0;

    BUF(buf);
    desc_to_str(sym->desc, &buf);

    if (exp->kind == EXPR_LITERAL_KIND) has_value = 1;

    klc_add_var(klc, sym->name, BUF_STR(buf), has_value);

    if (exp->kind == EXPR_LITERAL_KIND) {
        LitExpr *lit = (LitExpr *)exp;
        if (lit->which == LIT_EXPR_INT) {
            klc_add_int(klc, lit->ival, lit->len);
        } else if (lit->which == LIT_EXPR_BOOL) {
            klc_add_int(klc, lit->bval, 1);
        } else if (lit->which == LIT_EXPR_FLT) {
            klc_add_float(klc, lit->fval);
        } else {
            NYI();
        }
    }
    FINI_BUF(buf);
}

/* clang-format off */
static void (*handlers[])(ParserState *, Stmt *, KlcFile *klc) = {
    NULL,                       /* INVALID          */
    NULL, // parse_import,               /* IMPORT_KIND      */
    codegen_var_decl,             /* VAR_KIND         */
    NULL, // parse_tuple_var_decl,       /* TUPLE_VAR_KIND   */
    NULL, // parse_assign,               /* ASSIGN_KIND      */
    NULL, // parse_func_decl,            /* FUNC_KIND        */
    NULL, // parse_return,               /* RETURN_KIND      */
    NULL,                    /* EXPR_KIND        */
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

void kl_code_gen(ParserState *ps)
{
    KlcFile klc = { 0 };
    char *klc_filename = atom_concat(2, ps->filename, "c");
    init_klc_file(&klc, klc_filename);

    Stmt **stmt;
    vector_foreach(stmt, &ps->stmts) {
        handlers[(*stmt)->kind](ps, *stmt, &klc);
    }

    write_klc_file(&klc);

    klc_dump(&klc);
}

#ifdef __cplusplus
}
#endif
