/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include <inttypes.h>
#include "parser.h"
#include "memory.h"
#include "strbuf.h"

expr *expr_from_nil(void)
{
  expr *exp = kmalloc(sizeof(expr));
  exp->kind = NIL_KIND;
  return exp;
}

expr *expr_from_self(void)
{
  expr *exp = kmalloc(sizeof(expr));
  exp->kind = SELF_KIND;
  return exp;
}

expr *expr_from_super(void)
{
  expr *exp = kmalloc(sizeof(expr));
  exp->kind = SUPER_KIND;
  return exp;
}

expr *expr_from_integer(int64_t val)
{
  expr *exp = kmalloc(sizeof(expr));
  exp->kind = LITERAL_KIND;
  exp->desc = desc_from_base(BASE_INT);
  exp->k.value.kind = BASE_INT;
  exp->k.value.ival = val;
  return exp;
}

expr *expr_from_float(double val)
{
  expr *exp = kmalloc(sizeof(expr));
  exp->kind = LITERAL_KIND;
  exp->desc = desc_from_base(BASE_FLOAT);
  exp->k.value.kind = BASE_FLOAT;
  exp->k.value.fval = val;
  return exp;
}

expr *expr_from_bool(int val)
{
  expr *exp = kmalloc(sizeof(expr));
  exp->kind = LITERAL_KIND;
  exp->desc = desc_from_base(BASE_BOOL);
  exp->k.value.kind = BASE_BOOL;
  exp->k.value.bval = val;
  return exp;
}

expr *expr_from_string(char *val)
{
  expr *exp = kmalloc(sizeof(expr));
  exp->kind = LITERAL_KIND;
  exp->desc = desc_from_base(BASE_STR);
  exp->k.value.kind = BASE_STR;
  exp->k.value.str = val;
  return exp;
}

expr *expr_from_char(wchar val)
{
  expr *exp = kmalloc(sizeof(expr));
  exp->kind = LITERAL_KIND;
  exp->desc = desc_from_base(BASE_CHAR);
  exp->k.value.kind = BASE_CHAR;
  exp->k.value.cval = val;
  return exp;
}

expr *expr_from_ident(char *val)
{
  expr *exp = kmalloc(sizeof(expr));
  exp->kind = ID_KIND;
  exp->id.name = val;
  return exp;
}

expr *expr_from_unary(unaryopkind op, expr *exp)
{
  expr *uexp = kmalloc(sizeof(expr));
  uexp->kind = UNARY_KIND;
  /* it does not matter that exp->desc is null */
  uexp->desc = exp->desc;
  desc_incref(uexp->desc);
  uexp->unary.op = op;
  uexp->unary.exp = exp;
  return uexp;
}

expr *expr_from_binary(binaryopkind op, expr *left, expr *right)
{
  expr *exp = kmalloc(sizeof(expr));
  exp->kind = BINARY_KIND;
  /* it does not matter that exp->desc is null */
  exp->desc = left->desc;
  desc_incref(exp->desc);
  exp->binary.op = op;
  exp->binary.lexp = left;
  exp->binary.rexp = right;
  return exp;
}

expr *expr_from_ternary(expr *cond, expr *left, expr *right)
{
  expr *exp = kmalloc(sizeof(expr));
  exp->kind = TERNARY_KIND;
  exp->ternary.cond = cond;
  exp->ternary.lexp = left;
  exp->ternary.rexp = right;
  return exp;
}

expr *expr_from_attribute(ident id, expr *left)
{
  expr *exp = kmalloc(sizeof(expr));
  exp->kind = ATTRIBUTE_KIND;
  exp->attr.id = id;
  exp->attr.lexp = left;
  left->right = exp;
  return exp;
}

expr *expr_from_subScript(expr *left, expr *index)
{
  expr *exp = kmalloc(sizeof(expr));
  exp->kind = SUBSCRIPT_KIND;
  exp->subscr.index = index;
  exp->subscr.lexp = left;
  left->right = exp;
  return exp;
}

expr *expr_from_call(vector *args, expr *left)
{
  expr *exp = kmalloc(sizeof(expr));
  exp->kind = CALL_KIND;
  exp->call.args = args;
  exp->call.lexp = left;
  left->right = exp;
  return exp;
}

expr *expr_from_slice(expr *left, expr *start, expr *end)
{
  expr *exp = kmalloc(sizeof(expr));
  exp->kind = SLICE_KIND;
  exp->slice.start = start;
  exp->slice.end = end;
  exp->slice.lexp = left;
  left->right = exp;
  return exp;
}

expr *expr_from_tuple(vector *exps)
{
  expr *exp = kmalloc(sizeof(expr));
  exp->kind = TUPLE_KIND;
  exp->tuple = exps;
  exp->sym = find_from_builtins("Tuple");
  return exp;
}

static typedesc *get_subarray_type(vector *exps)
{
  if (exps == NULL)
    return desc_from_any;

  typedesc *desc = NULL;
  vector_iterator(iter, exps);
  expr *exp;
  iter_for_each(&iter, exp) {
    if (desc == NULL) {
      desc = exp->desc;
    } else {
      if (!desc_check(desc, exp->desc)) {
        return desc_from_any;
      }
    }
  }
  return desc_incref(desc);
}

