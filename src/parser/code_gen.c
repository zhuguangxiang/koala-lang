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

#define SYMBOL_FLAGS_HAS_VALUE (1 << 0)
#define SYMBOL_FLAGS_MUTABLE   (1 << 1)
#define SYMBOL_FLAGS_PUBLIC    (1 << 2)
#define SYMBOL_FLAGS_TAG_ONLY  (1 << 3)
#define SYMBOL_FLAGS_TAG_VALUE (1 << 4)

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

    int flags = 0;
    if (has_value) flags |= SYMBOL_FLAGS_HAS_VALUE;
    if (!var->ro) flags |= SYMBOL_FLAGS_MUTABLE;
    if (var->pub) flags |= SYMBOL_FLAGS_PUBLIC;

    klc_add_var(klc, sym->name, BUF_STR(buf), has_value, flags);

    if (exp->kind == EXPR_LITERAL_KIND) {
        LitExpr *lit = (LitExpr *)exp;
        if (lit->which == LIT_EXPR_INT) {
            klc_add_int(klc, lit->ival, lit->len);
        } else if (lit->which == LIT_EXPR_BOOL) {
            klc_add_int(klc, lit->bval, 1);
        } else if (lit->which == LIT_EXPR_FLT) {
            klc_add_float(klc, lit->fval, lit->len);
        } else if (lit->which == LIT_EXPR_STR) {
            klc_add_str(klc, lit->sval, lit->len);
        } else if (lit->which == LIT_EXPR_NONE) {
            klc_add_none(klc);
        } else {
            NYI();
        }
    }

    FINI_BUF(buf);
}

static void codegen_func_decl(ParserState *ps, Stmt *stmt, KlcFile *klc)
{
    FuncDeclStmt *fn = (FuncDeclStmt *)stmt;
    Ident *id = &fn->id;
    FuncSymbol *sym = (FuncSymbol *)id->sym;

    BUF(buf);
    desc_to_str(sym->ret, &buf);

    int flags = 0;
    PrefixFlags *prefix = &fn->flags;
    if (prefix->pub.flag) flags |= SYMBOL_FLAGS_PUBLIC;
    if (prefix->at.flag.flag) {
        if (prefix->at.assoc_ident) {
            flags |= SYMBOL_FLAGS_TAG_VALUE;
        } else {
            flags |= SYMBOL_FLAGS_TAG_ONLY;
        }
    }

    klc_add_func(klc, id->name, BUF_STR(buf), flags);

    FINI_BUF(buf);

    if (prefix->at.flag.flag) {
        char *s = prefix->at.ident;
        int len = strlen(s);
        klc_add_str(klc, s, len);
        if (prefix->at.assoc_ident) {
            s = prefix->at.assoc_ident;
            len = strlen(s);
            klc_add_str(klc, s, len);
        }
    }
}

static void codegen_class(ParserState *ps, Stmt *stmt, KlcFile *klc) {}

static void codegen_trait(ParserState *ps, Stmt *stmt, KlcFile *klc) {}

/* clang-format off */
static void (*handlers[])(ParserState *, Stmt *, KlcFile *klc) = {
    NULL,                       /* INVALID          */
    NULL, // parse_import,               /* IMPORT_KIND      */
    codegen_var_decl,             /* VAR_KIND         */
    NULL, // parse_assign,               /* ASSIGN_KIND      */
    codegen_func_decl,            /* FUNC_KIND        */
    NULL, // parse_return,               /* RETURN_KIND      */
    NULL,                    /* EXPR_KIND        */
    NULL, // parse_block,                /* BLOCK_KIND       */
    codegen_class,                /* CLASS_KIND       */
    codegen_trait,                /* TRAIT_KIND       */
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
    char *filename = atom_concat(2, ps->filename, "c");
    init_klc_file(&klc, filename);

    Stmt **stmt;
    vector_foreach(stmt, &ps->stmts) {
        handlers[(*stmt)->kind](ps, *stmt, &klc);
    }

    write_klc_file(&klc);

    klc_dump(&klc);

    KlcFile klc2 = { 0 };
    init_klc_file(&klc2, filename);
    read_klc_file(&klc2, 1);
    klc_dump(&klc2);
}

#ifdef __cplusplus
}
#endif
