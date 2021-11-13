/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "ast.h"
#include "util/log.h"
#include "util/mm.h"

#ifdef __cplusplus
extern "C" {
#endif

ExprType *expr_type_ref(TypeDesc *ty)
{
    ExprType *type = mm_alloc_obj(type);
    type->ty = ty;
    type->ref = 1;
    return type;
}

ExprType *expr_type_any(void)
{
    ExprType *type = mm_alloc_obj(type);
    type->ty = desc_from_any();
    return type;
}

ExprType *expr_type_int8(void)
{
    ExprType *type = mm_alloc_obj(type);
    type->ty = desc_from_int8();
    return type;
}

ExprType *expr_type_int16(void)
{
    ExprType *type = mm_alloc_obj(type);
    type->ty = desc_from_int16();
    return type;
}

ExprType *expr_type_int32(void)
{
    ExprType *type = mm_alloc_obj(type);
    type->ty = desc_from_int32();
    return type;
}

ExprType *expr_type_int64(void)
{
    ExprType *type = mm_alloc_obj(type);
    type->ty = desc_from_int64();
    return type;
}

ExprType *expr_type_float32(void)
{
    ExprType *type = mm_alloc_obj(type);
    type->ty = desc_from_float32();
    return type;
}

ExprType *expr_type_float64(void)
{
    ExprType *type = mm_alloc_obj(type);
    type->ty = desc_from_float64();
    return type;
}

ExprType *expr_type_bool(void)
{
    ExprType *type = mm_alloc_obj(type);
    type->ty = desc_from_bool();
    return type;
}

ExprType *expr_type_char(void)
{
    ExprType *type = mm_alloc_obj(type);
    type->ty = desc_from_char();
    return type;
}

ExprType *expr_type_str(void)
{
    ExprType *type = mm_alloc_obj(type);
    type->ty = desc_from_str();
    return type;
}

Vector *get_param_types(Vector *args)
{
    if (!args) return null;

    Vector *param_types = vector_create_ptr();

    ExprType **item;
    ExprType *type;
    vector_foreach(item, args, {
        type = *item;
#if !defined(NLOG)
        BUF(buf);
        desc_to_str(type->ty, &buf);
        debug("param_types[%d]: %s", i, BUF_STR(buf));
        FINI_BUF(buf);
#endif
        vector_push_back(param_types, &type->ty);
    });
    return param_types;
}

ExprType *expr_type_klass(Ident *pkg, Ident *name, Vector *args)
{
    ExprKlassType *type = mm_alloc_obj(type);
    type->pkg = *pkg;
    type->id = *name;
    type->args = args;
    debug("instance of type '%s':", name->str);
    Vector *param_types = get_param_types(args);
    debug("end");
    type->ty = desc_from_klass(pkg->str, name->str, param_types);
    return (ExprType *)type;
}

ExprType *expr_type_array(ExprType *sub)
{
    ExprType *type = mm_alloc_obj(type);
    return type;
}

ExprType *expr_type_map(ExprType *kty, ExprType *vty)
{
    ExprType *type = mm_alloc_obj(type);
    return type;
}

ExprType *expr_type_tuple(Vector *subs)
{
    ExprType *type = mm_alloc_obj(type);
    return type;
}

ExprType *expr_type_proto(ExprType *ret, Vector *params)
{
    ExprType *type = mm_alloc_obj(type);
    return type;
}

Expr *expr_from_nil(void)
{
    Expr *e = mm_alloc_obj(e);
    e->kind = EXPR_NIL_KIND;
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

Expr *expr_from_int64(int64 val)
{
    ConstExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_INT_KIND;
    exp->ival = val;
    exp->type = expr_type_int64();
    return (Expr *)exp;
}

Expr *expr_from_float64(double val)
{
    ConstExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_FLOAT_KIND;
    exp->fval = val;
    exp->type = expr_type_float64();
    return (Expr *)exp;
}

Expr *expr_from_bool(int val)
{
    ConstExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_BOOL_KIND;
    exp->bval = val;
    exp->type = expr_type_bool();
    return (Expr *)exp;
}

Expr *expr_from_str(char *val)
{
    ConstExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_STR_KIND;
    exp->sval = val;
    exp->type = expr_type_str();
    return (Expr *)exp;
}

Expr *expr_from_char(int val)
{
    ConstExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_CHAR_KIND;
    exp->cval = val;
    exp->type = expr_type_char();
    return (Expr *)exp;
}

Expr *expr_from_ident(Ident *val, ExprType *ty)
{
    IdExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_ID_KIND;
    exp->type = ty;
    exp->id = *val;
    return (Expr *)exp;
}

Expr *expr_from_unary(UnOpKind op, Loc oploc, Expr *e)
{
    UnaryExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_UNARY_KIND;
    exp->uop = op;
    exp->exp = e;
    exp->oploc = oploc;
    return (Expr *)exp;
}

Expr *expr_from_binary(BinOpKind op, Loc oploc, Expr *le, Expr *re)
{
    BinaryExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_BINARY_KIND;
    exp->bop = op;
    exp->oploc = oploc;
    exp->lexp = le;
    exp->rexp = re;
    return (Expr *)exp;
}

Expr *expr_from_type(ExprType *ty)
{
    Expr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_TYPE_KIND;
    return exp;
}

void free_expr(Expr *e)
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

Stmt *stmt_from_letdecl(Ident *name, ExprType *ty, Expr *e)
{
    VarDeclStmt *s = mm_alloc(sizeof(*s));
    s->kind = STMT_LET_KIND;
    s->id = *name;
    s->type = ty;
    s->exp = e;
    return (Stmt *)s;
}

Stmt *stmt_from_vardecl(Ident *name, ExprType *ty, Expr *e)
{
    VarDeclStmt *s = mm_alloc(sizeof(*s));
    s->kind = STMT_VAR_KIND;
    s->id = *name;
    s->type = ty;
    s->exp = e;
    return (Stmt *)s;
}

Stmt *stmt_from_assign(AssignOpKind op, Loc oploc, Expr *lhs, Expr *rhs)
{
    AssignStmt *s = mm_alloc(sizeof(*s));
    s->kind = STMT_ASSIGN_KIND;
    s->lhs = lhs;
    s->oploc = oploc;
    Expr *rexp = null;
    switch (op) {
        case OP_ASSIGN:
            rexp = rhs;
            break;
        case OP_PLUS_ASSIGN:
            rexp = expr_from_binary(BINARY_ADD, oploc, lhs, rhs);
            expr_set_loc(rexp, rhs->loc);
            break;
        case OP_MINUS_ASSIGN:
            rexp = expr_from_binary(BINARY_SUB, oploc, lhs, rhs);
            expr_set_loc(rexp, rhs->loc);
            break;
        case OP_MULT_ASSIGN:
            rexp = expr_from_binary(BINARY_MULT, oploc, lhs, rhs);
            expr_set_loc(rexp, rhs->loc);
            break;
        case OP_DIV_ASSIGN:
            rexp = expr_from_binary(BINARY_DIV, oploc, lhs, rhs);
            expr_set_loc(rexp, rhs->loc);
            break;
        case OP_MOD_ASSIGN:
            rexp = expr_from_binary(BINARY_MOD, oploc, lhs, rhs);
            expr_set_loc(rexp, rhs->loc);
            break;
        case OP_SHL_ASSIGN:
            rexp = expr_from_binary(BINARY_SHL, oploc, lhs, rhs);
            expr_set_loc(rexp, rhs->loc);
            break;
        case OP_SHR_ASSIGN:
            rexp = expr_from_binary(BINARY_SHR, oploc, lhs, rhs);
            expr_set_loc(rexp, rhs->loc);
            break;
        case OP_AND_ASSIGN:
            rexp = expr_from_binary(BINARY_BIT_AND, oploc, lhs, rhs);
            expr_set_loc(rexp, rhs->loc);
            break;
        case OP_OR_ASSIGN:
            rexp = expr_from_binary(BINARY_BIT_OR, oploc, lhs, rhs);
            expr_set_loc(rexp, rhs->loc);
            break;
        case OP_XOR_ASSIGN:
            rexp = expr_from_binary(BINARY_BIT_XOR, oploc, lhs, rhs);
            expr_set_loc(rexp, rhs->loc);
            break;
        default:
            expect(0);
            break;
    }
    s->rhs = rexp;
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

Stmt *stmt_from_funcdecl(Ident *name, Vector *param_types, Vector *args,
                         ExprType *ret, Vector *body)
{
    FuncDeclStmt *s = mm_alloc(sizeof(*s));
    s->kind = STMT_FUNC_KIND;
    s->id = *name;
    s->param_types = param_types;
    s->args = args;
    s->ret = ret;
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