expr *expr_from_array(vector *exps)
{
  expr *exp = kmalloc(sizeof(expr));
  exp->kind = ARRAY_KIND;
  typedesc *para = get_subarray_type(exps);
  exp->desc = desc_from_array(para);
  exp->array.elems = exps;
  exp->sym = find_from_builtins("Array");
  return exp;
}

expr *expr_from_mapentry(expr *key, expr *val)
{
  expr *exp = kmalloc(sizeof(expr));
  exp->kind = MAP_ENTRY_KIND;
  exp->mapentry.key = key;
  exp->mapentry.val = val;
  return exp;
}

expr *expr_from_map(vector *exps)
{
  expr *exp = kmalloc(sizeof(expr));
  exp->kind = MAP_KIND;
  exp->map = exps;
  exp->sym = find_from_builtins("Dict");
  return exp;
}

expr *expr_from_istype(expr *e, type type)
{
  expr *exp = kmalloc(sizeof(expr));
  exp->kind = IS_KIND;
  exp->isas.exp = e;
  exp->isas.type = type;
  return exp;
}

expr *expr_from_astype(expr *e, type type)
{
  expr *exp = kmalloc(sizeof(expr));
  exp->kind = AS_KIND;
  exp->isas.exp = e;
  exp->isas.type = type;
  return exp;
}

void exprlist_free(vector *vec)
{
  vector_iterator(iter, vec);
  expr *exp;
  iter_for_each(&iter, exp) {
    expr_free(exp);
  }
  vector_free(vec, NULL, NULL);
}

void expr_free(expr *exp)
{
  if (exp == NULL)
    return;

  desc_decref(exp->desc);

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
    desc_decref(exp->isas.type.desc);
    expr_free(exp->isas.exp);
    kfree(exp);
    break;
  default:
    panic("invalid branch %d", exp->kind);
    break;
  }
}

stmt *stmt_from_constdecl(ident id, type *type, expr *exp)
{
  stmt *s = kmalloc(sizeof(stmt));
  s->kind = CONST_KIND;
  s->vardecl.id = id;
  if (type != NULL)
    s->vardecl.type = *type;
  s->vardecl.exp = exp;
  return s;
}

stmt *stmt_from_vardecl(ident id, type *type, expr *exp)
{
  stmt *s = kmalloc(sizeof(stmt));
  s->kind = VAR_KIND;
  s->vardecl.id = id;
  if (type != NULL)
    s->vardecl.type = *type;
  s->vardecl.exp = exp;
  return s;
}

stmt *stmt_from_assign(assignopkind op, expr *left, expr *right)
{
  stmt *s = kmalloc(sizeof(stmt));
  s->kind = ASSIGN_KIND;
  s->assign.op = op;
  s->assign.lexp = left;
  s->assign.rexp = right;
  return s;
}

stmt *stmt_from_funcdecl(ident id, vector *typeparas, vector *args,
                         type *ret,  vector *body)
{
  stmt *s = kmalloc(sizeof(stmt));
  s->kind = FUNC_KIND;
  s->funcdecl.id = id;
  s->funcdecl.typeparas = typeparas;
  s->funcdecl.args = args;
  if (ret != NULL)
    s->funcdecl.ret = *ret;
  s->funcdecl.body = body;
  return s;
}

stmt *stmt_from_return(expr *exp)
{
  stmt *stmt = kmalloc(sizeof(stmt));
  stmt->kind = RETURN_KIND;
  stmt->ret.exp = exp;
  return stmt;
}

stmt *stmt_from_expr(expr *exp)
{
  stmt *s = kmalloc(sizeof(stmt));
  s->kind = EXPR_KIND;
  s->expr.exp = exp;
  return s;
}

stmt *stmt_from_block(vector *list)
{
  stmt *stmt = kmalloc(sizeof(stmt));
  stmt->kind = BLOCK_KIND;
  stmt->block.vec = list;
  return stmt;
}

static void stmt_block_free(vector *vec)
{
  int sz = vector_size(vec);
  stmt *s;
  for (int i = 0; i < sz; ++i) {
    s = vector_get(vec, i);
    stmt_free(s);
  }
}

void stmt_free(stmt *s)
{
  if (s == NULL)
    return;

  switch (s->kind) {
  case IMPORT_KIND:
    kfree(s);
    break;
  case CONST_KIND:
    kfree(s);
    break;
  case VAR_KIND:
    desc_decref(s->vardecl.type.desc);
    expr_free(s->vardecl.exp);
    kfree(s);
    break;
  case ASSIGN_KIND:
    expr_free(s->assign.lexp);
    expr_free(s->assign.rexp);
    kfree(s);
    break;
  case FUNC_KIND:
    desc_decref(s->funcdecl.ret.desc);
    stmt_block_free(s->funcdecl.body);
    kfree(s);
    break;
  case RETURN_KIND:
    expr_free(s->ret.exp);
    kfree(s);
    break;
  case EXPR_KIND:
    expr_free(s->expr.exp);
    kfree(s);
    break;
  case BLOCK_KIND:
    stmt_block_free(s->block.vec);
    kfree(s);
    break;
  default:
    panic("invalid stmt branch %d", s->kind);
    break;
  }
}
