/*
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "parser.h"
#include "hashfunc.h"
#include "mem.h"
#include "log.h"
#include "stringbuf.h"

LOGGER(0)

Ident *New_Ident(String name)
{
  Ident *ident = Malloc(sizeof(Ident));
  ident->name = name.str;
  return ident;
}

void Free_Ident(Ident *id)
{
  if (id == NULL)
    return;

  Mfree(id);
}

void Free_IdentList(Vector *vec)
{
  if (vec == NULL)
    return;

  Ident *id;
  Vector_ForEach(id, vec) {
    Free_Ident(id);
  }
  Vector_Free_Self(vec);
}

IdType *New_IdType(Ident *id, TypeWrapper *type)
{
  IdType *idType = Malloc(sizeof(IdType));
  if (id != NULL)
    idType->id = *id;
  TYPE_INCREF(type->desc);
  idType->type = *type;
  return idType;
}

void Free_IdType(IdType *idtype)
{
  if (idtype == NULL)
    return;

  TYPE_DECREF(idtype->type.desc);
  Mfree(idtype);
}

void Free_IdTypeList(Vector *vec)
{
  if (vec == NULL)
    return;

  IdType *idtype;
  Vector_ForEach(idtype, vec) {
    Free_IdType(idtype);
  }
  Vector_Free_Self(vec);
}

Expr *Expr_From_Nil(void)
{
  Expr *exp = Malloc(sizeof(Expr));
  exp->kind = NIL_KIND;
  return exp;
}

Expr *Expr_From_Self(void)
{
  Expr *exp = Malloc(sizeof(Expr));
  exp->kind = SELF_KIND;
  return exp;
}

Expr *Expr_From_Super(void)
{
  Expr *exp = Malloc(sizeof(Expr));
  exp->kind = SUPER_KIND;
  return exp;
}

Expr *Expr_From_Integer(int64 val)
{
  ConstExpr *constExp = Malloc(sizeof(ConstExpr));
  constExp->kind = CONST_KIND;
  constExp->desc = TypeDesc_Get_Base(BASE_INT);
  TYPE_INCREF(constExp->desc);
  constExp->value.kind = BASE_INT;
  constExp->value.ival = val;
  return (Expr *)constExp;
}

Expr *Expr_From_Float(float64 val)
{
  ConstExpr *constExp = Malloc(sizeof(ConstExpr));
  constExp->kind = CONST_KIND;
  constExp->desc = TypeDesc_Get_Base(BASE_FLOAT);
  TYPE_INCREF(constExp->desc);
  constExp->value.kind = BASE_FLOAT;
  constExp->value.fval = val;
  return (Expr *)constExp;
}

Expr *Expr_From_Bool(int val)
{
  ConstExpr *constExp = Malloc(sizeof(ConstExpr));
  constExp->kind = CONST_KIND;
  constExp->desc = TypeDesc_Get_Base(BASE_BOOL);
  TYPE_INCREF(constExp->desc);
  constExp->value.kind = BASE_BOOL;
  constExp->value.bval = val;
  return (Expr *)constExp;
}

Expr *Expr_From_String(char *val)
{
  ConstExpr *constExp = Malloc(sizeof(ConstExpr));
  constExp->kind = CONST_KIND;
  constExp->desc = TypeDesc_Get_Base(BASE_STRING);
  TYPE_INCREF(constExp->desc);
  constExp->value.kind = BASE_STRING;
  constExp->value.str = val;
  return (Expr *)constExp;
}

Expr *Expr_From_Char(uchar val)
{
  ConstExpr *constExp = Malloc(sizeof(ConstExpr));
  constExp->kind = CONST_KIND;
  constExp->desc = TypeDesc_Get_Base(BASE_CHAR);
  TYPE_INCREF(constExp->desc);
  constExp->value.kind = BASE_CHAR;
  constExp->value.ch = val;
  return (Expr *)constExp;
}

Expr *Expr_From_Ident(char *val)
{
  IdentExpr *idExp = Malloc(sizeof(IdentExpr));
  idExp->kind = ID_KIND;
  idExp->name = val;
  return (Expr *)idExp;
}

/* FIXME: unchanged variable, see parse_operator.c */
int Expr_Is_Const(Expr *exp)
{
  if (exp->kind == CONST_KIND)
    return 1;

  if (exp->kind == ID_KIND) {
    Symbol *sym = exp->sym;
    if (sym != NULL && sym->kind == SYM_CONST)
      return 1;
  }

  if (exp->kind == UNARY_KIND) {
    UnaryExpr *unExpr = (UnaryExpr *)exp;
    if (unExpr->val.kind != 0)
      return 1;
  }

  if (exp->kind == BINARY_KIND) {
    BinaryExpr *biExpr = (BinaryExpr *)exp;
    if (biExpr->val.kind != 0)
      return 1;
  }

  return 0;
}

Expr *Expr_From_Unary(UnaryOpKind op, Expr *exp)
{
  UnaryExpr *unaryExp = Malloc(sizeof(UnaryExpr));
  unaryExp->kind = UNARY_KIND;
  /* it does not matter that exp->desc is null */
  unaryExp->desc = exp->desc;
  TYPE_INCREF(unaryExp->desc);
  unaryExp->op = op;
  unaryExp->exp = exp;
  return (Expr *)unaryExp;
}

Expr *Expr_From_Binary(BinaryOpKind op, Expr *left, Expr *right)
{
  BinaryExpr *binaryExp = Malloc(sizeof(BinaryExpr));
  binaryExp->kind = BINARY_KIND;
  /* it does not matter that exp->desc is null */
  binaryExp->desc = left->desc;
  TYPE_INCREF(binaryExp->desc);
  binaryExp->op = op;
  binaryExp->lexp = left;
  binaryExp->rexp = right;
  return (Expr *)binaryExp;
}

Expr *Expr_From_Attribute(Ident id, Expr *left)
{
  AttributeExpr *attrExp = Malloc(sizeof(AttributeExpr));
  attrExp->kind = ATTRIBUTE_KIND;
  attrExp->id = id;
  attrExp->left = left;
  left->right = (Expr *)attrExp;
  return (Expr *)attrExp;
}

Expr *Expr_From_SubScript(Expr *index, Expr *left)
{
  SubScriptExpr *subExp = Malloc(sizeof(SubScriptExpr));
  subExp->kind = SUBSCRIPT_KIND;
  subExp->index = index;
  subExp->left = left;
  left->right = (Expr *)subExp;
  return (Expr *)subExp;
}

