/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2023 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "ast.h"

#ifdef __cplusplus
extern "C" {
#endif

Type *int8_type(void)
{
    Type *ty = mm_alloc_obj(ty);
    ty->desc = (TypeDesc *)&int8_desc;
    ty->stbl = NULL;
    return ty;
}

Type *int16_type(void)
{
    Type *ty = mm_alloc_obj(ty);
    ty->desc = (TypeDesc *)&int16_desc;
    ty->stbl = NULL;
    return ty;
}

Type *int32_type(void)
{
    Type *ty = mm_alloc_obj(ty);
    ty->desc = (TypeDesc *)&int32_desc;
    ty->stbl = NULL;
    return ty;
}

Type *int64_type(void)
{
    Type *ty = mm_alloc_obj(ty);
    ty->desc = (TypeDesc *)&int64_desc;
    ty->stbl = NULL;
    return ty;
}

Type *float32_type(void)
{
    Type *ty = mm_alloc_obj(ty);
    ty->desc = (TypeDesc *)&float32_desc;
    ty->stbl = NULL;
    return ty;
}

Type *float64_type(void)
{
    Type *ty = mm_alloc_obj(ty);
    ty->desc = (TypeDesc *)&float64_desc;
    ty->stbl = NULL;
    return ty;
}

Type *bool_type(void)
{
    Type *ty = mm_alloc_obj(ty);
    ty->desc = (TypeDesc *)&bool_desc;
    ty->stbl = NULL;
    return ty;
}

Type *object_type(void)
{
    Type *ty = mm_alloc_obj(ty);
    ty->desc = (TypeDesc *)&object_desc;
    ty->stbl = NULL;
    return ty;
}

Expr *expr_from_lit_int(int64_t val)
{
    LitExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_LITERAL_KIND;
    exp->which = LIT_EXPR_INT;
    exp->ival = val;
    exp->desc = (TypeDesc *)&int64_desc;
    return (Expr *)exp;
}

Expr *expr_from_lit_float(double val)
{
    LitExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_LITERAL_KIND;
    exp->which = LIT_EXPR_FLT;
    exp->fval = val;
    exp->desc = (TypeDesc *)&float64_desc;
    return (Expr *)exp;
}

Expr *expr_from_lit_bool(int val)
{
    LitExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_LITERAL_KIND;
    exp->which = LIT_EXPR_BOOL;
    exp->bval = val;
    exp->desc = (TypeDesc *)&bool_desc;
    return (Expr *)exp;
}

Stmt *stmt_from_var_decl(Ident id, Type *ty, int ro, int pub, Expr *e)
{
    VarDeclStmt *s = mm_alloc_obj(s);
    s->kind = STMT_VAR_KIND;
    s->id = id;
    s->ro = ro;
    s->pub = pub;
    s->type = ty;
    s->exp = e;
    return (Stmt *)s;
}

void stmt_free(Stmt *stmt) {}

#ifdef __cplusplus
}
#endif
