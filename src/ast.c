/*
 MIT License

 Copyright (c) 2018 James, https://github.com/zhuguangxiang

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
*/

#include <inttypes.h>
#include "parser.h"
#include "memory.h"
#include "strbuf.h"

Expr *expr_from_null(void)
{
  Expr *exp = kmalloc(sizeof(Expr));
  exp->kind = NULL_KIND;
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

Expr *expr_from_byte(uint8_t val)
{
  Expr *exp = kmalloc(sizeof(Expr));
  exp->kind = LITERAL_KIND;
  exp->desc = desc_from_base(BASE_BYTE);
  exp->k.value.kind = BASE_BYTE;
  exp->k.value.ival = val;
  return exp;
}

Expr *expr_from_int(int64_t val)
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

Expr *expr_from_str(char *val)
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

Expr *expr_from_underscore(void)
{
  Expr *exp = kmalloc(sizeof(Expr));
  exp->kind = UNDER_KIND;
  return exp;
}

Expr *expr_from_unary(UnaryOpKind op, Expr *exp)
{
  Expr *uexp = kmalloc(sizeof(Expr));
  uexp->kind = UNARY_KIND;
  uexp->unary.op = op;
  uexp->unary.exp = exp;
  return uexp;
}

Expr *expr_from_binary(BinaryOpKind op, Expr *left, Expr *right)
{
  Expr *exp = kmalloc(sizeof(Expr));
  exp->kind = BINARY_KIND;
  exp->binary.op = op;
  exp->binary.lexp = left;
  exp->binary.rexp = right;
  return exp;
}

Expr *expr_from_ternary(Expr *test, Expr *left, Expr *right)
{
  Expr *exp = kmalloc(sizeof(Expr));
  exp->kind = TERNARY_KIND;
  exp->ternary.test = test;
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

Expr *expr_from_typeargs(Vector *types, Expr *left)
{
  Expr *exp = kmalloc(sizeof(Expr));
  exp->kind = DOT_TYPEARGS_KIND;
  exp->typeargs.types = types;
  exp->typeargs.lexp = left;
  left->right = exp;
  return exp;
}

Expr *expr_from_subscr(Expr *left, Expr *index)
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

Expr *expr_from_dottuple(int64_t index, Expr *left)
{
  Expr *exp = kmalloc(sizeof(Expr));
  exp->kind = DOT_TUPLE_KIND;
  exp->dottuple.index = index;
  exp->dottuple.lexp = left;
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

Expr *expr_from_array(Vector *exps)
{
  Expr *exp = kmalloc(sizeof(Expr));
  exp->kind = ARRAY_KIND;
  exp->array = exps;
  exp->sym = find_from_builtins("Array");
  return exp;
}

MapEntry *new_mapentry(Expr *key, Expr *val)
{
  MapEntry *entry = kmalloc(sizeof(MapEntry));
  entry->key = key;
  entry->val = val;
  return entry;
}

Expr *expr_from_map(Vector *exps)
{
  Expr *exp = kmalloc(sizeof(Expr));
  exp->kind = MAP_KIND;
  exp->map = exps;
  exp->sym = find_from_builtins("Map");
  return exp;
}

Expr *expr_from_anony(Vector *idtypes, Type *ret, Vector *body)
{
  Expr *exp = kmalloc(sizeof(Expr));
  exp->kind = ANONY_KIND;
  exp->anony.idtypes = idtypes;
  if (ret != NULL)
    exp->anony.ret = *ret;
  exp->anony.body = body;
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

Expr *expr_from_range(int type, Expr *start, Expr *end)
{
  Expr *exp = kmalloc(sizeof(Expr));
  exp->kind = RANGE_KIND;
  exp->range.type = type;
  exp->range.start = start;
  exp->range.end = end;
  return exp;
}

Expr *expr_from_binary_match(Expr *pattern, Expr *some)
{
  Expr *exp = kmalloc(sizeof(Expr));
  exp->kind = BINARY_MATCH_KIND;
  exp->binary_match.pattern = pattern;
  exp->binary_match.some = some;
  return exp;
}

void exprlist_free(Vector *vec)
{
  Expr *exp;
  vector_for_each(exp, vec) {
    expr_free(exp);
  }
  vector_free(vec);
}

void expr_free(Expr *exp)
{
  if (exp == NULL)
    return;

  TYPE_DECREF(exp->desc);

  switch (exp->kind) {
  case NULL_KIND:
  case SELF_KIND:
  case SUPER_KIND:
    kfree(exp);
    break;
  case LITERAL_KIND:
    kfree(exp);
    break;
  case ID_KIND:
    TYPE_DECREF(exp->id.etype);
    kfree(exp);
    break;
  case UNDER_KIND:
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
    expr_free(exp->ternary.test);
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
  case DOT_TYPEARGS_KIND:
    free_descs(exp->typeargs.types);
    expr_free(exp->typeargs.lexp);
    kfree(exp);
    break;
  case DOT_TUPLE_KIND:
    expr_free(exp->dottuple.lexp);
    kfree(exp);
    break;
  case TUPLE_KIND:
    exprlist_free(exp->tuple);
    kfree(exp);
    break;
  case ARRAY_KIND:
    exprlist_free(exp->array);
    kfree(exp);
    break;
  case MAP_KIND: {
    MapEntry *entry;
    vector_for_each(entry, exp->map) {
      expr_free(entry->key);
      expr_free(entry->val);
      kfree(entry);
    }
    vector_free(exp->map);
    kfree(exp);
    break;
  }
  case ANONY_KIND: {
    free_idtypes(exp->anony.idtypes);
    TYPE_DECREF(exp->anony.ret.desc);
    stmt_block_free(exp->anony.body);
    kfree(exp);
    break;
  }
  case IS_KIND:
  case AS_KIND:
    TYPE_DECREF(exp->isas.type.desc);
    expr_free(exp->isas.exp);
    kfree(exp);
    break;
  case RANGE_KIND:
    expr_free(exp->range.start);
    expr_free(exp->range.end);
    kfree(exp);
    break;
  case BINARY_MATCH_KIND:
    expr_free(exp->binary_match.pattern);
    expr_free(exp->binary_match.some);
    kfree(exp);
    break;
  default:
    panic("invalid expr kind %d", exp->kind);
    break;
  }
}

Stmt *stmt_from_import(Ident *id, char *path)
{
  Stmt *stmt = kmalloc(sizeof(Stmt));
  stmt->kind = IMPORT_KIND;
  if (id != NULL)
    stmt->import.id = *id;
  stmt->import.path = path;
  return stmt;
}

Stmt *stmt_from_import_all(char *path)
{
  Stmt *stmt = kmalloc(sizeof(Stmt));
  stmt->kind = IMPORT_KIND;
  stmt->import.type = IMPORT_ALL;
  stmt->import.path = path;
  return stmt;
}

Stmt *stmt_from_import_partial(Vector *vec, char *path)
{
  Stmt *stmt = kmalloc(sizeof(Stmt));
  stmt->kind = IMPORT_KIND;
  stmt->import.type = IMPORT_PARTIAL;
  stmt->import.aliases = vec;
  stmt->import.path = path;
  return stmt;
}

Stmt *stmt_from_native(char *path)
{
  Stmt *stmt = kmalloc(sizeof(Stmt));
  stmt->kind = NATIVE_KIND;
  stmt->native.path = path;
  return stmt;
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
  if (type != NULL) {
    stmt->vardecl.type = *type;
    if (exp != NULL) {
      Expr *lexp = NULL;
      if (exp->kind == ID_KIND) {
        lexp = exp;
      } else if (exp->kind == CALL_KIND) {
        lexp = exp->call.lexp;
        lexp = (lexp->kind == ID_KIND) ? lexp : NULL;
      }
      if (lexp != NULL) {
        lexp->id.etype = TYPE_INCREF(type->desc);
      }
    }
  }
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
  right->ctx = EXPR_LOAD;
  return stmt;
}

Stmt *stmt_from_funcdecl(Ident id, Vector *typeparas, Vector *args,
                         Type *ret,  Vector *body)
{
  Stmt *stmt = kmalloc(sizeof(Stmt));
  stmt->kind = FUNC_KIND;
  stmt->funcdecl.id = id;
  stmt->funcdecl.typeparas = typeparas;
  stmt->funcdecl.idtypes = args;
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

Stmt *stmt_from_break(short row, short col)
{
  Stmt *stmt = kmalloc(sizeof(Stmt));
  stmt->kind = BREAK_KIND;
  stmt->row = row;
  stmt->col = col;
  return stmt;
}

Stmt *stmt_from_continue(short row, short col)
{
  Stmt *stmt = kmalloc(sizeof(Stmt));
  stmt->kind = CONTINUE_KIND;
  stmt->row = row;
  stmt->col = col;
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

Stmt *stmt_from_if(Expr *test, Vector *block, Stmt *orelse)
{
  Stmt *stmt = kmalloc(sizeof(Stmt));
  stmt->kind = IF_KIND;
  stmt->if_stmt.test = test;
  stmt->if_stmt.block = block;
  stmt->if_stmt.orelse = orelse;
  return stmt;
}

Stmt *stmt_from_while(Expr *test, Vector *block)
{
  Stmt *stmt = kmalloc(sizeof(Stmt));
  stmt->kind = WHILE_KIND;
  stmt->while_stmt.test = test;
  stmt->while_stmt.block = block;
  return stmt;
}

Stmt *stmt_from_for(Expr *vexp, Expr *iter, Expr *step, Vector *block)
{
  Stmt *stmt = kmalloc(sizeof(Stmt));
  stmt->kind = FOR_KIND;
  stmt->for_stmt.vexp = vexp;
  stmt->for_stmt.iter = iter;
  stmt->for_stmt.step = step;
  stmt->for_stmt.block = block;
  return stmt;
}

void free_type_para(TypeParaDef *typepara)
{
  TypeDesc *item;
  vector_for_each(item, typepara->bounds) {
    TYPE_DECREF(item);
  }
  vector_free(typepara->bounds);
  kfree(typepara);
}

void free_type_paras(Vector *vec)
{
  TypeParaDef *item;
  vector_for_each(item, vec) {
    free_type_para(item);
  }
  vector_free(vec);
}

Stmt *stmt_from_class(Ident id, Vector *typeparas, ExtendsDef *extends,
                      Vector *body)
{
  Stmt *stmt = kmalloc(sizeof(Stmt));
  stmt->kind = CLASS_KIND;
  stmt->class_stmt.id = id;
  stmt->class_stmt.typeparas = typeparas;
  stmt->class_stmt.extends = extends;
  stmt->class_stmt.body = body;
  return stmt;
}

Stmt *stmt_from_trait(Ident id, Vector *typeparas, ExtendsDef *extends,
                      Vector *body)
{
  Stmt *stmt = kmalloc(sizeof(Stmt));
  stmt->kind = TRAIT_KIND;
  stmt->class_stmt.id = id;
  stmt->class_stmt.typeparas = typeparas;
  stmt->class_stmt.extends = extends;
  stmt->class_stmt.body = body;
  return stmt;
}

Stmt *stmt_from_ifunc(Ident id, Vector *args, Type *ret)
{
  Stmt *stmt = kmalloc(sizeof(Stmt));
  stmt->kind = IFUNC_KIND;
  stmt->funcdecl.id = id;
  stmt->funcdecl.idtypes = args;
  if (ret != NULL)
    stmt->funcdecl.ret = *ret;
  return stmt;
}

Stmt *stmt_from_enum(Ident id, Vector *typeparas, EnumMembers mbrs)
{
  Stmt *stmt = kmalloc(sizeof(Stmt));
  stmt->kind = ENUM_KIND;
  stmt->enum_stmt.id = id;
  stmt->enum_stmt.typeparas = typeparas;
  stmt->enum_stmt.mbrs = mbrs;
  return stmt;
}

Stmt *stmt_from_match(Expr *exp, Vector *clauses)
{
  Stmt *stmt = kmalloc(sizeof(Stmt));
  stmt->kind = MATCH_KIND;
  stmt->match_stmt.exp = exp;
  stmt->match_stmt.clauses = clauses;
  return stmt;
}

void stmt_block_free(Vector *vec)
{
  if (vec == NULL)
    return;

  Stmt *stmt;
  vector_for_each(stmt, vec) {
    stmt_free(stmt);
  }
  vector_free(vec);
}

void free_idtypes(Vector *vec)
{
  IdType *item;
  vector_for_each(item, vec) {
    free_idtype(item);
  }
  vector_free(vec);
}

static inline void free_match_clauses(Vector *vec)
{
  if (vec == NULL)
    return;

  MatchClause *match;
  vector_for_each(match, vec) {
    if (match != NULL) {
      exprlist_free(match->patts);
      stmt_free(match->block);
      kfree(match);
    }
  }
  vector_free(vec);
}

void stmt_free(Stmt *stmt)
{
  if (stmt == NULL)
    return;

  TYPE_DECREF(stmt->desc);

  switch (stmt->kind) {
  case IMPORT_KIND:
    free_aliases(stmt->import.aliases);
    kfree(stmt);
    break;
  case NATIVE_KIND:
    kfree(stmt);
    break;
  case CONST_KIND:
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
    free_idtypes(stmt->funcdecl.idtypes);
    TYPE_DECREF(stmt->funcdecl.ret.desc);
    stmt_block_free(stmt->funcdecl.body);
    kfree(stmt);
    break;
  case BREAK_KIND:
  case CONTINUE_KIND:
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
  case IF_KIND:
    expr_free(stmt->if_stmt.test);
    stmt_block_free(stmt->if_stmt.block);
    stmt_free(stmt->if_stmt.orelse);
    kfree(stmt);
    break;
  case WHILE_KIND:
    expr_free(stmt->while_stmt.test);
    stmt_block_free(stmt->while_stmt.block);
    kfree(stmt);
    break;
  case FOR_KIND:
    expr_free(stmt->for_stmt.vexp);
    expr_free(stmt->for_stmt.iter);
    expr_free(stmt->for_stmt.step);
    stmt_block_free(stmt->for_stmt.block);
    kfree(stmt);
    break;
  case CLASS_KIND:
  case TRAIT_KIND:
    free_extends(stmt->class_stmt.extends);
    free_type_paras(stmt->class_stmt.typeparas);
    stmt_block_free(stmt->class_stmt.body);
    kfree(stmt);
    break;
  case ENUM_KIND:
    free_labels(stmt->enum_stmt.mbrs.labels);
    stmt_block_free(stmt->enum_stmt.mbrs.methods);
    kfree(stmt);
    break;
  case IFUNC_KIND:
    free_idtypes(stmt->funcdecl.idtypes);
    TYPE_DECREF(stmt->funcdecl.ret.desc);
    kfree(stmt);
    break;
  case MATCH_KIND:
    expr_free(stmt->match_stmt.exp);
    free_match_clauses(stmt->match_stmt.clauses);
    kfree(stmt);
    break;
  default:
    panic("invalid stmt kind %d", stmt->kind);
    break;
  }
}