Expr *Expr_From_Call(Vector *args, Expr *left)
{
  CallExpr *callExp = Malloc(sizeof(CallExpr));
  callExp->kind = CALL_KIND;
  callExp->args = args;
  callExp->left = left;
  left->right = (Expr *)callExp;
  return (Expr *)callExp;
}

Expr *Expr_From_Slice(Expr *start, Expr *end, Expr *left)
{
  SliceExpr *sliceExp = Malloc(sizeof(SliceExpr));
  sliceExp->kind = SLICE_KIND;
  sliceExp->start = start;
  sliceExp->end = end;
  sliceExp->left = left;
  left->right = (Expr *)sliceExp;
  return (Expr *)sliceExp;
}

static int arraylist_get_nesting(Vector *vec)
{
  int max = 0;
  ListExpr *listExp;
  Expr *e;
  Vector_ForEach(e, vec) {
    if (e->kind == ARRAY_LIST_KIND) {
      listExp = (ListExpr *)e;
      if (max < listExp->nesting)
        max = listExp->nesting;
    }
  }
  return max;
}

Expr *Expr_From_ArrayListExpr(Vector *vec)
{
  int nesting = arraylist_get_nesting(vec) + 1;
  ListExpr *listExp = Malloc(sizeof(ListExpr));
  listExp->kind = ARRAY_LIST_KIND;
  listExp->nesting = nesting;
  listExp->vec = vec;
  return (Expr *)listExp;
}

Expr *Expr_From_Array(Vector *dims, TypeWrapper base, Expr *listExp)
{
  ArrayExpr *arrayExp = Malloc(sizeof(ArrayExpr));
  arrayExp->kind = ARRAY_KIND;
  arrayExp->dims = dims;
  arrayExp->base = base;
  assert(listExp != NULL ? listExp->kind == ARRAY_LIST_KIND : 1);
  arrayExp->listExp = (ListExpr *)listExp;
  return (Expr *)arrayExp;
}

Expr *Parser_New_Array(Vector *vec, int dims, TypeWrapper type, Expr *listExp)
{
  Vector *dimsVec;
  TypeDesc *base;
  if (vec != NULL) {
    assert(dims == 0);
    dimsVec = vec;
    if (type.desc->kind == TYPE_ARRAY) {
      ArrayDesc *arrayDesc = (ArrayDesc *)type.desc;
      base = arrayDesc->base;
      /* append null to occupy a position */
      for (int i = 0; i < arrayDesc->dims; i++)
        Vector_Append(dimsVec, NULL);
      /* free array desc */
      TYPE_INCREF(base);
      TYPE_DECREF(type.desc);
    } else {
      base = type.desc;
      TYPE_INCREF(base);
    }
  } else {
    assert(dims != 0);
    dimsVec = Vector_New();
    /* append null to occupy a position */
    for (int i = 0; i < dims; i++)
      Vector_Append(dimsVec, NULL);
    assert(type.desc->kind != TYPE_ARRAY);
    base = type.desc;
    TYPE_INCREF(base);
  }
  TypeWrapper basetype = {base, type.pos};
  return Expr_From_Array(dimsVec, basetype, listExp);
}

static int maplist_get_nesting(Vector *vec)
{
  int max = 0;
  ListExpr *listExp;
  Expr *v;
  MapEntryExpr *e;
  Vector_ForEach(e, vec) {
    assert(e->kind == MAP_ENTRY_KIND);
    v = e->val;
    assert(v != NULL);
    if (v->kind == MAP_LIST_KIND) {
      listExp = (ListExpr *)v;
      if (max < listExp->nesting)
        max = listExp->nesting;
    }
  }
  return max;
}

Expr *Expr_From_MapListExpr(Vector *vec)
{
  int nesting = maplist_get_nesting(vec) + 1;
  ListExpr *listExp = Malloc(sizeof(ListExpr));
  listExp->kind = MAP_LIST_KIND;
  listExp->nesting = nesting;
  listExp->vec = vec;
  return (Expr *)listExp;
}

Expr *Expr_From_MapEntry(Expr *k, Expr *v)
{
  MapEntryExpr *entExp = Malloc(sizeof(MapEntryExpr));
  entExp->kind = MAP_ENTRY_KIND;
  entExp->key = k;
  entExp->val = v;
  return (Expr *)entExp;
}

Expr *Expr_From_Map(TypeWrapper type, Expr *listExp)
{
  MapExpr *mapExp = Malloc(sizeof(MapExpr));
  mapExp->kind = MAP_KIND;
  TYPE_INCREF(type.desc);
  mapExp->type = type;
  assert(listExp != NULL ? listExp->kind == MAP_LIST_KIND : 1);
  mapExp->listExp = (ListExpr *)listExp;
  return (Expr *)mapExp;
}

Expr *Expr_From_Set(TypeWrapper type, Expr *listExp)
{
  SetExpr *setExp = Malloc(sizeof(SetExpr));
  setExp->kind = SET_KIND;
  TYPE_INCREF(type.desc);
  setExp->type = type;
  assert(listExp != NULL ? listExp->kind == ARRAY_LIST_KIND : 1);
  setExp->listExp = (ListExpr *)listExp;
  return (Expr *)setExp;
}

Expr *Expr_From_Anony(Vector *args, Vector *rets, Vector *body)
{
  AnonyExpr *anonyExp = Malloc(sizeof(AnonyExpr));
  anonyExp->kind = ANONY_FUNC_KIND;
  anonyExp->args = args;
  anonyExp->rets = rets;
  anonyExp->body = body;
  return (Expr *)anonyExp;
}

static inline void free_exprlist(Vector *vec);

int Expr_Maybe_Stored(Expr *exp)
{
  if (exp->kind == ID_KIND || exp->kind == ATTRIBUTE_KIND ||
      exp->kind == SUBSCRIPT_KIND)
    return 1;
  else
    return 0;
}

static void free_expr(Expr *exp)
{
  TYPE_DECREF(exp->desc);
  Mfree(exp);
}

static void free_unary_expr(Expr *exp)
{
  UnaryExpr *unExp = (UnaryExpr *)exp;
  Free_Expr(unExp->exp);
  free_expr(exp);
}

static void free_binary_expr(Expr *exp)
{
  BinaryExpr *binExp = (BinaryExpr *)exp;
  Free_Expr(binExp->lexp);
  Free_Expr(binExp->rexp);
  free_expr(exp);
}

static void free_attribute_expr(Expr *exp)
{
  AttributeExpr *attrExp = (AttributeExpr *)exp;
  Free_Expr(attrExp->left);
  free_expr(exp);
}

static void free_subscript_expr(Expr *exp)
{
  SubScriptExpr *subExp = (SubScriptExpr *)exp;
  Free_Expr(subExp->index);
  Free_Expr(subExp->left);
  free_expr(exp);
}

