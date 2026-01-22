/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "parser.h"

#ifdef __cplusplus
extern "C" {
#endif

static void codegen_visit_expr(ParserState *ps, Expr *exp);

static void codegen_ident(ParserState *ps, Expr *exp)
{
    IdentExpr *ident = (IdentExpr *)exp;
    Symbol *sym = ident->sym;
    if (!sym) {
        kl_error(ident->id.loc, "undefined symbol '%s'", ident->id.name);
        return;
    }

    ParserScope *sc = ps->scope;

    KlrBuilder bldr;
    klr_builder_end(&bldr, sc->bb);

    switch (sym->kind) {
        case SYM_VAR: {
            VarSymbol *var_sym = (VarSymbol *)sym;
            exp->ir_val = klr_build_load(&bldr, var_sym->ir_val, ident->id.name);
            break;
        }
        case SYM_FUNC: {
            FuncSymbol *func_sym = (FuncSymbol *)sym;
            exp->ir_val = func_sym->ir_val;
            break;
        }
        default: {
            UNREACHABLE();
            break;
        }
    }
}

static void codegen_literal(ParserState *ps, Expr *exp)
{
    LitExpr *lit = (LitExpr *)exp;
    switch (lit->which) {
        case LIT_EXPR_INT: {
            exp->ir_val = klr_const_int(lit->ival, lit->ts);
            break;
        }
        case LIT_EXPR_FLT: {
            exp->ir_val = klr_const_float(lit->fval, lit->ts);
            break;
        }
        case LIT_EXPR_BOOL: {
            exp->ir_val = klr_const_bool(lit->bval);
            break;
        }
        case LIT_EXPR_STR: {
            exp->ir_val = klr_const_str(lit->sval, lit->len);
            break;
        }
        case LIT_EXPR_NONE: {
            UNREACHABLE();
            break;
        }
        default: {
            UNREACHABLE();
            break;
        }
    }
}

static void codegen_type(ParserState *ps, Expr *exp) {}

static void codegen_call(ParserState *ps, Expr *exp)
{
    CallExpr *call = (CallExpr *)exp;
    // Expr *func = call->func;
    // Vector *args = &call->args;
    // size_t size = vector_size(args);

    // // generate code for function expression
    // func->ctx = EXPR_CTX_LOAD;
    // codegen_visit_expr(ps, func);
    // if (!func->ts) return;

    // // generate code for arguments
    // Expr **arg_exp;
    // vector_foreach(arg_exp, args) {
    //     (*arg_exp)->ctx = EXPR_CTX_LOAD;
    //     codegen_visit_expr(ps, *arg_exp);
    //     if (!(*arg_exp)->ts) return;
    // }

    // codegen
}

static OpCode get_binary_op_code(BiOpKind op)
{
    switch (op) {
        case BINARY_ADD:
            return OP_BINARY_ADD;
        case BINARY_SUB:
            return OP_BINARY_SUB;
        case BINARY_MUL:
            return OP_BINARY_MUL;
        case BINARY_DIV:
            return OP_BINARY_DIV;
        case BINARY_MOD:
            return OP_BINARY_MOD;
        default:
            UNREACHABLE();
            return OP_NOP;
    }
}

static char *get_binary_op_name(BiOpKind op)
{
    switch (op) {
        case BINARY_ADD:
            return "add";
        case BINARY_SUB:
            return "sub";
        case BINARY_MUL:
            return "mul";
        case BINARY_DIV:
            return "div";
        case BINARY_MOD:
            return "mod";
        default:
            UNREACHABLE();
            return "";
    }
}

static void codegen_binary(ParserState *ps, Expr *exp)
{
    BinaryExpr *bin = (BinaryExpr *)exp;
    BiOpKind op = bin->op;
    Expr *lhs = bin->lhs;
    Expr *rhs = bin->rhs;

    lhs->ctx = EXPR_CTX_LOAD;
    codegen_visit_expr(ps, lhs);
    if (!lhs->ir_val) return;

    rhs->ctx = EXPR_CTX_LOAD;
    codegen_visit_expr(ps, rhs);
    if (!rhs->ir_val) return;

    // codegen
    ParserScope *sc = ps->scope;

    KlrBuilder bldr;
    klr_builder_end(&bldr, sc->bb);

    KlrValue *res = klr_build_binary(&bldr, lhs->ir_val, rhs->ir_val,
                                     get_binary_op_code(op), "", get_binary_op_name(op));
    exp->ir_val = res;
}

static void codegen_visit_expr(ParserState *ps, Expr *exp)
{
    if (!exp) return;

    /* if errors is greater than MAX_ERRORS, stop parsing */
    if (ps->errors >= MAX_ERRORS) return;

    /* clang-format off */
    static void (*handlers[])(ParserState *, Expr *) = {
        NULL,                            /* UNKNOWN    */
        codegen_ident,                     /* ID         */
        NULL, // codegen_under,                     /* UNDER      */
        codegen_literal,                   /* LITERAL    */
        NULL,// codegen_self,                      /* SELF       */
        NULL,// codegen_super,                     /* SUPER      */
        NULL,// codegen_array_expr,                /* ARRAY      */
        NULL,// codegen_map_expr,                  /* MAP        */
        NULL,                            /* MAP_ENTRY  */
        NULL,// codegen_tuple_expr,                /* TUPLE      */
        NULL,// codegen_anony,                     /* ANONY      */
        NULL,
        codegen_type,                      /* TYPE       */
        codegen_call,                      /* CALL       */
        NULL,
        NULL, // codegen_attr,                      /* ATTR       */
        NULL, // codegen_tuple_get,                 /* TUPLE_GET  */
        NULL, // codegen_index,                     /* INDEX      */
        NULL, // codegen_unary,                     /* UNARY      */
        codegen_binary,                    /* BINARY     */
        // codegen_range,                     /* RANGE      */
        // codegen_is_expr,                   /* IS         */
        // codegen_as_expr,                   /* AS         */
        NULL,                 /* OPT        */
        NULL,                   /* OPT_NOT    */
    };
    /* clang-format on */

    handlers[exp->kind](ps, exp);
}

static void codegen_var_decl(ParserState *ps, Stmt *stmt)
{
    VarDeclStmt *var = (VarDeclStmt *)stmt;
    Expr *exp = var->exp;
    if (!exp) return;

    exp->ctx = EXPR_CTX_LOAD;
    codegen_visit_expr(ps, exp);
    if (!exp->ir_val) return;

    ParserScope *sc = ps->scope;

    KlrBuilder bldr;
    klr_builder_end(&bldr, sc->bb);

    VarSymbol *sym = (VarSymbol *)var->sym;
    klr_build_store(&bldr, sym->ir_val, exp->ir_val);
}

static void codegen_func_decl(ParserState *ps, Stmt *stmt) {}

static void codegen_class(ParserState *ps, Stmt *stmt) {}

static void codegen_trait(ParserState *ps, Stmt *stmt) {}

static void codegen_return(ParserState *ps, Stmt *stmt) {}

static void codegen_expr(ParserState *ps, Stmt *stmt)
{
    ExprStmt *s = (ExprStmt *)stmt;
    Expr *exp = s->exp;
    exp->ctx = EXPR_CTX_LOAD;
    codegen_visit_expr(ps, exp);
}

static void codegen_stmt(ParserState *ps, Stmt *stmt)
{
    if (!stmt) return;

    /* if errors is greater than MAX_ERRORS, stop parsing */
    if (ps->errors >= MAX_ERRORS) return;

    /* clang-format off */
    static void (*handlers[])(ParserState *, Stmt *) = {
        NULL,                               /* INVALID          */
        NULL, // parse_import,              /* IMPORT_KIND      */
        codegen_var_decl,                   /* VAR_KIND         */
        codegen_func_decl,                  /* FUNC_KIND        */
        codegen_class,                      /* CLASS_KIND       */
        codegen_trait,                      /* TRAIT_KIND       */
        codegen_return,                     /* RETURN_KIND      */
        NULL, // parse_assign,              /* ASSIGN_KIND      */
        NULL, // parse_break,               /* BREAK_KIND       */
        NULL, // parse_continue,            /* CONTINUE_KIND    */
        codegen_expr,                       /* EXPR_KIND        */
        NULL, // parse_block,               /* BLOCK_KIND       */
        NULL, // parse_if,                  /* IF_KIND          */
        NULL, // parse_while,               /* WHILE_KIND       */
        NULL, // parse_for,                 /* FOR_KIND         */
        NULL, // parse_match,               /* MATCH_KIND       */
    };
    /* clang-format on */

    handlers[stmt->kind](ps, stmt);
}

void codegen_ast(ParserState *ps)
{
    KlrModule *m = klr_create_module(ps->filename);
    if (!m) return;
    ps->module = m;

    // visit all global variables and add them to ir module
    Stmt *s;
    vector_foreach(s, &ps->stmts) {
        if (!s) continue;
        if (s->kind == STMT_VAR_KIND) {
            VarDeclStmt *var = (VarDeclStmt *)s;
            Ident *id = &var->id;
            VarSymbol *sym = (VarSymbol *)var->sym;
            KlrValue *gvar = klr_add_global(m, sym->ts, id->name);
            sym->ir_val = gvar;
        }
    }

    KlrValue *fn = klr_add_func(m, NULL, NULL, "__init__");
    m->init = (KlrFunc *)fn;

    ParserScope *scope = enter_scope(ps, SCOPE_TOP, 0);
    scope->stbl = ps->stbl;
    scope->sym = NULL;
    KlrBasicBlock *entry = klr_append_block(fn, "entry");
    scope->bb = entry;

    // codegen all statements
    vector_foreach(s, &ps->stmts) {
        if (!s) continue;
        codegen_stmt(ps, s);
    }

    exit_scope(ps);

    klr_print_module(m, stdout);
}

#ifdef __cplusplus
}
#endif
