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
    ty->stbl = NULL;
    return ty;
}

Type *int16_type(void)
{
    Type *ty = mm_alloc_obj(ty);
    ty->desc = desc_int16();
    ty->stbl = NULL;
    return ty;
}

Type *int32_type(void)
{
    Type *ty = mm_alloc_obj(ty);
    ty->desc = desc_int32();
    ty->stbl = NULL;
    return ty;
}

Type *int64_type(void)
{
    Type *ty = mm_alloc_obj(ty);
    ty->desc = desc_int64();
    ty->stbl = NULL;
    return ty;
}

Type *float32_type(void)
{
    Type *ty = mm_alloc_obj(ty);
    ty->desc = desc_float32();
    ty->stbl = NULL;
    return ty;
}

Type *float64_type(void)
{
    Type *ty = mm_alloc_obj(ty);
    ty->desc = desc_float64();
    ty->stbl = NULL;
    return ty;
}

Type *bool_type(void)
{
    Type *ty = mm_alloc_obj(ty);
    ty->desc = desc_bool();
    ty->stbl = NULL;
    return ty;
}

Type *str_type(void)
{
    Type *ty = mm_alloc_obj(ty);
    ty->desc = desc_str();
    ty->stbl = NULL;
    return ty;
}

Type *object_type(void)
{
    Type *ty = mm_alloc_obj(ty);
    ty->desc = desc_object();
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
    exp->desc = (TypeDesc *)&str_desc;
    buf->len = 0;
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