static void free_call_expr(Expr *exp)
{
  CallExpr *callExp = (CallExpr *)exp;
  free_exprlist(callExp->args);
  Free_Expr(callExp->left);
  free_expr(exp);
}

static void free_slice_expr(Expr *exp)
{
  SliceExpr *sliceExp = (SliceExpr *)exp;
  Free_Expr(sliceExp->start);
  Free_Expr(sliceExp->end);
  Free_Expr(sliceExp->left);
  free_expr(exp);
}

static void free_list_expr(Expr *exp)
{
  if (exp == NULL)
    return;
  ListExpr *listExp = (ListExpr *)exp;
  free_exprlist(listExp->vec);
  free_expr(exp);
}

static void free_mapentry_expr(Expr *exp)
{
  MapEntryExpr *entExp = (MapEntryExpr *)exp;
  Free_Expr(entExp->key);
  Free_Expr(entExp->val);
  free_expr(exp);
}

static void free_array_expr(Expr *exp)
{
  ArrayExpr *arrayExp = (ArrayExpr *)exp;
  free_exprlist(arrayExp->dims);
  TYPE_DECREF(arrayExp->base.desc);
  free_list_expr((Expr *)arrayExp->listExp);
  free_expr(exp);
}

static void free_map_expr(Expr *exp)
{
  MapExpr *mapExp = (MapExpr *)exp;
  TYPE_DECREF(mapExp->type.desc);
  free_list_expr((Expr *)mapExp->listExp);
  free_expr(exp);
}

static void free_set_expr(Expr *exp)
{
  SetExpr *setExp = (SetExpr *)exp;
  TYPE_DECREF(setExp->type.desc);
  free_list_expr((Expr *)setExp->listExp);
  free_expr(exp);
}

static void free_anony_expr(Expr *exp)
{
  AnonyExpr *anonyExp = (AnonyExpr *)exp;
  Free_IdTypeList(anonyExp->args);
  Free_IdTypeList(anonyExp->rets);
  Vector_Free(anonyExp->body, Free_Stmt_Func, NULL);
  free_expr(exp);
}

static void (*__free_expr_funcs[])(Expr *) = {
  NULL,                 /* INVALID          */
  free_expr,            /* NIL_KIND         */
  free_expr,            /* SELF_KIND        */
  free_expr,            /* SUPER_KIND       */
  free_expr,            /* CONST_KIND       */
  free_expr,            /* ID_KIND          */
  free_unary_expr,      /* UNARY_KIND       */
  free_binary_expr,     /* BINARY_KIND      */
  free_attribute_expr,  /* ATTRIBUTE_KIND   */
  free_subscript_expr,  /* SUBSCRIPT_KIND   */
  free_call_expr,       /* CALL_KIND        */
  free_slice_expr,      /* SLICE_KIND       */
  free_list_expr,       /* ARRAY_LIST_KIND  */
  free_list_expr,       /* MAP_LIST_KIND    */
  free_mapentry_expr,   /* MAP_ENTRY_KIND   */
  free_array_expr,      /* ARRAY_KIND       */
  free_map_expr,        /* MAP_KIND         */
  free_set_expr,        /* SET_KIND         */
  free_anony_expr,      /* ANONY_FUNC_KIND  */
};

void Free_Expr(Expr *exp)
{
  if (exp == NULL)
    return;

  assert(exp->kind >= 1 && exp->kind < nr_elts(__free_expr_funcs));
  void (*__free_expr_func)(Expr *) = __free_expr_funcs[exp->kind];
  __free_expr_func(exp);
}

static void free_expr_func(void *item, void *arg)
{
  UNUSED_PARAMETER(arg);
  Free_Expr(item);
}

static inline void free_exprlist(Vector *vec)
{
  Vector_Free(vec, free_expr_func, NULL);
}

static void free_vardecl_stmt(Stmt *stmt)
{
  VarDeclStmt *varStmt = (VarDeclStmt *)stmt;
  TYPE_DECREF(varStmt->type.desc);
  Free_Expr(varStmt->exp);
  Mfree(stmt);
}

static void free_varlistdecl_stmt(Stmt *stmt)
{
  VarListDeclStmt *varListStmt = (VarListDeclStmt *)stmt;
  Free_IdentList(varListStmt->ids);
  TYPE_DECREF(varListStmt->type.desc);
  Free_Expr(varListStmt->exp);
  Mfree(stmt);
}

static void free_assign_stmt(Stmt *stmt)
{
  AssignStmt *assignStmt = (AssignStmt *)stmt;
  Free_Expr(assignStmt->left);
  Free_Expr(assignStmt->right);
  Mfree(stmt);
}

static void free_assignlist_stmt(Stmt *stmt)
{
  AssignListStmt *assListStmt = (AssignListStmt *)stmt;
  free_exprlist(assListStmt->left);
  Free_Expr(assListStmt->right);
  Mfree(stmt);
}

static void free_funcdecl_stmt(Stmt *stmt)
{
  FuncDeclStmt *funcStmt = (FuncDeclStmt *)stmt;
  Free_IdTypeList(funcStmt->args);
  Free_IdTypeList(funcStmt->rets);
  Vector_Free(funcStmt->body, Free_Stmt_Func, NULL);
  Mfree(stmt);
}

static void free_expr_stmt(Stmt *stmt)
{
  ExprStmt *expStmt = (ExprStmt *)stmt;
  Free_Expr(expStmt->exp);
  Mfree(stmt);
}

static void free_return_stmt(Stmt *stmt)
{
  ReturnStmt *retStmt = (ReturnStmt *)stmt;
  free_exprlist(retStmt->exps);
  Mfree(stmt);
}

static void free_list_stmt(Stmt *stmt)
{
  ListStmt *listStmt = (ListStmt *)stmt;
  Vector_Free(listStmt->vec, Free_Stmt_Func, NULL);
  Mfree(stmt);
}

static void free_alias_stmt(Stmt *stmt)
{
  TypeAliasStmt *aliasStmt = (TypeAliasStmt *)stmt;
  TYPE_DECREF(aliasStmt->desc);
  Mfree(stmt);
}

static void free_typedesc_func(void *item, void *arg)
{
  TYPE_DECREF(item);
}

static void free_klass_stmt(Stmt *stmt)
{
  KlassStmt *klsStmt = (KlassStmt *)stmt;
  Vector_Free(klsStmt->super, free_typedesc_func, NULL);
  Vector_Free_Self(klsStmt->body);
  Mfree(stmt);
}

