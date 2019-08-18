/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include <inttypes.h>
#include "parser.h"
#include "memory.h"
#include "strbuf.h"

Expr *expr_from_nil(void)
{
  Expr *exp = kmalloc(sizeof(Expr));
  exp->kind = NIL_KIND;
  return exp;
}

Expr *expr_from_self(void)
{
  Expr *exp = kmalloc(sizeof(Expr));
  exp->kind = SELF_KIND;
  return exp;
}

Expr *expr_from_super(void)
{
  Expr *exp = kmalloc(sizeof(Expr));
  exp->kind = SUPER_KIND;
  return exp;
}

Expr *expr_from_integer(int64_t val)
{
  Expr *exp = kmalloc(sizeof(Expr));
  exp->kind = LITERAL_KIND;
  exp->desc = desc_getbase(BASE_INT);
  TYPE_INCREF(exp->desc);
  exp->k.value.kind = BASE_INT;
  exp->k.value.ival = val;
  return exp;
}

Expr *expr_from_float(double val)
{
  Expr *exp = kmalloc(sizeof(Expr));
  exp->kind = LITERAL_KIND;
  exp->desc = desc_getbase(BASE_FLOAT);
  TYPE_INCREF(exp->desc);
  exp->k.value.kind = BASE_FLOAT;
  exp->k.value.fval = val;
  return exp;
}

Expr *expr_from_bool(int val)
{
  Expr *exp = kmalloc(sizeof(Expr));
  exp->kind = LITERAL_KIND;
  exp->desc = desc_getbase(BASE_BOOL);
  TYPE_INCREF(exp->desc);
  exp->k.value.kind = BASE_BOOL;
  exp->k.value.bval = val;
  return exp;
}

Expr *expr_from_string(char *val)
{
  Expr *exp = kmalloc(sizeof(Expr));
  exp->kind = LITERAL_KIND;
  exp->desc = desc_getbase(BASE_STR);
  TYPE_INCREF(exp->desc);
  exp->k.value.kind = BASE_STR;
  exp->k.value.str = val;
  return exp;
}

Expr *expr_from_char(wchar val)
{
  Expr *exp = kmalloc(sizeof(Expr));
  exp->kind = LITERAL_KIND;
  exp->desc = desc_getbase(BASE_CHAR);
  TYPE_INCREF(exp->desc);
  exp->k.value.kind = BASE_CHAR;
  exp->k.value.cval = val;
  return exp;
}

Expr *expr_from_ident(char *val)
{
  Expr *exp = kmalloc(sizeof(Expr));
  exp->kind = ID_KIND;
  exp->id.name = val;
  return exp;
}

Expr *expr_from_unary(UnaryOpKind op, Expr *exp)
{
  Expr *uexp = kmalloc(sizeof(Expr));
  uexp->kind = UNARY_KIND;
  /* it does not matter that exp->desc is null */
  uexp->desc = exp->desc;
  TYPE_INCREF(uexp->desc);
  uexp->unary.op = op;
  uexp->unary.exp = exp;
  return uexp;
}

Expr *expr_from_binary(BinaryOpKind op, Expr *left, Expr *right)
{
  Expr *exp = kmalloc(sizeof(Expr));
  exp->kind = BINARY_KIND;
  /* it does not matter that exp->desc is null */
  exp->desc = left->desc;
  TYPE_INCREF(exp->desc);
  exp->binary.op = op;
  exp->binary.lexp = left;
  exp->binary.rexp = right;
  return exp;
}

Expr *expr_from_attribute(Ident id, Expr *left)
{
  Expr *exp = kmalloc(sizeof(Expr));
  exp->kind = ATTRIBUTE_KIND;
  exp->attr.id = id;
  exp->attr.lexp = left;
  left->right = exp;
  return exp;
}

Expr *expr_from_subScript(Expr *index, Expr *left)
{
  Expr *exp = kmalloc(sizeof(Expr));
  exp->kind = SUBSCRIPT_KIND;
  exp->subscript.index = index;
  exp->subscript.lexp = left;
  left->right = exp;
  return exp;
}

Expr *expr_from_call(Vector *args, Expr *left)
{
  Expr *exp = kmalloc(sizeof(Expr));
  exp->kind = CALL_KIND;
  exp->call.args = args;
  exp->call.lexp = left;
  left->right = exp;
  return exp;
}

