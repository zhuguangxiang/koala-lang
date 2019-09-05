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
  exp->desc = desc_from_base(BASE_INT);
  exp->k.value.kind = BASE_INT;
  exp->k.value.ival = val;
  return exp;
}

Expr *expr_from_float(double val)
{
  Expr *exp = kmalloc(sizeof(Expr));
  exp->kind = LITERAL_KIND;
  exp->desc = desc_from_base(BASE_FLOAT);
  exp->k.value.kind = BASE_FLOAT;
  exp->k.value.fval = val;
  return exp;
}

Expr *expr_from_bool(int val)
{
  Expr *exp = kmalloc(sizeof(Expr));
  exp->kind = LITERAL_KIND;
  exp->desc = desc_from_base(BASE_BOOL);
  exp->k.value.kind = BASE_BOOL;
  exp->k.value.bval = val;
  return exp;
}

Expr *expr_from_string(char *val)
{
  Expr *exp = kmalloc(sizeof(Expr));
  exp->kind = LITERAL_KIND;
  exp->desc = desc_from_base(BASE_STR);
  exp->k.value.kind = BASE_STR;
  exp->k.value.str = val;
  return exp;
}

Expr *expr_from_char(wchar val)
{
  Expr *exp = kmalloc(sizeof(Expr));
  exp->kind = LITERAL_KIND;
  exp->desc = desc_from_base(BASE_CHAR);
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

Expr *expr_from_ternary(Expr *cond, Expr *left, Expr *right)
{
  Expr *exp = kmalloc(sizeof(Expr));
  exp->kind = TERNARY_KIND;
  exp->ternary.cond = cond;
  exp->ternary.lexp = left;
  exp->ternary.rexp = right;
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

Expr *expr_from_subScript(Expr *left, Expr *index)
{
  Expr *exp = kmalloc(sizeof(Expr));
  exp->kind = SUBSCRIPT_KIND;
  exp->subscr.index = index;
  exp->subscr.lexp = left;
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

Expr *expr_from_slice(Expr *left, Expr *start, Expr *end)
{
  Expr *exp = kmalloc(sizeof(Expr));
  exp->kind = SLICE_KIND;
  exp->slice.start = start;
  exp->slice.end = end;
  exp->slice.lexp = left;
  left->right = exp;
  return exp;
}

Expr *expr_from_tuple(Vector *exps)
{
  Expr *exp = kmalloc(sizeof(Expr));
  exp->kind = TUPLE_KIND;
  exp->tuple = exps;
  exp->sym = find_from_builtins("Tuple");
  return exp;
}

static TypeDesc *get_subarray_type(Vector *exps)
{
  if (exps == NULL)
    return desc_from_any;

  TypeDesc *desc = NULL;
  VECTOR_ITERATOR(iter, exps);
  Expr *exp;
  iter_for_each(&iter, exp) {
    if (desc == NULL) {
      desc = exp->desc;
    } else {
      if (!desc_check(desc, exp->desc)) {
        return desc_from_any;
      }
    }
  }
  return TYPE_INCREF(desc);
}

Expr *expr_from_array(Vector *exps)
{
  Expr *exp = kmalloc(sizeof(Expr));
  exp->kind = ARRAY_KIND;
  TypeDesc *para = get_subarray_type(exps);
  exp->desc = desc_from_array(para);
  exp->array.elems = exps;
  exp->sym = find_from_builtins("Array");
  return exp;
}

Expr *expr_from_mapentry(Expr *key, Expr *val)
{
  Expr *exp = kmalloc(sizeof(Expr));
  exp->kind = MAP_ENTRY_KIND;
  exp->mapentry.key = key;
  exp->mapentry.val = val;
  return exp;
}

Expr *expr_from_map(Vector *exps)
{
  Expr *exp = kmalloc(sizeof(Expr));
  exp->kind = MAP_KIND;
  exp->map = exps;
  exp->sym = find_from_builtins("Dict");
  return exp;
}

Expr *expr_from_istype(Expr *e, Type type)
{
  Expr *exp = kmalloc(sizeof(Expr));
  exp->kind = IS_KIND;
  exp->isas.exp = e;
  exp->isas.type = type;
  return exp;
}

Expr *expr_from_astype(Expr *e, Type type)
{
  Expr *exp = kmalloc(sizeof(Expr));
  exp->kind = AS_KIND;
  exp->isas.exp = e;
  exp->isas.type = type;
  return exp;
}

void exprlist_free(Vector *vec)
{
  VECTOR_ITERATOR(iter, vec);
  Expr *exp;
  iter_for_each(&iter, exp) {
    expr_free(exp);
  }
  vector_free(vec, NULL, NULL);
}

void expr_free(Expr *exp)
{
  if (exp == NULL)
    return;

  TYPE_DECREF(exp->desc);

  switch (exp->kind) {
  case NIL_KIND:
  case SELF_KIND:
  case SUPER_KIND:
    kfree(exp);
    break;
  case LITERAL_KIND:
    kfree(exp);
    break;
  case ID_KIND:
    kfree(exp);
    break;
  case UNARY_KIND:
    expr_free(exp->unary.exp);
    kfree(exp);
    break;
  case BINARY_KIND:
    expr_free(exp->binary.lexp);
    expr_free(exp->binary.rexp);
    kfree(exp);
    break;
  case TERNARY_KIND:
    expr_free(exp->ternary.cond);
    expr_free(exp->ternary.lexp);
    expr_free(exp->ternary.rexp);
    kfree(exp);
    break;
  case ATTRIBUTE_KIND:
    expr_free(exp->attr.lexp);
    kfree(exp);
    break;
  case SUBSCRIPT_KIND:
    expr_free(exp->subscr.index);
    expr_free(exp->subscr.lexp);
    kfree(exp);
    break;
  case CALL_KIND:
    exprlist_free(exp->call.args);
    expr_free(exp->call.lexp);
    kfree(exp);
    break;
  case SLICE_KIND:
    expr_free(exp->slice.start);
    expr_free(exp->slice.end);
    expr_free(exp->slice.lexp);
    kfree(exp);
    break;
  case TUPLE_KIND:
    exprlist_free(exp->tuple);
    kfree(exp);
    break;
  case ARRAY_KIND:
    exprlist_free(exp->array.elems);
    kfree(exp);
    break;
  case MAP_ENTRY_KIND:
    expr_free(exp->mapentry.key);
    expr_free(exp->mapentry.val);
    kfree(exp);
    break;
  case MAP_KIND:
    exprlist_free(exp->map);
    kfree(exp);
    break;
  case IS_KIND:
  case AS_KIND:
    TYPE_DECREF(exp->isas.type.desc);
    expr_free(exp->isas.exp);
    kfree(exp);
    break;
  default:
    panic("invalid branch %d", exp->kind);
    break;
  }
}

Stmt *stmt_from_constdecl(Ident id, Type *type, Expr *exp)
{
  Stmt *stmt = kmalloc(sizeof(Stmt));
  stmt->kind = CONST_KIND;
  stmt->vardecl.id = id;
  if (type != NULL)
    stmt->vardecl.type = *type;
  stmt->vardecl.exp = exp;
  return stmt;
}

Stmt *stmt_from_vardecl(Ident id, Type *type, Expr *exp)
{
  Stmt *stmt = kmalloc(sizeof(Stmt));
  stmt->kind = VAR_KIND;
  stmt->vardecl.id = id;
  if (type != NULL)
    stmt->vardecl.type = *type;
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

Stmt *stmt_from_funcdecl(Ident id, Vector *typeparas, Vector *args,
                         Type *ret,  Vector *body)
{
  Stmt *stmt = kmalloc(sizeof(Stmt));
  stmt->kind = FUNC_KIND;
  stmt->funcdecl.id = id;
  stmt->funcdecl.typeparas = typeparas;
  stmt->funcdecl.args = args;
  if (ret != NULL)
    stmt->funcdecl.ret = *ret;
  stmt->funcdecl.body = body;
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

Stmt *stmt_from_block(Vector *list)
{
  Stmt *stmt = kmalloc(sizeof(Stmt));
  stmt->kind = BLOCK_KIND;
  stmt->block.vec = list;
  return stmt;
}

static void stmt_block_free(Vector *vec)
{
  int sz = vector_size(vec);
  Stmt *stmt;
  for (int i = 0; i < sz; ++i) {
    stmt = vector_get(vec, i);
    stmt_free(stmt);
  }
}

void stmt_free(Stmt *stmt)
{
  if (stmt == NULL)
    return;

  switch (stmt->kind) {
  case IMPORT_KIND:
    kfree(stmt);
    break;
  case CONST_KIND:
    kfree(stmt);
    break;
  case VAR_KIND:
    TYPE_DECREF(stmt->vardecl.type.desc);
    expr_free(stmt->vardecl.exp);
    kfree(stmt);
    break;
  case ASSIGN_KIND:
    expr_free(stmt->assign.lexp);
    expr_free(stmt->assign.rexp);
    kfree(stmt);
    break;
  case FUNC_KIND:
    TYPE_DECREF(stmt->funcdecl.ret.desc);
    stmt_block_free(stmt->funcdecl.body);
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
  case BLOCK_KIND:
    stmt_block_free(stmt->block.vec);
    kfree(stmt);
    break;
  default:
    panic("invalid stmt branch %d", stmt->kind);
    break;
  }
}