static void (*__free_stmt_funcs[])(Stmt *) = {
  NULL,                     /* INVALID         */
  free_vardecl_stmt,        /* VAR_KIND        */
  free_varlistdecl_stmt,    /* VARLIST_KIND    */
  free_assign_stmt,         /* ASSIGN_KIND     */
  free_assignlist_stmt,     /* ASSIGNLIST_KIND */
  free_funcdecl_stmt,       /* FUNC_KIND       */
  free_funcdecl_stmt,       /* PROTO_KIND      */
  free_expr_stmt,           /* EXPR_KIND       */
  free_return_stmt,         /* RETURN_KIND     */
  free_list_stmt,           /* LIST_KIND       */
  free_alias_stmt,          /* TYPEALIAS_KIND  */
  free_klass_stmt,          /* CLASS_KIND      */
  free_klass_stmt,          /* TRAIT_KIND      */
  NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL,
};

void Free_Stmt_Func(void *item, void *arg)
{
  Stmt *stmt = item;
  assert(stmt->kind >= 1 && stmt->kind < nr_elts(__free_stmt_funcs));
  void (*__free_stmt_func)(Stmt *) = __free_stmt_funcs[stmt->kind];
  __free_stmt_func(stmt);
}

Stmt *__Stmt_From_VarDecl(Ident *id, TypeWrapper type, Expr *exp, int konst)
{
  VarDeclStmt *varStmt = Malloc(sizeof(VarDeclStmt));
  varStmt->kind = VAR_KIND;
  varStmt->id = *id;
  varStmt->type = type;
  varStmt->exp = exp;
  varStmt->konst = konst;
  return (Stmt *)varStmt;
}

Stmt *__Stmt_From_VarListDecl(Vector *ids, TypeWrapper type,
                              Expr *exp, int konst)
{
  VarListDeclStmt *varListStmt = Malloc(sizeof(VarListDeclStmt));
  varListStmt->kind = VARLIST_KIND;
  varListStmt->ids = ids;
  varListStmt->type = type;
  varListStmt->exp = exp;
  varListStmt->konst = konst;
  return (Stmt *)varListStmt;
}

Stmt *Stmt_From_Assign(AssignOpKind op, Expr *left, Expr *right)
{
  AssignStmt *assignStmt = Malloc(sizeof(AssignStmt));
  assignStmt->kind = ASSIGN_KIND;
  assignStmt->op = op;
  assignStmt->left = left;
  assignStmt->right = right;
  return (Stmt *)assignStmt;
}

Stmt *Stmt_From_AssignList(Vector *left, Expr *right)
{
  AssignListStmt *assignListStmt = Malloc(sizeof(AssignListStmt));
  assignListStmt->kind = ASSIGNLIST_KIND;
  assignListStmt->left = left;
  assignListStmt->right = right;
  return (Stmt *)assignListStmt;
}

Stmt *Stmt_From_FuncDecl(Ident id, Vector *args, Vector *rets, Vector *stmts)
{
  FuncDeclStmt *funcStmt = Malloc(sizeof(FuncDeclStmt));
  funcStmt->kind = FUNC_KIND;
  funcStmt->id = id;
  funcStmt->args = args;
  funcStmt->rets = rets;
  funcStmt->body = stmts;
  return (Stmt *)funcStmt;
}

Stmt *Stmt_From_ProtoDecl(Ident id, Vector *args, Vector *rets)
{
  FuncDeclStmt *protoStmt = Malloc(sizeof(FuncDeclStmt));
  protoStmt->kind = PROTO_KIND;
  protoStmt->id = id;
  protoStmt->args = args;
  protoStmt->rets = rets;
  return (Stmt *)protoStmt;
}

Stmt *Stmt_From_Expr(Expr *exp)
{
  ExprStmt *expStmt = Malloc(sizeof(ExprStmt));
  expStmt->kind = EXPR_KIND;
  expStmt->exp = exp;
  return (Stmt *)expStmt;
}

Stmt *Stmt_From_Return(Vector *exps)
{
  ReturnStmt *retStmt = Malloc(sizeof(ReturnStmt));
  retStmt->kind = RETURN_KIND;
  retStmt->exps = exps;
  return (Stmt *)retStmt;
}

Stmt *Stmt_From_List(Vector *vec)
{
  ListStmt *listStmt = Malloc(sizeof(ListStmt));
  listStmt->kind = LIST_KIND;
  listStmt->vec = vec;
  return (Stmt *)listStmt;
}

Stmt *Stmt_From_TypeAlias(Ident id, TypeDesc *desc)
{
  TypeAliasStmt *aliasStmt = Malloc(sizeof(TypeAliasStmt));
  aliasStmt->kind = TYPEALIAS_KIND;
  aliasStmt->id = id;
  aliasStmt->desc = desc;
  return (Stmt *)aliasStmt;
}

Stmt *Stmt_From_Klass(Ident id, StmtKind kind, Vector *super, Vector *body)
{
  assert(kind == CLASS_KIND || kind == TRAIT_KIND);
  KlassStmt *klsStmt = Malloc(sizeof(KlassStmt));
  klsStmt->kind = kind;
  klsStmt->id = id;
  klsStmt->super = super;
  klsStmt->body = body;
  return (Stmt *)klsStmt;
}

static int file_exist(char *path)
{
  struct stat attr;
  if (stat(path, &attr) == - 1)
    return 0;

  if (!S_ISREG(attr.st_mode))
    return 0;

  return 1;
}

static int dir_exist(char *path)
{
  struct stat attr;
  if (stat(path, &attr) == - 1)
    return 0;

  if (!S_ISDIR(attr.st_mode))
    return 0;

  return 1;
}

static int dir_later_file(char *dirpath, char *filepath)
{
  char *path;
  time_t mtime = {0};
  struct stat attr;
  DIR *dir = opendir(dirpath);
  struct dirent *dent;
  while ((dent = readdir(dir))) {
    if (!strcmp(dent->d_name, ".") ||
        !strcmp(dent->d_name, ".."))
      continue;
      path = AtomString_Format("#/#", dirpath, dent->d_name);
    if (stat(path, &attr) == -1)
      continue;
    Log_Printf("%s:  %s", path, ctime(&attr.st_mtime));
    if (difftime(mtime, attr.st_mtime) < 0)
      mtime = attr.st_mtime;
  }
  closedir(dir);

  if (stat(filepath, &attr) != -1) {
    Log_Printf("%s:  %s", filepath, ctime(&attr.st_mtime));
    return difftime(mtime, attr.st_mtime);
  }

  return 1;
}

