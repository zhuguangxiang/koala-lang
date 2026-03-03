/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "parser.h"

#ifdef __cplusplus
extern "C" {
#endif

#define KLR_BUILDER(ps) \
    ParserScope *sc = ps->scope; \
    KlrBuilder bldr; \
    klr_builder_end(&bldr, sc->bb);

static void emit_ir_visit_expr(ParserState *ps, Expr *exp);

static void emit_ir_ident(ParserState *ps, Expr *exp)
{
    IdentExpr *ident = (IdentExpr *)exp;
    Symbol *sym = ident->sym;
    if (!sym) {
        kl_error(ident->id.loc, "undefined symbol '%s'", ident->id.name);
        return;
    }

    KLR_BUILDER(ps);

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

static void emit_ir_literal(ParserState *ps, Expr *exp)
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

static void emit_ir_type(ParserState *ps, Expr *exp) {}

static void emit_ir_call(ParserState *ps, Expr *exp)
{
    CallExpr *call = (CallExpr *)exp;
    Expr *lhs = call->lhs;
    Vector *args = call->args;
    int size = vector_size(args);

    // gen ir for lhs
    lhs->ctx = EXPR_CTX_LOAD;
    emit_ir_visit_expr(ps, lhs);
    if (!lhs->ir_val) return;

    KlrValue *ir_args[size];

    // gen ir for arguments
    Expr *e;
    vector_foreach(e, args) {
        e->ctx = EXPR_CTX_LOAD;
        emit_ir_visit_expr(ps, e);
        if (!e->ir_val) return;
        ir_args[i__] = e->ir_val;
    }

    // codegen

    KLR_BUILDER(ps);

    KlrValue *ret = klr_build_call(&bldr, lhs->ir_val, ir_args, size, "");
    exp->ir_val = ret;
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

static void emit_ir_binary(ParserState *ps, Expr *exp)
{
    BinaryExpr *bin = (BinaryExpr *)exp;
    BiOpKind op = bin->op;
    Expr *lhs = bin->lhs;
    Expr *rhs = bin->rhs;

    lhs->ctx = EXPR_CTX_LOAD;
    emit_ir_visit_expr(ps, lhs);
    if (!lhs->ir_val) return;

    rhs->ctx = EXPR_CTX_LOAD;
    emit_ir_visit_expr(ps, rhs);
    if (!rhs->ir_val) return;

    KLR_BUILDER(ps);

    KlrValue *res = klr_build_binary(&bldr, lhs->ir_val, rhs->ir_val,
                                     get_binary_op_code(op), "", get_binary_op_name(op));
    exp->ir_val = res;
}

static void emit_ir_visit_expr(ParserState *ps, Expr *exp)
{
    if (!exp) return;

    /* if errors is greater than MAX_ERRORS, stop parsing */
    if (ps->errors >= MAX_ERRORS) return;

    /* clang-format off */
    static void (*handlers[])(ParserState *, Expr *) = {
        [EXPR_ID_KIND]      = emit_ir_ident,
        [EXPR_LITERAL_KIND] = emit_ir_literal,
        [EXPR_TYPE_KIND]    = emit_ir_type,
        [EXPR_CALL_KIND]    = emit_ir_call,
        [EXPR_BINARY_KIND]  = emit_ir_binary,
    };
    /* clang-format on */

    handlers[exp->kind](ps, exp);
}

static void emit_ir_var_decl(ParserState *ps, Stmt *stmt)
{
    VarDeclStmt *var = (VarDeclStmt *)stmt;
    Expr *exp = var->exp;
    if (!exp) return;

    exp->ctx = EXPR_CTX_LOAD;
    emit_ir_visit_expr(ps, exp);
    if (!exp->ir_val) return;

    KLR_BUILDER(ps);

    VarSymbol *sym = (VarSymbol *)var->sym;
    klr_build_store(&bldr, sym->ir_val, exp->ir_val);
}

static void emit_ir_func_decl(ParserState *ps, Stmt *stmt) {}

static void emit_ir_class(ParserState *ps, Stmt *stmt) {}

static void emit_ir_trait(ParserState *ps, Stmt *stmt) {}

static void emit_ir_return(ParserState *ps, Stmt *stmt) {}

static void emit_ir_expr(ParserState *ps, Stmt *stmt)
{
    ExprStmt *s = (ExprStmt *)stmt;
    Expr *exp = s->exp;
    exp->ctx = EXPR_CTX_LOAD;
    emit_ir_visit_expr(ps, exp);
}

static void emit_ir_stmt(ParserState *ps, Stmt *stmt)
{
    if (!stmt) return;

    /* if errors is greater than MAX_ERRORS, stop parsing */
    if (ps->errors >= MAX_ERRORS) return;

    /* clang-format off */
    static void (*handlers[STMT_MAX_KIND])(ParserState *, Stmt *) = {
        [STMT_VAR_KIND]     = emit_ir_var_decl,
        [STMT_FUNC_KIND]    = emit_ir_func_decl,
        [STMT_CLASS_KIND]   = emit_ir_class,
        [STMT_TRAIT_KIND]   = emit_ir_trait,
        [STMT_RETURN_KIND]  = emit_ir_return,
        [STMT_EXPR_KIND]    = emit_ir_expr,
    };
    /* clang-format on */

    handlers[stmt->kind](ps, stmt);
}

void ast_emit_ir(ParserState *ps)
{
    KlrModule *m = ps->module;

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

    ParserScope *scope = enter_scope(ps, SCOPE_TOP, 0, "top");
    scope->stbl = ps->stbl;
    scope->sym = NULL;
    KlrBasicBlock *entry = klr_append_block(fn, "entry");
    scope->bb = entry;

    // emit ir for all statements
    vector_foreach(s, &ps->stmts) {
        if (!s) continue;
        emit_ir_stmt(ps, s);
    }

    exit_scope(ps);

    klr_print_module(m, stdout);
}

#ifdef __cplusplus
}
#endif