Expr *expr_from_slice(Expr *start, Expr *end, Expr *left)
{
  Expr *exp = kmalloc(sizeof(Expr));
  exp->kind = SLICE_KIND;
  exp->slice.start = start;
  exp->slice.end = end;
  exp->slice.lexp = left;
  left->right = exp;
  return exp;
}

void expr_free(Expr *exp)
{
  switch (exp->kind) {
  case NIL_KIND:
  case SELF_KIND:
  case SUPER_KIND:
    kfree(exp);
    break;
  case LITERAL_KIND:
    TYPE_DECREF(exp->desc);
    kfree(exp);
    break;
  case ID_KIND:
    kfree(exp);
    break;
  case UNARY_KIND:
    TYPE_DECREF(exp->desc);
    expr_free(exp->unary.exp);
    kfree(exp);
    break;
  case BINARY_KIND:
    TYPE_DECREF(exp->desc);
    expr_free(exp->binary.lexp);
    expr_free(exp->binary.rexp);
    kfree(exp);
    break;
  case ATTRIBUTE_KIND:
    TYPE_DECREF(exp->desc);
    expr_free(exp->attr.lexp);
    kfree(exp);
    break;
  case SUBSCRIPT_KIND:
    TYPE_DECREF(exp->desc);
    expr_free(exp->subscript.index);
    expr_free(exp->subscript.lexp);
    kfree(exp);
    break;
  case CALL_KIND:
    TYPE_DECREF(exp->desc);
    //exprlist_free(exp->call.args);
    expr_free(exp->call.lexp);
    kfree(exp);
    break;
  case SLICE_KIND:
    TYPE_DECREF(exp->desc);
    expr_free(exp->slice.start);
    expr_free(exp->slice.end);
    expr_free(exp->slice.lexp);
    kfree(exp);
    break;
  default:
    panic("invalid branch %d", exp->kind);
    break;
  }
}

Stmt *stmt_from_constdecl(Ident id, Type type, Expr *exp)
{
  Stmt *stmt = kmalloc(sizeof(Stmt));
  stmt->kind = CONST_KIND;
  stmt->vardecl.id = id;
  stmt->vardecl.type = type;
  stmt->vardecl.exp = exp;
  return stmt;
}

Stmt *stmt_from_vardecl(Ident id, Type type, Expr *exp)
{
  Stmt *stmt = kmalloc(sizeof(Stmt));
  stmt->kind = VAR_KIND;
  stmt->vardecl.id = id;
  stmt->vardecl.type = type;
  stmt->vardecl.exp = exp;
  return stmt;
}

Stmt *stmt_from_assign(AssignOpKind op, Expr *left, Expr *right)
{
  Stmt *stmt = kmalloc(sizeof(Stmt));
  stmt->kind = ASSIGN_KIND;
  stmt->assign.op = op;
  stmt->assign.lexp = left;
  stmt->assign.rexp = right;
  return stmt;
}

Stmt *stmt_from_funcdecl(Ident id, Vector *typeparams, Vector *args,
                         Type ret, Vector *stmts)
{
  Stmt *stmt = kmalloc(sizeof(Stmt));
  stmt->kind = FUNC_KIND;
  stmt->funcdecl.id = id;
  stmt->funcdecl.args = args;
  stmt->funcdecl.ret = ret;
  stmt->funcdecl.body = stmts;
  return stmt;
}

Stmt *stmt_from_return(Expr *exp)
{
  Stmt *stmt = kmalloc(sizeof(Stmt));
  stmt->kind = RETURN_KIND;
  stmt->ret.exp = exp;
  return stmt;
}

Stmt *stmt_from_expr(Expr *exp)
{
  Stmt *stmt = kmalloc(sizeof(Stmt));
  stmt->kind = EXPR_KIND;
  stmt->expr.exp = exp;
  return stmt;
}

 void stmt_free(Stmt *stmt)
{
  switch (stmt->kind) {
  case IMPORT_KIND:
    kfree(stmt);
    break;
  case CONST_KIND:
    kfree(stmt);
    break;
  case VAR_KIND:
    kfree(stmt);
    break;
  case ASSIGN_KIND:
    kfree(stmt);
    break;
  case FUNC_KIND:
    kfree(stmt);
    break;
  case RETURN_KIND:
    expr_free(stmt->ret.exp);
    kfree(stmt);
    break;
  case EXPR_KIND:
    expr_free(stmt->expr.exp);
    kfree(stmt);
    break;
  default:
    panic("invalid branch %d", stmt->kind);
    break;
  }
}