static inline PkgSymbol *__in_extstbl(ParserState *ps, char *name)
{
  if (ps->extstbl == NULL)
    return NULL;
  return (PkgSymbol *)STable_Get(ps->extstbl, name);
}

static inline RefSymbol *__is_inextdots(ParserState *ps, char *name)
{
  if (ps->extdots == NULL)
    return NULL;
  return (RefSymbol *)STable_Get(ps->extdots, name);
}

struct load_dotsym_param {
  ParserState *ps;
  char *path;
  Position pos;
};

static void load_dotsym_func(Symbol *sym, void *arg)
{
  struct load_dotsym_param *param = arg;
  ParserState *ps = param->ps;

  if (ps->errnum > MAX_ERRORS)
    return;

  PkgSymbol *pkgSym = __in_extstbl(ps, sym->name);
  if (pkgSym != NULL) {
    Syntax_Error(ps, &param->pos, "'%s' redeclared during import '%s',\n"
                 "\tprevious declaration at %s:%d:%d", sym->name, param->path,
                 pkgSym->filename, pkgSym->pos.row, pkgSym->pos.col);
    return;
  }

  RefSymbol *dot = STable_Add_Reference(ps->extdots, sym->name);
  if (dot == NULL) {
    dot = (RefSymbol *)STable_Get(ps->extdots, sym->name);
    Syntax_Error(ps, &param->pos, "'%s' redeclared during import '%s',\n"
                 "\tprevious declaration during import '%s' at %s:%d:%d",
                 sym->name, param->path, dot->path, dot->filename,
                 dot->pos.row, dot->pos.col);
  } else {
    dot->sym = sym;
    dot->path = param->path;
    dot->filename = ps->filename;
    dot->pos = param->pos;
  }
}

static Import *new_import(Ident *id, Ident *path)
{
  Import *import = Malloc(sizeof(Import));
  if (id != NULL) {
    import->id = id->name;
    import->idpos = id->pos;
  }
  import->path = path->name;
  import->pathpos = path->pos;
  return import;
}

static inline void free_import(Import *import)
{
  Mfree(import);
}

void Parse_Imports(ParserState *ps)
{
  char *name;
  Symbol *sym;
  Import *import;
  Vector_ForEach(import, &ps->imports) {
    Package *pkg = Find_Package(import->path);
    assert(pkg != NULL);

    if (import->id == NULL) {
      /* use package's name as name */
      name = pkg->pkgname;
    } else {
      /* use id as name */
      name = import->id;
    }

    if (name[0] != '.') {
      RefSymbol *dot = __is_inextdots(ps, name);
      if (dot != NULL) {
        Syntax_Error(ps, &import->pathpos,
                     "'%s' redeclared as imported package name,\n"
                     "\tprevious declaration during import '%s' at %s:%d:%d",
                     name, dot->path,
                     dot->filename, dot->pos.row, dot->pos.col);
      } else {
        PkgSymbol *sym = STable_Add_Package(ps->extstbl, name);
        if (sym != NULL) {
          Log_Debug("add package '%s <- %s' successfully", name, import->path);
          sym->pkg = pkg;
          sym->filename = ps->filename;
          sym->pos = (import->id == NULL) ? import->pathpos : import->idpos;
        } else {
          sym = (PkgSymbol *)STable_Get(ps->extstbl, name);
          Syntax_Error(ps, &import->pathpos,
                       "'%s' redeclared as imported package name,\n"
                       "\tprevious declaration at %s:%d:%d", name,
                       sym->filename, sym->pos.row, sym->pos.col);
        }
      }
    } else {
      /* load all symbols to current module */
      struct load_dotsym_param param = {ps, import->path, import->pathpos};
      STable_Visit(pkg->stbl, load_dotsym_func, &param);
    }

    free_import(import);
  }
}

void Parser_New_Import(ParserState *ps, Ident *id, Ident *path)
{
  if (!strcmp(path->name, ps->grp->pkg.path)) {
    Syntax_Error(ps, &path->pos, "imported self-package");
    return;
  }

  Import *import;
  Vector_ForEach(import, &ps->imports) {
    if (!strcmp(import->path, path->name)) {
      Syntax_Error(ps, &path->pos, "'%s' imported duplicately,\n"
                   "\tprevious import at %s:%d:%d",
                   path->name, ps->filename,
                   import->pathpos.row, import->pathpos.col);
      return;
    }

    if ((id != NULL && id->name[0] != '.') && (import->id != NULL)) {
      if (!strcmp(import->id, id->name)) {
        Syntax_Error(ps, &id->pos, "'%s' redeclared as imported package name,\n"
                     "\tprevious declaration during import at %s:%d:%d",
                     id->name, ps->filename,
                     import->idpos.row, import->idpos.col);
        return;
      }
    }
  }

  Package *pkg = Find_Package(path->name);
  if (pkg == NULL) {
    /* find source path */
    Options *opts = &options;
    char *dir = path->name;
    if (opts->srcpath != NULL)
      dir = AtomString_Format("#/#", opts->srcpath, dir);

    if (dir_exist(dir)) {
      /* find .klc in source path */
      char *klc = AtomString_Format("#.klc", dir);

      if (!file_exist(klc)) {
        Log_Debug("compile package '%s' for it's not exist", dir);
        Add_ParserGroup(path->name);
      } else {
        if (dir_later_file(dir, klc) > 0) {
          Log_Debug("compile package '%s' for it's later", dir);
          Add_ParserGroup(path->name);
        } else {
          /* load image */
          Log_Debug("load package '%s'", dir);
          pkg = New_Package(path->name);
          STable_From_Image(klc, &pkg->pkgname, &pkg->stbl);
        }
      }
    }
  }

  Vector_Append(&ps->imports, new_import(id, path));
}

static inline void __add_stmt(ParserState *ps, Stmt *stmt)
{
  Vector_Append(&ps->stmts, stmt);
}

static void __new_var(ParserState *ps, Ident *id, TypeDesc *desc, int k)
{
  ParserUnit *u = ps->u;
  Symbol *sym;

  sym = (Symbol *)__in_extstbl(ps, id->name);
  if (sym != NULL) {
    Syntax_Error(ps, &id->pos, "'%s' redeclared,\n"
                 "\tprevious declaration at %s:%d:%d", id->name,
                 sym->filename, sym->pos.row, sym->pos.col);
    return;
  }

  if (k)
    sym = STable_Add_Const(u->stbl, id->name, desc);
  else
    sym = STable_Add_Var(u->stbl, id->name, desc);

  if (sym != NULL) {
    Log_Debug("add %s '%s' successfully", k ? "const" : "var", id->name);
    sym->filename = ps->filename;
    sym->pos = id->pos;
  } else {
    sym = STable_Get(u->stbl, id->name);
    Syntax_Error(ps, &id->pos, "'%s' redeclared,\n"
                 "\tprevious declaration at %s:%d:%d", id->name,
                 sym->filename, sym->pos.row, sym->pos.col);
  }
}

