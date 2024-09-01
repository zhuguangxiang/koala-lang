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
    ty->desc = desc_int8();
    return ty;
}

Type *int16_type(void)
{
    Type *ty = mm_alloc_obj(ty);
    ty->desc = desc_int16();
    return ty;
}

Type *int32_type(void)
{
    Type *ty = mm_alloc_obj(ty);
    ty->desc = desc_int32();
    return ty;
}

Type *int64_type(void)
{
    Type *ty = mm_alloc_obj(ty);
    ty->desc = desc_int64();
    return ty;
}

Type *float32_type(void)
{
    Type *ty = mm_alloc_obj(ty);
    ty->desc = desc_float32();
    return ty;
}

Type *float64_type(void)
{
    Type *ty = mm_alloc_obj(ty);
    ty->desc = desc_float64();
    return ty;
}

Type *bool_type(void)
{
    Type *ty = mm_alloc_obj(ty);
    ty->desc = desc_bool();
    return ty;
}

Type *str_type(void)
{
    Type *ty = mm_alloc_obj(ty);
    ty->desc = desc_str();
    return ty;
}

Type *object_type(void)
{
    Type *ty = mm_alloc_obj(ty);
    ty->desc = desc_object();
    return ty;
}

Type *optional_type(Type *sub)
{
    Type *ty = mm_alloc_obj(ty);
    ty->desc = desc_optional(sub->desc);
    ty->subs = vector_create_ptr();
    vector_push_back(ty->subs, &sub);
    return ty;
}

Type *array_type(Type *sub)
{
    Type *ty = mm_alloc_obj(ty);
    ty->desc = desc_array((sub ? sub->desc : NULL));
    if (sub) {
        ty->subs = vector_create_ptr();
        vector_push_back(ty->subs, &sub);
    }
    return ty;
}

void free_type(Type *ty)
{
    if (!ty) return;

    free_desc(ty->desc);

    Type **item;
    vector_foreach(item, ty->subs) {
        free_type(*item);
    }
    vector_destroy(ty->subs);
    mm_free(ty);
}

Expr *expr_from_lit_int(int64_t val)
{
    LitExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_LITERAL_KIND;
    exp->which = LIT_EXPR_INT;
    exp->ival = val;
    exp->desc = desc_int64();
    return (Expr *)exp;
}

Expr *expr_from_lit_float(double val)
{
    LitExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_LITERAL_KIND;
    exp->which = LIT_EXPR_FLT;
    exp->fval = val;
    exp->desc = desc_float64();
    return (Expr *)exp;
}

Expr *expr_from_lit_bool(int val)
{
    LitExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_LITERAL_KIND;
    exp->which = LIT_EXPR_BOOL;
    exp->bval = val;
    exp->desc = desc_bool();
    return (Expr *)exp;
}

static char esc_char(char ch)
{
    char val;
    switch (ch) {
        case 'a':
            val = 7;
            break;
        case 'b':
            val = 8;
            break;
        case 'f':
            val = 12;
            break;
        case 'n':
            val = 10;
            break;
        case 'r':
            val = 13;
            break;
        case 't':
            val = 9;
            break;
        case 'v':
            val = 11;
            break;
        default:
            val = ch;
            break;
    }
    return val;
}

static void do_escape(Buffer *buf)
{
    char *s = buf->buf;
    int len = buf->len;
    if (len == 0) return;

    int j = 0;
    char ch;
    for (int i = 0; i < len; ++i) {
        ch = s[i];
        if (ch == '\\') {
            if (i + 1 < len) {
                ch = s[++i];
                s[j++] = esc_char(ch);
            }
        } else {
            s[j++] = ch;
        }
    }
    ASSERT(j <= len);
    buf->len = j;
    s[j] = '\0';
}

Expr *expr_from_lit_str(Buffer *buf)
{
    LitExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_LITERAL_KIND;
    exp->which = LIT_EXPR_STR;
    exp->sval = mm_alloc_fast(buf->len + 1);
    memcpy(exp->sval, buf->buf, buf->len);
    exp->sval[buf->len] = '\0';
    exp->len = buf->len;
    exp->desc = desc_str();
    buf->len = 0;
    return (Expr *)exp;
}

Expr *expr_from_lit_none(void)
{
    LitExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_LITERAL_KIND;
    exp->which = LIT_EXPR_NONE;
    return (Expr *)exp;
}

Expr *expr_from_ident(Ident *id)
{
    IdentExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_ID_KIND;
    exp->id = *id;
    return (Expr *)exp;
}

Expr *expr_from_under(void)
{
    Expr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_UNDER_KIND;
    return exp;
}

Expr *expr_from_self(void)
{
    Expr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_SELF_KIND;
    return exp;
}

Expr *expr_from_super(void)
{
    Expr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_SUPER_KIND;
    return exp;
}

void expr_free(Expr *exp) { mm_free(exp); }

Stmt *stmt_from_var_decl(Ident id, Type *ty, int ro, Expr *e)
{
    VarDeclStmt *s = mm_alloc_obj(s);
    s->kind = STMT_VAR_KIND;
    s->id = id;
    s->ro = ro;
    s->type = ty;
    s->exp = e;
    return (Stmt *)s;
}

Stmt *stmt_from_assignment(AssignOpKind op, Expr *lhs, Expr *rhs)
{
    AssignStmt *s = mm_alloc_obj(s);
    s->kind = STMT_ASSIGN_KIND;
    s->op = op;
    s->lhs = lhs;
    s->rhs = rhs;
    return (Stmt *)s;
}

Stmt *stmt_from_block(Vector *stmts)
{
    BlockStmt *s = mm_alloc_obj(s);
    s->kind = STMT_BLOCK_KIND;
    s->stmts = stmts;
    return (Stmt *)s;
}

Stmt *stmt_from_if(Expr *cond, Vector *block, Stmt *_else)
{
    IfStmt *s = mm_alloc_obj(s);
    s->kind = STMT_IF_KIND;
    s->cond = cond;
    s->block = block;
    s->_else = _else;
    return (Stmt *)s;
}

Stmt *stmt_from_if_let(Ident *id, Expr *exp, Vector *block)
{
    IfLetStmt *s = mm_alloc_obj(s);
    s->kind = STMT_IF_LET_KIND;
    s->id = *id;
    s->exp = exp;
    s->block = block;
    return (Stmt *)s;
}

Stmt *stmt_from_guard_let(Ident *id, Expr *exp, Vector *block)
{
    GuardLetStmt *s = mm_alloc_obj(s);
    s->kind = STMT_IF_LET_KIND;
    s->id = *id;
    s->exp = exp;
    s->block = block;
    return (Stmt *)s;
}

Stmt *stmt_from_expr(Expr *exp)
{
    ExprStmt *s = mm_alloc_obj(s);
    s->kind = STMT_IF_LET_KIND;
    s->exp = exp;
    return (Stmt *)s;
}

void stmt_free(Stmt *stmt) { ASSERT(0); }

#ifdef __cplusplus
}
#endif
