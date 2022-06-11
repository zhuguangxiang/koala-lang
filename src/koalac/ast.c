/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2018-2022 James <zhuguangxiang@gmail.com>
 */

#include "ast.h"
#include "common/mm.h"

#ifdef __cplusplus
extern "C" {
#endif

Expr *expr_from_null(void)
{
    Expr *e = mm_alloc_obj(e);
    e->kind = EXPR_NULL_KIND;
    return e;
}

Expr *expr_from_self(void)
{
    Expr *e = mm_alloc_obj(e);
    e->kind = EXPR_SELF_KIND;
    return e;
}

Expr *expr_from_super(void)
{
    Expr *e = mm_alloc_obj(e);
    e->kind = EXPR_SUPER_KIND;
    return e;
}

Expr *expr_from_under(void)
{
    Expr *e = mm_alloc_obj(e);
    e->kind = EXPR_UNDER_KIND;
    return e;
}

Expr *expr_from_int8(int8_t val)
{
    LiteralExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_LITERAL_KIND;
    exp->i8val = val;
    exp->ty = desc_from_int8();
    return (Expr *)exp;
}

Expr *expr_from_int16(int16_t val)
{
    LiteralExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_LITERAL_KIND;
    exp->i16val = val;
    exp->ty = desc_from_int16();
    return (Expr *)exp;
}

Expr *expr_from_int32(int32_t val)
{
    LiteralExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_LITERAL_KIND;
    exp->i32val = val;
    exp->ty = desc_from_int32();
    return (Expr *)exp;
}

Expr *expr_from_int64(int64_t val)
{
    LiteralExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_LITERAL_KIND;
    exp->i64val = val;
    exp->ty = desc_from_int64();
    return (Expr *)exp;
}

Expr *expr_from_uint8(uint8_t val)
{
    LiteralExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_LITERAL_KIND;
    exp->u8val = val;
    exp->ty = desc_from_uint8();
    return (Expr *)exp;
}

Expr *expr_from_uint16(uint16_t val)
{
    LiteralExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_LITERAL_KIND;
    exp->u16val = val;
    exp->ty = desc_from_uint16();
    return (Expr *)exp;
}

Expr *expr_from_uint32(uint32_t val)
{
    LiteralExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_LITERAL_KIND;
    exp->u32val = val;
    exp->ty = desc_from_uint32();
    return (Expr *)exp;
}

Expr *expr_from_uint64(uint64_t val)
{
    LiteralExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_LITERAL_KIND;
    exp->u64val = val;
    exp->ty = desc_from_uint64();
    return (Expr *)exp;
}

Expr *expr_from_float32(float val)
{
    LiteralExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_LITERAL_KIND;
    exp->f32val = val;
    exp->ty = desc_from_float64();
    return (Expr *)exp;
}

Expr *expr_from_float64(double val)
{
    LiteralExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_LITERAL_KIND;
    exp->f64val = val;
    exp->ty = desc_from_float64();
    return (Expr *)exp;
}

Expr *expr_from_bool(int val)
{
    LiteralExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_LITERAL_KIND;
    exp->bval = val;
    exp->ty = desc_from_bool();
    return (Expr *)exp;
}

Expr *expr_from_str(char *val)
{
    LiteralExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_LITERAL_KIND;
    exp->sval = val;
    exp->ty = desc_from_str();
    return (Expr *)exp;
}

Expr *expr_from_char(int val)
{
    LiteralExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_LITERAL_KIND;
    exp->cval = val;
    exp->ty = desc_from_char();
    return (Expr *)exp;
}

Expr *expr_from_ident(Ident *val, ExprType *ty)
{
    IdExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_ID_KIND;
    exp->ty = ty->ty;
    exp->id = *val;
    return (Expr *)exp;
}

Expr *expr_from_unary(UnOpKind op, Loc op_loc, Expr *e)
{
    UnaryExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_UNARY_KIND;
    exp->uop = op;
    exp->op_loc = op_loc;
    exp->exp = e;
    return (Expr *)exp;
}

Expr *expr_from_binary(BinOpKind op, Loc op_loc, Expr *le, Expr *re)
{
    BinaryExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_BINARY_KIND;
    exp->bop = op;
    exp->op_loc = op_loc;
    exp->lexp = le;
    exp->rexp = re;
    return (Expr *)exp;
}

void free_expr(Expr *e)
{
    if (!e) return;
    switch (e->kind) {
        case EXPR_NULL_KIND:
        case EXPR_SELF_KIND:
        case EXPR_SUPER_KIND:
        case EXPR_LITERAL_KIND:
        case EXPR_ID_KIND:
        case EXPR_UNDER_KIND:
            mm_free(e);
            break;
        case EXPR_UNARY_KIND: {
            UnaryExpr *exp = (UnaryExpr *)e;
            free_expr(exp->exp);
            mm_free(e);
            break;
        }
        case EXPR_BINARY_KIND: {
            BinaryExpr *exp = (BinaryExpr *)e;
            free_expr(exp->lexp);
            free_expr(exp->rexp);
            mm_free(e);
            break;
        }
        default:
            break;
    }
}

Stmt *stmt_from_let_decl(Ident name, ExprType ty, Expr *e)
{
    VarDeclStmt *s = mm_alloc(sizeof(*s));
    s->kind = STMT_LET_KIND;
    s->id = name;
    s->ty = ty;
    s->exp = e;
    return (Stmt *)s;
}

Stmt *stmt_from_var_decl(Ident name, ExprType ty, Expr *e)
{
    VarDeclStmt *s = mm_alloc(sizeof(*s));
    s->kind = STMT_VAR_KIND;
    s->id = name;
    s->ty = ty;
    s->exp = e;
    return (Stmt *)s;
}

Stmt *stmt_from_assign(AssignOpKind op, Loc op_loc, Expr *lhs, Expr *rhs)
{
    AssignStmt *s = mm_alloc(sizeof(*s));
    s->kind = STMT_ASSIGN_KIND;
    s->lhs = lhs;
    s->rhs = rhs;
    s->op = op;
    s->op_loc = op_loc;
    return (Stmt *)s;
}

Stmt *stmt_from_ret(Expr *ret)
{
    ReturnStmt *s = mm_alloc(sizeof(*s));
    s->kind = STMT_RETURN_KIND;
    s->exp = ret;
    return (Stmt *)s;
}

Stmt *stmt_from_break(void)
{
    Stmt *s = mm_alloc(sizeof(*s));
    s->kind = STMT_BREAK_KIND;
    return s;
}

Stmt *stmt_from_continue(void)
{
    Stmt *s = mm_alloc(sizeof(*s));
    s->kind = STMT_CONTINUE_KIND;
    return s;
}

Stmt *stmt_from_block(Vector *stmts)
{
    BlockStmt *s = mm_alloc(sizeof(*s));
    s->kind = STMT_BLOCK_KIND;
    s->stmts = stmts;
    return (Stmt *)s;
}

Stmt *stmt_from_expr(Expr *e)
{
    ExprStmt *s = mm_alloc(sizeof(*s));
    s->kind = STMT_EXPR_KIND;
    s->exp = e;
    return (Stmt *)s;
}

Stmt *stmt_from_func_decl(Ident name, Vector *type_params, Vector *args,
                          ExprType *ret, Vector *body)
{
    FuncDeclStmt *s = mm_alloc(sizeof(*s));
    s->kind = STMT_FUNC_KIND;
    s->id = name;
    s->type_params = type_params;
    s->args = args;
    if (ret) s->ret = *ret;
    s->body = body;
    return (Stmt *)s;
}

void free_stmt(Stmt *s)
{
    if (!s) return;
    switch (s->kind) {
        case STMT_EXPR_KIND: {
            ExprStmt *es = (ExprStmt *)s;
            free_expr(es->exp);
            mm_free(s);
            break;
        }
        default:
            break;
    }
}

#ifdef __cplusplus
}
#endif