void Parser_New_Variables(ParserState *ps, Stmt *stmt)
{
  if (stmt == NULL)
    return;

  if (stmt->kind == VAR_KIND) {
    VarDeclStmt *varStmt = (VarDeclStmt *)stmt;
    __add_stmt(ps, stmt);
    __new_var(ps, &varStmt->id, varStmt->type.desc, varStmt->konst);
  } else if (stmt->kind == LIST_KIND) {
    ListStmt *listStmt = (ListStmt *)stmt;
    Stmt *s;
    VarDeclStmt *varStmt;
    Vector_ForEach(s, listStmt->vec) {
      assert(s->kind == VAR_KIND);
      __add_stmt(ps, s);
      varStmt = (VarDeclStmt *)s;
      __new_var(ps, &varStmt->id, varStmt->type.desc, varStmt->konst);
    }
    Vector_Free_Self(listStmt->vec);
    Mfree(listStmt);
  } else {
    assert(stmt->kind == VARLIST_KIND);
    __add_stmt(ps, stmt);
    VarListDeclStmt *varsStmt = (VarListDeclStmt *)stmt;
    Ident *id;
    Vector_ForEach(id, varsStmt->ids) {
      __new_var(ps, id, varsStmt->type.desc, varsStmt->konst);
    }
  }
}

static int __validate_count(ParserState *ps, int lsz, int rsz)
{
  if (lsz < rsz) {
    /* var a = foo(), 100; whatever foo() is single or multi values */
    return 0;
  }

  if (lsz > rsz) {
    /*
     * if exprs > 1, it has an error
     * if exprs == 1, it's partially ok and must be a multi-values exprs
     * if exprs == 0, it's ok
    */
    if (rsz > 1)
      return 0;
  }

  /* if ids is equal with exprs, it MAYBE ok and will be checked in later */
  return 1;
}

Stmt *__Parser_Do_Variables(ParserState *ps, Vector *ids, TypeWrapper type,
                            Vector *exps, int konst)
{
  int isz = Vector_Size(ids);
  int esz = Vector_Size(exps);
  if (!__validate_count(ps, isz, esz)) {
    Expr *e = Vector_Get(exps, 0);
    Syntax_Error(ps, &e->pos, "left and right are not matched");
    Free_IdentList(ids);
    free_exprlist(exps);
    return NULL;
  }

  if (isz == esz) {
    Stmt *stmt;
    if (isz == 1) {
      /* only one variable */
      Ident *id = Vector_Get(ids, 0);
      Expr *exp = Vector_Get(exps, 0);
      TYPE_INCREF(type.desc);
      stmt = __Stmt_From_VarDecl(id, type, exp, konst);
      Free_Ident(id);
    } else {
      /* count of left ids == count of right expressions */
      Ident *id;
      Expr *exp;
      Stmt *varStmt;
      ListStmt *listStmt = (ListStmt *)Stmt_From_List(Vector_New());
      Vector_ForEach(id, ids) {
        exp = Vector_Get(exps, i);
        TYPE_INCREF(type.desc);
        varStmt = __Stmt_From_VarDecl(id, type, exp, konst);
        Free_Ident(id);
        Vector_Append(listStmt->vec, varStmt);
      }
      stmt = (Stmt *)listStmt;
    }

    Vector_Free_Self(ids);
    Vector_Free_Self(exps);
    return stmt;
  }

  assert(isz > esz && esz >=0 && esz <= 1);

  /* count of right expressions is 1 */
  if (esz == 1) {
    Expr *e = Vector_Get(exps, 0);
    Vector_Free_Self(exps);
    TYPE_INCREF(type.desc);
    return __Stmt_From_VarListDecl(ids, type, e, konst);
  }

  /* count of right expressions is 0 */
  assert(exps == NULL);

  if (isz == 1) {
    Ident *id = Vector_Get(ids, 0);
    Vector_Free_Self(ids);
    TYPE_INCREF(type.desc);
    Stmt *stmt = __Stmt_From_VarDecl(id, type, NULL, konst);
    Free_Ident(id);
    return stmt;
  } else {
    ListStmt *listStmt = (ListStmt *)Stmt_From_List(Vector_New());
    Ident *id;
    Stmt *varStmt;
    Vector_ForEach(id, ids) {
      TYPE_INCREF(type.desc);
      varStmt = __Stmt_From_VarDecl(id, type, NULL, konst);
      Free_Ident(id);
      Vector_Append(listStmt->vec, varStmt);
    }
    Vector_Free_Self(ids);
    return (Stmt *)listStmt;
  }
}

Stmt *Parser_Do_Typeless_Variables(ParserState *ps, Vector *ids, Vector *exps)
{
  int isz = Vector_Size(ids);
  int esz = Vector_Size(exps);
  if (!__validate_count(ps, isz, esz)) {
    Expr *e = Vector_Get(ids, 0);
    Syntax_Error(ps, &e->pos, "left and expr are not matched");
    free_exprlist(ids);
    free_exprlist(exps);
    return NULL;
  }

  IdentExpr *e;
  Expr *right;
  TypeWrapper nulltype = {NULL, {0, 0}};
  if (isz == esz) {
    Stmt *stmt;
    if (isz == 1) {
      /* only one variable */
      e = Vector_Get(ids, 0);
      if (e->kind != ID_KIND) {
        Syntax_Error(ps, &e->pos, "needs an identifier");
        free_exprlist(ids);
        free_exprlist(exps);
        return NULL;
      }
      Ident ident = {e->name, e->pos};
      Expr *exp = Vector_Get(exps, 0);
      stmt = __Stmt_From_VarDecl(&ident, nulltype, exp, 0);
    } else {
      /* count of left ids == count of right expressions */
      ListStmt *listStmt = (ListStmt *)Stmt_From_List(Vector_New());
      Ident ident;
      Stmt *varStmt;
      Vector_ForEach(e, ids) {
        if (e->kind != ID_KIND) {
          Syntax_Error(ps, &e->pos, "needs an identifier");
          Free_Stmt_Func(listStmt, NULL);
          free_exprlist(ids);
          free_exprlist(exps);
          return NULL;
        }
        right = Vector_Get(exps, i);
        assert(right);
        ident.name = e->name;
        ident.pos = e->pos;
        varStmt = __Stmt_From_VarDecl(&ident, nulltype, right, 0);
        Vector_Append(listStmt->vec, varStmt);
      }
      stmt = (Stmt *)listStmt;
    }
    free_exprlist(ids);
    Vector_Free_Self(exps);
    return stmt;
  }

  assert(isz > esz && esz == 1);

  /* count of right expressions is 1 */
  Vector *_ids = Vector_New();
  Ident *ident;
  String s;
  Vector_ForEach(e, ids) {
    if (e->kind != ID_KIND) {
      Syntax_Error(ps, &e->pos, "needs an identifier");
      Free_IdentList(_ids);
      free_exprlist(ids);
      free_exprlist(exps);
      return NULL;
    }
    s.str = e->name;
    ident = New_Ident(s);
    ident->pos = e->pos;
    Vector_Append(_ids, ident);
  }
  right = Vector_Get(exps, 0);

  free_exprlist(ids);
  Vector_Free_Self(exps);
  return __Stmt_From_VarListDecl(_ids, nulltype, right, 0);
}

