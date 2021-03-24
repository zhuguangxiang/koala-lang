/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "ast.h"
#include "mm.h"

#ifdef __cplusplus
extern "C" {
#endif

ExprRef expr_from_nil(void)
{
    ExprRef e = mm_alloc(sizeof(*e));
    e->kind = EXPR_NIL_KIND;
    return e;
}

ExprRef expr_from_self(void)
{
    ExprRef e = mm_alloc(sizeof(*e));
    e->kind = EXPR_SELF_KIND;
    return e;
}

ExprRef expr_from_super(void)
{
    ExprRef e = mm_alloc(sizeof(*e));
    e->kind = EXPR_SUPER_KIND;
    return e;
}

ExprRef expr_from_under(void)
{
    ExprRef e = mm_alloc(sizeof(*e));
    e->kind = EXPR_UNDER_KIND;
    return e;
}

ExprRef expr_from_int64(int64_t val)
{
    ExprKRef k = mm_alloc(sizeof(*k));
    k->kind = EXPR_INT_KIND;
    k->ival = val;
    return (ExprRef)k;
}

ExprRef expr_from_float64(double val)
{
    ExprKRef k = mm_alloc(sizeof(*k));
    k->kind = EXPR_FLOAT_KIND;
    k->fval = val;
    return (ExprRef)k;
}

ExprRef expr_from_bool(int val)
{
    ExprKRef k = mm_alloc(sizeof(*k));
    k->kind = EXPR_BOOL_KIND;
    k->bval = val;
    return (ExprRef)k;
}

ExprRef expr_from_str(char *val)
{
    ExprKRef k = mm_alloc(sizeof(*k));
    k->kind = EXPR_STR_KIND;
    k->sval = val;
    return (ExprRef)k;
}

ExprRef expr_from_char(int val)
{
    ExprKRef k = mm_alloc(sizeof(*k));
    k->kind = EXPR_CHAR_KIND;
    k->cval = val;
    return (ExprRef)k;
}

ExprRef expr_from_ident(char *val)
{
    ExprIDRef id = mm_alloc(sizeof(*id));
    id->kind = EXPR_ID_KIND;
    id->name = val;
    return (ExprRef)id;
}

ExprRef expr_from_unary(UnKind ukind, ExprRef e)
{
    ExprUnRef un = mm_alloc(sizeof(*un));
    un->kind = EXPR_UNARY_KIND;
    un->ukind = ukind;
    un->exp = e;
    return (ExprRef)un;
}

ExprRef expr_from_binary(BiKind bkind, ExprRef le, ExprRef re)
{
    ExprBiRef bi = mm_alloc(sizeof(*bi));
    bi->kind = EXPR_BINARY_KIND;
    bi->bkind = bkind;
    bi->lexp = le;
    bi->rexp = re;
    return (ExprRef)bi;
}

void free_expr(ExprRef e)
{
    if (!e) return;
    switch (e->kind) {
        case EXPR_NIL_KIND:
        case EXPR_SELF_KIND:
        case EXPR_SUPER_KIND:
        case EXPR_INT_KIND:
        case EXPR_FLOAT_KIND:
        case EXPR_BOOL_KIND:
        case EXPR_CHAR_KIND:
        case EXPR_STR_KIND:
        case EXPR_ID_KIND:
        case EXPR_UNDER_KIND:
            mm_free(e);
            break;
        case EXPR_UNARY_KIND: {
            ExprUnRef un = (ExprUnRef)e;
            free_expr(un->exp);
            mm_free(e);
            break;
        }
        case EXPR_BINARY_KIND: {
            ExprBiRef bi = (ExprBiRef)e;
            free_expr(bi->lexp);
            free_expr(bi->rexp);
            mm_free(e);
            break;
        }
        default:
            break;
    }
}

StmtRef stmt_from_expr(ExprRef e)
{
    StmtExprRef s = mm_alloc(sizeof(*s));
    s->kind = STMT_EXPR_KIND;
    s->exp = e;
    return (StmtRef)s;
}

void free_stmt(StmtRef s)
{
    if (!s) return;
    switch (s->kind) {
        case STMT_EXPR_KIND: {
            StmtExprRef s_e = (StmtExprRef)s;
            free_expr(s_e->exp);
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