Stmt *Parser_Do_Assignments(ParserState *ps, Vector *left, Vector *right)
{
  int lsz = Vector_Size(left);
  int rsz = Vector_Size(right);
  if (!__validate_count(ps, lsz, rsz)) {
    Expr *e = Vector_Get(right, 0);
    Syntax_Error(ps, &e->pos, "left and right are not matched");
    free_exprlist(left);
    free_exprlist(right);
    return NULL;
  }

  if (lsz == rsz) {
    Stmt *stmt;
    if (lsz == 1) {
      /* only one identifier */
      Expr *lexp = Vector_Get(left, 0);
      if (!Expr_Maybe_Stored(lexp)) {
        Syntax_Error(ps, &lexp->pos, "expr is not left expr");
        free_exprlist(left);
        free_exprlist(right);
        return NULL;
      }
      Expr *rexp = Vector_Get(right, 0);
      stmt = Stmt_From_Assign(OP_ASSIGN, lexp, rexp);
    } else {
      /* count of left expressions == count of right expressions */
      ListStmt *listStmt = (ListStmt *)Stmt_From_List(Vector_New());
      Expr *lexp, *rexp;
      Stmt *assignStmt;
      Vector_ForEach(lexp, left) {
        if (!Expr_Maybe_Stored(lexp)) {
          Syntax_Error(ps, &lexp->pos, "expr is not left expr");
          free_exprlist(left);
          free_exprlist(right);
          return NULL;
        }
        rexp = Vector_Get(right, i);
        assignStmt = Stmt_From_Assign(OP_ASSIGN, lexp, rexp);
        Vector_Append(listStmt->vec, assignStmt);
      }
      stmt = (Stmt *)listStmt;
    }
    Vector_Free_Self(left);
    Vector_Free_Self(right);
    return stmt;
  }

  assert(lsz > rsz && rsz == 1);

  /* count of right expressions is 1 */
  Expr *lexp;
  Vector_ForEach(lexp, left) {
    if (!Expr_Maybe_Stored(lexp)) {
      Syntax_Error(ps, &lexp->pos, "expr is not left expr");
      free_exprlist(left);
      free_exprlist(right);
      return NULL;
    }
  }
  Expr *rexp = Vector_Get(right, 0);
  Vector_Free_Self(right);
  return Stmt_From_AssignList(left, rexp);
}

static TypeDesc *__get_proto(FuncDeclStmt *stmt)
{
  Vector *pdesc = NULL;
  Vector *rdesc = NULL;
  IdType *idType;

  if (stmt->args != NULL) {
    pdesc = Vector_New();
    Vector_ForEach(idType, stmt->args) {
      TYPE_INCREF(idType->type.desc);
      Vector_Append(pdesc, idType->type.desc);
    }
  }

  if (stmt->rets != NULL) {
    rdesc = Vector_New();
    Vector_ForEach(idType, stmt->rets) {
      TYPE_INCREF(idType->type.desc);
      Vector_Append(rdesc, idType->type.desc);
    }
  }

  return TypeDesc_Get_Proto(pdesc, rdesc);
}

static void __parse_funcdecl(ParserState *ps, Stmt *stmt)
{
  ParserUnit *u = ps->u;
  FuncDeclStmt *funcStmt = (FuncDeclStmt *)stmt;
  char *name = funcStmt->id.name;
  Symbol *sym;

  sym = (Symbol *)__in_extstbl(ps, name);
  if (sym != NULL) {
    Syntax_Error(ps, &funcStmt->id.pos, "'%s' redeclared,\n"
                     "\tprevious declaration at %s:%d:%d", name,
                     sym->filename, sym->pos.row, sym->pos.col);
    return;
  }

  TypeDesc *proto = __get_proto(funcStmt);

  if (funcStmt->kind == PROTO_KIND) {
    if (funcStmt->native) {
      sym = STable_Add_NFunc(u->stbl, name, proto);
      if (sym != NULL) {
        Log_Debug("add native func '%s' successfully", name);
        sym->filename = ps->filename;
        sym->pos = funcStmt->id.pos;
      } else {
        sym = STable_Get(u->stbl, name);
        Syntax_Error(ps, &funcStmt->id.pos, "'%s' redeclared,\n"
                     "\tprevious declaration at %s:%d:%d", name,
                     sym->filename, sym->pos.row, sym->pos.col);
      }
    } else {
      sym = STable_Add_IFunc(u->stbl, name, proto);
      if (sym != NULL) {
        Log_Debug("add proto '%s' successfully", name);
        sym->filename = ps->filename;
        sym->pos = funcStmt->id.pos;
      } else {
        sym = STable_Get(u->stbl, name);
        Syntax_Error(ps, &funcStmt->id.pos, "'%s' redeclared,\n"
                     "\tprevious declaration at %s:%d:%d", name,
                     sym->filename, sym->pos.row, sym->pos.col);
      }
    }
  } else {
    assert(funcStmt->kind == FUNC_KIND);
    sym = STable_Add_Func(u->stbl, name, proto);
    if (sym != NULL) {
      Log_Debug("add func '%s' successfully", name);
      sym->filename = ps->filename;
      sym->pos = funcStmt->id.pos;
    } else {
      sym = STable_Get(u->stbl, name);
      Syntax_Error(ps, &funcStmt->id.pos, "'%s' redeclared,\n"
                   "\tprevious declaration at %s:%d:%d", name,
                   sym->filename, sym->pos.row, sym->pos.col);
    }
  }

  if (sym == NULL) {
    Position pos = funcStmt->id.pos;
    Syntax_Error(ps, &pos, "Symbol '%s' is duplicated", name);
    TYPE_DECREF(proto);
  }
}

void Parser_New_Function(ParserState *ps, Stmt *stmt)
{
  if (stmt != NULL) {
    __add_stmt(ps, stmt);
    __parse_funcdecl(ps, stmt);
  }
}

void Parser_New_TypeAlias(ParserState *ps, Stmt *stmt)
{
  if (stmt == NULL)
    return;
  assert(stmt->kind == TYPEALIAS_KIND);
  TypeAliasStmt *aliasStmt = (TypeAliasStmt *)stmt;
  STable_Add_Alias(ps->u->stbl, aliasStmt->id.name, aliasStmt->desc);
  Log_Debug("add typealias '%s' successful", aliasStmt->id.name);
  Mfree(stmt);
}

void Parser_New_ClassOrTrait(ParserState *ps, Stmt *stmt)
{
  if (stmt == NULL)
    return;
  __add_stmt(ps, stmt);

  ParserUnit *u = ps->u;
  KlassStmt *klsStmt = (KlassStmt *)stmt;
  char *name = klsStmt->id.name;
  Symbol *sym;

  sym = (Symbol *)__in_extstbl(ps, name);
  if (sym != NULL) {
    Syntax_Error(ps, &klsStmt->id.pos, "'%s' redeclared,\n"
                   "\tprevious declaration at %s:%d:%d", name,
                   sym->filename, sym->pos.row, sym->pos.col);
    return;
  }

  if (stmt->kind == CLASS_KIND) {
    sym = STable_Add_Class(u->stbl, name);
    if (sym != NULL) {
      Log_Debug("add class '%s' successfully", sym->name);
      sym->filename = ps->filename;
      sym->pos = klsStmt->id.pos;
    } else {
      sym = STable_Get(u->stbl, name);
      Syntax_Error(ps, &klsStmt->id.pos, "'%s' redeclared,\n"
                   "\tprevious declaration at %s:%d:%d", name,
                   sym->filename, sym->pos.row, sym->pos.col);
    }
  } else {
    assert(stmt->kind == TRAIT_KIND);
    sym = STable_Add_Trait(u->stbl, name);
    if (sym != NULL) {
      Log_Debug("add trait '%s' successfully", sym->name);
      sym->filename = ps->filename;
      sym->pos = klsStmt->id.pos;
    } else {
      sym = STable_Get(u->stbl, name);
      Syntax_Error(ps, &klsStmt->id.pos, "'%s' redeclared,\n"
                   "\tprevious declaration at %s:%d:%d", name,
                   sym->filename, sym->pos.row, sym->pos.col);
    }
  }

  Parser_Enter_Scope(ps, SCOPE_CLASS);
  /* ClassSymbol */
  ps->u->sym = sym;
  ps->u->stbl = ((ClassSymbol *)sym)->stbl;

  Stmt *s;
  Vector_ForEach(s, klsStmt->body) {
    if (s->kind == VAR_KIND) {
      VarDeclStmt *varStmt = (VarDeclStmt *)s;
      assert(varStmt->konst == 0);
      __new_var(ps, &varStmt->id, varStmt->type.desc, 0);
    } else if (s->kind == FUNC_KIND) {
      assert(s->kind == FUNC_KIND || s->kind == PROTO_KIND);
      __parse_funcdecl(ps, s);
      FuncDeclStmt *funcStmt = (FuncDeclStmt *)s;
      char *name = funcStmt->id.name;
      if (funcStmt->kind == PROTO_KIND) {
        if (funcStmt->native) {
          assert(sym->kind == SYM_CLASS);
          Log_Debug("add native func '%s' to '%s'", name, sym->name);
        } else {
          assert(sym->kind == SYM_TRAIT);
          Log_Debug("add proto '%s' to '%s'", name, sym->name);
        }
      } else {
        assert(funcStmt->kind == FUNC_KIND);
        assert(!funcStmt->native);
        Log_Debug("add func '%s' to '%s'", name, sym->name);
      }
    }
  }

  Parser_Exit_Scope(ps);
}

TypeDesc *Parser_New_KlassType(ParserState *ps, Ident *id, Ident *klazz)
{
  char *path = NULL;
  if (id != NULL) {
    Symbol *sym = STable_Get(ps->extstbl, id->name);
    if (sym == NULL) {
      Syntax_Error(ps, &id->pos, "undefined '%s'", id->name);
      return NULL;
    }
    assert(sym->kind == SYM_PKG);
    sym->used++;
    //ExtPkg *extpkg = ((ExtPkgSymbol *)sym)->extpkg;
    //path = extpkg->path;
    //sym = STable_Get(pkg->stbl, klazz->name);
    if (sym == NULL) {
      Syntax_Error(ps, &id->pos, "undefined '%s.%s'", path, klazz->name);
      return NULL;
    }
  }
  return TypeDesc_Get_Klass(path, klazz->name);
}

void Syntax_Error(ParserState *ps, Position *pos, char *fmt, ...)
{
  /* if errors are more than MAX_ERRORS, discard left errors shown */
  if (++ps->errnum > MAX_ERRORS) {
    fprintf(stderr, "%s:%d:%d: \x1b[31merror:\x1b[0m too many errors\n",
            ps->filename, pos->row, pos->col);
    return;
  }

  va_list ap;
  va_start(ap, fmt);
  fprintf(stderr, "%s:%d:%d: \x1b[31merror:\x1b[0m ",
          ps->filename, pos->row, pos->col);
  vfprintf(stderr, fmt, ap);
  puts(""); /* newline */
  va_end(ap);
}

int Lexer_DoYYInput(ParserState *ps, char *buf, int size, FILE *in)
{
  LineBuffer *linebuf = &ps->line;

  if (linebuf->lineleft <= 0) {
    if (!fgets(linebuf->buf, LINE_MAX_LEN, in)) {
      if (ferror(in))
        clearerr(in);
      return 0;
    }

    linebuf->linelen = strlen(linebuf->buf);
    linebuf->lineleft = linebuf->linelen;
    linebuf->lastpos = linebuf->pos;
    linebuf->lastpos.col += linebuf->len;
    linebuf->len = 0;
    linebuf->pos.row++;
    linebuf->pos.col = 0;
  }

  int sz = min(linebuf->lineleft, size);
  memcpy(buf, linebuf->buf, sz);
  linebuf->lineleft -= sz;
  return sz;
}

void Lexer_DoUserAction(ParserState *ps, char *text)
{
  LineBuffer *linebuf = &ps->line;
  linebuf->pos.col += linebuf->len;
  strncpy(linebuf->token, text, TOKEN_MAX_LEN);
  linebuf->len = strlen(text);
}
