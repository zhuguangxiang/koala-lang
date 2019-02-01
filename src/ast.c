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

Ident *New_Ident(String name)
{
  Ident *ident = mm_alloc(sizeof(Ident));
  ident->name = name.str;
  return ident;
}

void Free_Ident(Ident *id)
{
  if (id == NULL)
    return;

  mm_free(id);
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
  IdType *idType = mm_alloc(sizeof(IdType));
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
  mm_free(idtype);
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
  Expr *exp = mm_alloc(sizeof(Expr));
  exp->kind = NIL_KIND;
  return exp;
}

Expr *Expr_From_Self(void)
{
  Expr *exp = mm_alloc(sizeof(Expr));
  exp->kind = SELF_KIND;
  return exp;
}

Expr *Expr_From_Super(void)
{
  Expr *exp = mm_alloc(sizeof(Expr));
  exp->kind = SUPER_KIND;
  return exp;
}

Expr *Expr_From_Integer(int64 val)
{
  BaseExpr *baseExp = mm_alloc(sizeof(BaseExpr));
  baseExp->kind = INT_KIND;
  baseExp->ival = val;
  return (Expr *)baseExp;
}

Expr *Expr_From_Float(float64 val)
{
  BaseExpr *baseExp = mm_alloc(sizeof(BaseExpr));
  baseExp->kind = FLOAT_KIND;
  baseExp->fval = val;
  return (Expr *)baseExp;
}

Expr *Expr_From_Bool(int val)
{
  BaseExpr *baseExp = mm_alloc(sizeof(BaseExpr));
  baseExp->kind = BOOL_KIND;
  baseExp->bval = val;
  return (Expr *)baseExp;
}

Expr *Expr_From_String(char *val)
{
  BaseExpr *baseExp = mm_alloc(sizeof(BaseExpr));
  baseExp->kind = STRING_KIND;
  baseExp->str = val;
  return (Expr *)baseExp;
}

Expr *Expr_From_Char(uchar val)
{
  BaseExpr *baseExp = mm_alloc(sizeof(BaseExpr));
  baseExp->kind = CHAR_KIND;
  baseExp->ch = val;
  return (Expr *)baseExp;
}

Expr *Expr_From_Id(char *val)
{
  BaseExpr *baseExp = mm_alloc(sizeof(BaseExpr));
  baseExp->kind = ID_KIND;
  baseExp->id = val;
  return (Expr *)baseExp;
}

Expr *Expr_From_Unary(UnaryOpKind op, Expr *exp)
{
  UnaryExpr *unaryExp = mm_alloc(sizeof(UnaryExpr));
  unaryExp->kind = UNARY_KIND;
  unaryExp->op = op;
  unaryExp->exp = exp;
  return (Expr *)unaryExp;
}

Expr *Expr_From_Binary(BinaryOpKind op, Expr *left, Expr *right)
{
  BinaryExpr *binaryExp = mm_alloc(sizeof(BinaryExpr));
  binaryExp->kind = BINARY_KIND;
  binaryExp->op = op;
  binaryExp->lexp = left;
  binaryExp->rexp = right;
  return (Expr *)binaryExp;
}

Expr *Expr_From_Attribute(Ident id, Expr *left)
{
  AttributeExpr *attrExp = mm_alloc(sizeof(AttributeExpr));
  attrExp->kind = ATTRIBUTE_KIND;
  attrExp->id = id;
  attrExp->left = left;
  left->right = (Expr *)attrExp;
  return (Expr *)attrExp;
}

Expr *Expr_From_SubScript(Expr *index, Expr *left)
{
  SubScriptExpr *subExp = mm_alloc(sizeof(SubScriptExpr));
  subExp->kind = SUBSCRIPT_KIND;
  subExp->index = index;
  subExp->left = left;
  left->right = (Expr *)subExp;
  return (Expr *)subExp;
}

Expr *Expr_From_Call(Vector *args, Expr *left)
{
  CallExpr *callExp = mm_alloc(sizeof(CallExpr));
  callExp->kind = CALL_KIND;
  callExp->args = args;
  callExp->left = left;
  left->right = (Expr *)callExp;
  return (Expr *)callExp;
}

Expr *Expr_From_Slice(Expr *start, Expr *end, Expr *left)
{
  SliceExpr *sliceExp = mm_alloc(sizeof(SliceExpr));
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
  ListExpr *listExp = mm_alloc(sizeof(ListExpr));
  listExp->kind = ARRAY_LIST_KIND;
  listExp->nesting = nesting;
  listExp->vec = vec;
  return (Expr *)listExp;
}

Expr *Expr_From_Array(Vector *dims, TypeDesc *base, Expr *listExp)
{
  ArrayExpr *arrayExp = mm_alloc(sizeof(ArrayExpr));
  arrayExp->kind = ARRAY_KIND;
  arrayExp->dims = dims;
  arrayExp->base = base;
  assert(listExp != NULL ? listExp->kind == ARRAY_LIST_KIND : 1);
  arrayExp->listExp = (ListExpr *)listExp;
  return (Expr *)arrayExp;
}

Expr *Parser_New_Array(Vector *vec, int dims, TypeDesc *desc, Expr *listExp)
{
  Vector *dimsVec;
  TypeDesc *base;
  if (vec != NULL) {
    assert(dims == 0);
    dimsVec = vec;
    if (desc->kind == TYPE_ARRAY) {
      ArrayDesc *arrayDesc = (ArrayDesc *)desc;
      base = arrayDesc->base;
      /* append null to occupy a position */
      for (int i = 0; i < arrayDesc->dims; i++)
        Vector_Append(dimsVec, NULL);
      /* free array desc */
      TYPE_INCREF(base);
      TYPE_DECREF(desc);
    } else {
      base = desc;
      TYPE_INCREF(base);
    }
  } else {
    assert(dims != 0);
    dimsVec = Vector_New();
    /* append null to occupy a position */
    for (int i = 0; i < dims; i++)
      Vector_Append(dimsVec, NULL);
    assert(desc->kind != TYPE_ARRAY);
    base = desc;
    TYPE_INCREF(base);
  }
  return Expr_From_Array(dimsVec, base, listExp);
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
  ListExpr *listExp = mm_alloc(sizeof(ListExpr));
  listExp->kind = MAP_LIST_KIND;
  listExp->nesting = nesting;
  listExp->vec = vec;
  return (Expr *)listExp;
}

Expr *Expr_From_MapEntry(Expr *k, Expr *v)
{
  MapEntryExpr *entExp = mm_alloc(sizeof(MapEntryExpr));
  entExp->kind = MAP_ENTRY_KIND;
  entExp->key = k;
  entExp->val = v;
  return (Expr *)entExp;
}

Expr *Expr_From_Map(TypeDesc *desc, Expr *listExp)
{
  MapExpr *mapExp = mm_alloc(sizeof(MapExpr));
  mapExp->kind = MAP_KIND;
  TYPE_INCREF(desc);
  mapExp->mapDesc = desc;
  assert(listExp != NULL ? listExp->kind == MAP_LIST_KIND : 1);
  mapExp->listExp = (ListExpr *)listExp;
  return (Expr *)mapExp;
}

Expr *Expr_From_Set(TypeDesc *desc, Expr *listExp)
{
  SetExpr *setExp = mm_alloc(sizeof(SetExpr));
  setExp->kind = SET_KIND;
  TYPE_INCREF(desc);
  setExp->setDesc = desc;
  assert(listExp != NULL ? listExp->kind == ARRAY_LIST_KIND : 1);
  setExp->listExp = (ListExpr *)listExp;
  return (Expr *)setExp;
}

Expr *Expr_From_Anony(Vector *args, Vector *rets, Vector *body)
{
  AnonyExpr *anonyExp = mm_alloc(sizeof(AnonyExpr));
  anonyExp->kind = ANONY_FUNC_KIND;
  anonyExp->args = args;
  anonyExp->rets = rets;
  anonyExp->body = body;
  return (Expr *)anonyExp;
}

static void free_expr_func(void *item, void *arg);
static void expr_free(Expr *exp);

static void free_simple_expr(Expr *exp)
{
  TYPE_DECREF(exp->desc);
  mm_free(exp);
}

static void free_unary_expr(Expr *exp)
{
  UnaryExpr *unExp = (UnaryExpr *)exp;
  expr_free(unExp->exp);
  mm_free(exp);
}

static void free_binary_expr(Expr *exp)
{
  BinaryExpr *binExp = (BinaryExpr *)exp;
  expr_free(binExp->lexp);
  expr_free(binExp->rexp);
  mm_free(exp);
}

static void free_attribute_expr(Expr *exp)
{
  AttributeExpr *attrExp = (AttributeExpr *)exp;
  expr_free(attrExp->left);
  mm_free(exp);
}

static void free_subscript_expr(Expr *exp)
{
  SubScriptExpr *subExp = (SubScriptExpr *)exp;
  expr_free(subExp->index);
  expr_free(subExp->left);
  mm_free(exp);
}

static void free_call_expr(Expr *exp)
{
  CallExpr *callExp = (CallExpr *)exp;
  Vector_Free(callExp->args, free_expr_func, NULL);
  expr_free(callExp->left);
  mm_free(exp);
}

static void free_slice_expr(Expr *exp)
{
  SliceExpr *sliceExp = (SliceExpr *)exp;
  expr_free(sliceExp->start);
  expr_free(sliceExp->end);
  expr_free(sliceExp->left);
  mm_free(exp);
}

static void free_list_expr(Expr *exp)
{
  if (exp == NULL)
    return;
  ListExpr *listExp = (ListExpr *)exp;
  Vector_Free(listExp->vec, free_expr_func, NULL);
  mm_free(exp);
}

static void free_mapentry_expr(Expr *exp)
{
  MapEntryExpr *entExp = (MapEntryExpr *)exp;
  expr_free(entExp->key);
  expr_free(entExp->val);
  mm_free(exp);
}

static void free_array_expr(Expr *exp)
{
  ArrayExpr *arrayExp = (ArrayExpr *)exp;
  Vector_Free(arrayExp->dims, free_expr_func, NULL);
  TYPE_DECREF(arrayExp->base);
  free_list_expr((Expr *)arrayExp->listExp);
  mm_free(exp);
}

static void free_map_expr(Expr *exp)
{
  MapExpr *mapExp = (MapExpr *)exp;
  TYPE_DECREF(mapExp->mapDesc);
  free_list_expr((Expr *)mapExp->listExp);
  mm_free(exp);
}

static void free_set_expr(Expr *exp)
{
  SetExpr *setExp = (SetExpr *)exp;
  TYPE_DECREF(setExp->setDesc);
  free_list_expr((Expr *)setExp->listExp);
  mm_free(exp);
}

static void free_idtype_func(void *item, void *arg);

static void free_anony_expr(Expr *exp)
{
  AnonyExpr *anonyExp = (AnonyExpr *)exp;
  Vector_Free(anonyExp->args, free_idtype_func, NULL);
  Vector_Free(anonyExp->rets, free_idtype_func, NULL);
  Vector_Free(anonyExp->body, Free_Stmt_Func, NULL);
  mm_free(exp);
}

static void (*__free_expr_funcs[])(Expr *) = {
  NULL,                 /* INVALID          */
  free_simple_expr,     /* NIL_KIND         */
  free_simple_expr,     /* SELF_KIND        */
  free_simple_expr,     /* SUPER_KIND       */
  free_simple_expr,     /* INT_KIND         */
  free_simple_expr,     /* FLOAT_KIND       */
  free_simple_expr,     /* BOOL_KIND        */
  free_simple_expr,     /* STRING_KIND      */
  free_simple_expr,     /* CHAR_KIND        */
  free_simple_expr,     /* ID_KIND          */
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

static void expr_free(Expr *exp)
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
  expr_free(item);
}

static void free_vardecl_stmt(Stmt *stmt)
{
  VarDeclStmt *varStmt = (VarDeclStmt *)stmt;
  TYPE_DECREF(varStmt->desc);
  expr_free(varStmt->exp);
  mm_free(stmt);
}

static void free_varlistdecl_stmt(Stmt *stmt)
{
  VarListDeclStmt *varListStmt = (VarListDeclStmt *)stmt;
  Vector_Free_Self(varListStmt->ids);
  TYPE_DECREF(varListStmt->desc);
  mm_free(stmt);
}

static void free_assign_stmt(Stmt *stmt)
{
  mm_free(stmt);
}

static void free_assignlist_stmt(Stmt *stmt)
{
  AssignListStmt *assignListStmt = (AssignListStmt *)stmt;
  Vector_Free_Self(assignListStmt->left);
  mm_free(stmt);
}

static void free_idtype_func(void *item, void *arg)
{
  Free_IdType(item);
}

static void free_funcdecl_stmt(Stmt *stmt)
{
  FuncDeclStmt *funcStmt = (FuncDeclStmt *)stmt;
  Vector_Free(funcStmt->args, free_idtype_func, NULL);
  Vector_Free(funcStmt->rets, free_idtype_func, NULL);
  Vector_Free(funcStmt->body, Free_Stmt_Func, NULL);
  mm_free(stmt);
}

static void free_expr_stmt(Stmt *stmt)
{
  ExprStmt *expStmt = (ExprStmt *)stmt;
  expr_free(expStmt->exp);
  mm_free(stmt);
}

static void free_return_stmt(Stmt *stmt)
{
  ReturnStmt *retStmt = (ReturnStmt *)stmt;
  Vector_Free(retStmt->exps, free_expr_func, NULL);
  mm_free(stmt);
}

static void free_list_stmt(Stmt *stmt)
{
  ListStmt *listStmt = (ListStmt *)stmt;
  Vector_Free(listStmt->vec, Free_Stmt_Func, NULL);
  mm_free(stmt);
}

static void free_alias_stmt(Stmt *stmt)
{
  TypeAliasStmt *aliasStmt = (TypeAliasStmt *)stmt;
  TYPE_DECREF(aliasStmt->desc);
  mm_free(stmt);
}

static void free_typedesc_func(void *item, void *arg)
{
  TYPE_DECREF(item);
}

static void free_klass_stmt(Stmt *stmt)
{
  KlassStmt *klsStmt = (KlassStmt *)stmt;
  Vector_Free(klsStmt->super, free_typedesc_func, NULL);
  Vector_Free(klsStmt->body, Free_Stmt_Func, NULL);
  mm_free(stmt);
}

static void free_proto_stmt(Stmt *stmt)
{
  ProtoDeclStmt *protoStmt = (ProtoDeclStmt *)stmt;
  Vector_Free(protoStmt->args, free_idtype_func, NULL);
  Vector_Free(protoStmt->rets, free_idtype_func, NULL);
  mm_free(stmt);
}

static void (*__free_stmt_funcs[])(Stmt *) = {
  NULL,                     /* INVALID            */
  free_vardecl_stmt,        /* VAR_KIND           */
  free_varlistdecl_stmt,    /* VARLIST_EXPR_KIND  */
  free_assign_stmt,         /* ASSIGN_KIND        */
  free_assignlist_stmt,     /* ASSIGNLIST_KIND    */
  free_funcdecl_stmt,       /* FUNC_KIND          */
  free_proto_stmt,          /* PROTO_KIND         */
  free_expr_stmt,           /* EXPR_KIND          */
  free_return_stmt,         /* RETURN_KIND        */
  free_list_stmt,           /* LIST_KIND          */
  free_alias_stmt,          /* TYPEALIAS_KIND     */
  free_klass_stmt,          /* CLASS_KIND         */
  free_klass_stmt,          /* TRAIT_KIND         */
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

Stmt *__Stmt_From_VarDecl(Ident *id, TypeDesc *desc, Expr *exp, int k)
{
  VarDeclStmt *varStmt = mm_alloc(sizeof(VarDeclStmt));
  varStmt->kind = VAR_KIND;
  varStmt->id = *id;
  varStmt->desc = desc;
  varStmt->exp = exp;
  varStmt->konst = k;
  return (Stmt *)varStmt;
}

Stmt *__Stmt_From_VarListDecl(Vector *ids, TypeDesc *desc, Expr *exp, int k)
{
  VarListDeclStmt *varListStmt = mm_alloc(sizeof(VarListDeclStmt));
  varListStmt->kind = VARLIST_EXPR_KIND;
  varListStmt->ids = ids;
  varListStmt->desc = desc;
  varListStmt->exp = exp;
  varListStmt->konst = k;
  return (Stmt *)varListStmt;
}

Stmt *Stmt_From_Assign(AssignOpKind op, Expr *left, Expr *right)
{
  AssignStmt *assignStmt = mm_alloc(sizeof(AssignStmt));
  assignStmt->kind = ASSIGN_KIND;
  assignStmt->op = op;
  assignStmt->left = left;
  assignStmt->right = right;
  return (Stmt *)assignStmt;
}

Stmt *Stmt_From_AssignList(Vector *left, Expr *right)
{
  AssignListStmt *assignListStmt = mm_alloc(sizeof(AssignListStmt));
  assignListStmt->kind = ASSIGNLIST_KIND;
  assignListStmt->left = left;
  assignListStmt->right = right;
  return (Stmt *)assignListStmt;
}

Stmt *Stmt_From_FuncDecl(Ident id, Vector *args, Vector *rets, Vector *stmts)
{
  FuncDeclStmt *funcStmt = mm_alloc(sizeof(FuncDeclStmt));
  funcStmt->kind = FUNC_KIND;
  funcStmt->id = id;
  funcStmt->args = args;
  funcStmt->rets = rets;
  funcStmt->body = stmts;
  return (Stmt *)funcStmt;
}

Stmt *Stmt_From_Expr(Expr *exp)
{
  ExprStmt *expStmt = mm_alloc(sizeof(ExprStmt));
  expStmt->kind = EXPR_KIND;
  expStmt->exp = exp;
  return (Stmt *)expStmt;
}

Stmt *Stmt_From_Return(Vector *exps)
{
  ReturnStmt *retStmt = mm_alloc(sizeof(ReturnStmt));
  retStmt->kind = RETURN_KIND;
  retStmt->exps = exps;
  return (Stmt *)retStmt;
}

Stmt *Stmt_From_List(Vector *vec)
{
  ListStmt *listStmt = mm_alloc(sizeof(ListStmt));
  listStmt->kind = LIST_KIND;
  listStmt->vec = vec;
  return (Stmt *)listStmt;
}

Stmt *Stmt_From_TypeAlias(Ident id, TypeDesc *desc)
{
  TypeAliasStmt *aliasStmt = mm_alloc(sizeof(TypeAliasStmt));
  aliasStmt->kind = TYPEALIAS_KIND;
  aliasStmt->id = id;
  aliasStmt->desc = desc;
  return (Stmt *)aliasStmt;
}

Stmt *Stmt_From_Klass(Ident id, StmtKind kind, Vector *super, Vector *body)
{
  assert(kind == CLASS_KIND || kind == TRAIT_KIND);
  KlassStmt *klsStmt = mm_alloc(sizeof(KlassStmt));
  klsStmt->kind = kind;
  klsStmt->id = id;
  klsStmt->super = super;
  klsStmt->body = body;
  return (Stmt *)klsStmt;
}

Stmt *Stmt_From_ProtoDecl(Ident id, Vector *args, Vector *rets)
{
  ProtoDeclStmt *protoStmt = mm_alloc(sizeof(ProtoDeclStmt));
  protoStmt->kind = PROTO_KIND;
  protoStmt->id = id;
  protoStmt->args = args;
  protoStmt->rets = rets;
  return (Stmt *)protoStmt;
}

typedef struct import {
  HashNode hnode;
  char *path;
  STable *stbl;
} Import;

static uint32 import_hash_func(void *k)
{
  Import *import = k;
  return hash_string(import->path);
}

static int import_equal_func(void *k1, void *k2)
{
  Import *import1 = k1;
  Import *import2 = k2;
  return !strcmp(import1->path, import2->path);
}

static void import_free_func(HashNode *hnode, void *arg)
{
  UNUSED_PARAMETER(arg);
  Import *import = container_of(hnode, Import, hnode);
  mm_free(import);
}

void Init_Imports(ParserState *ps)
{
  HashTable_Init(&ps->imports, import_hash_func, import_equal_func);
  ps->extstbl = STable_New();
}

void Fini_Imports(ParserState *ps)
{
  HashTable_Fini(&ps->imports, import_free_func, NULL);
  STable_Free(ps->extstbl, NULL, NULL);
}

static Import *__find_import(ParserState *ps, char *path)
{
  Import key = {.path = path};
  HashNode *hnode = HashTable_Find(&ps->imports, &key);
  if (hnode == NULL)
    return NULL;
  return container_of(hnode, Import, hnode);
}

static Import *__new_import(ParserState *ps, char *path, STable *stbl)
{
  Import *import = mm_alloc(sizeof(Import));
  import->path = path;
  Init_HashNode(&import->hnode, import);
  import->stbl = stbl;
  int result = HashTable_Insert(&ps->imports, &import->hnode);
  assert(!result);
  return import;
}

struct extpkg {
  char *name;
  STable *stbl;
};

static struct extpkg *new_extpkg(char *name, STable *stbl)
{
  struct extpkg *extpkg = mm_alloc(sizeof(struct extpkg));
  extpkg->name = name;
  extpkg->stbl = stbl;
  return extpkg;
}

static void free_extpkg(struct extpkg *extpkg)
{
  mm_free(extpkg);
}

#if 0
static void compile_pkg(char *path, Options *opts)
{
  Log_Debug("load package '%s' failed, try to compile it", path);
  pid_t pid = fork();
  if (pid == 0) {
    Log_Debug("child process %d", getpid());
    int argc = 3 + options_number(opts);
    char *argv[argc];
    argv[0] = "koalac";
    options_toarray(opts, argv, 1);
    argv[argc - 2] = path;
    argv[argc - 1] = NULL;
    execvp("koalac", argv);
    assert(0); /* never go here */
  }
  int status = 0;
  pid = wait(&status);
  Log_Debug("child process %d return status:%d", pid, status);
  if (WIFEXITED(status)) {
    int exitstatus = WEXITSTATUS(status);
    Log_Debug("child process %d: %s", pid, strerror(exitstatus));
    if (exitstatus)
      exit(-1);
  }
}

static void __to_stbl_fn(HashNode *hnode, void *arg)
{
  STable *stbl = arg;
  MemberDef *m = container_of(hnode, MemberDef, hnode);

  switch (m->kind) {
  case MEMBER_CLASS: {
    ClassSymbol *clsSym = STable_Add_Class(stbl, m->name);
    HashTable_Visit(m->klazz->table, __to_stbl_fn, clsSym->stbl);
    break;
  }
  case MEMBER_TRAIT: {
    ClassSymbol *clsSym = STable_Add_Trait(stbl, m->name);
    HashTable_Visit(m->klazz->table, __to_stbl_fn, clsSym->stbl);
    break;
  }
  case MEMBER_VAR: {
    if (m->k)
      STable_Add_Const(stbl, m->name, m->desc);
    else
      STable_Add_Var(stbl, m->name, m->desc);
    break;
  }
  case MEMBER_CODE: {
    STable_Add_Func(stbl, m->name, m->desc);
    break;
  }
  case MEMBER_PROTO: {
    STable_Add_IFunc(stbl, m->name, m->desc);
    break;
  }
  default: {
    assert(0);
    break;
  }
  }
}

static STable *pkg_to_stbl(Object *ob)
{
  PackageObject *pkg = (PackageObject *)ob;
  STable *stbl = STable_New();
  HashTable_Visit(pkg->table, __to_stbl_fn, stbl);
  return stbl;
}
#endif

static struct extpkg *load_extpkg(ParserState *ps, char *path)
{
  return NULL;
  #if 0
  Object *pkg = Koala_Get_Package(path);
  if (pkg == NULL) {
    compile_pkg(path, ps->pkg->opts);
    pkg = Koala_Load_Package(path);
    if (pkg == NULL)
      return NULL;
  }

  STable *stbl = pkg_to_stbl(pkg);
  if (stbl == NULL)
    return NULL;

  return new_extpkg(Package_Name(pkg), stbl);
  #endif
}

Symbol *Parser_New_Import(ParserState *ps, char *id, char *path,
                          Position *idloc, Position *pathloc)
{
  Import *import =  __find_import(ps, path);
  if (import != NULL) {
    Syntax_Error(ps, pathloc, "Package '%s' is imported duplicately.", path);
    return NULL;
  }

  ImportSymbol *sym;
  if (id != NULL) {
    sym = (ImportSymbol *)STable_Get(ps->extstbl, id);
    if (sym != NULL) {
      Syntax_Error(ps, idloc, "Symbol '%s' is duplicated.", id);
      return NULL;
    }
  }

  struct extpkg *extpkg = load_extpkg(ps, path);
  if (extpkg == NULL) {
    Syntax_Error(ps, pathloc, "Package '%s' is loaded failure.", path);
    return NULL;
  }

  /* use package-name as imported-name if imported-name is not set */
  if (id == NULL)
    id = extpkg->name;

  /* save external symbol table and free struct extpkg */
  STable *extstbl = extpkg->stbl;
  free_extpkg(extpkg);

  sym = (ImportSymbol *)STable_Get(ps->extstbl, id);
  if (sym != NULL) {
    free_extpkg(extpkg);
    Syntax_Error(ps, idloc, "Symbol '%s' is duplicated.", id);
    return NULL;
  }

  import = __new_import(ps, path, extstbl);
  assert(import != NULL);

  sym = STable_Add_Import(ps->extstbl, id);
  assert(sym != NULL);
  sym->import = import;

  Log_Debug("add package '%s <- %s' successfully", id, path);

  return (Symbol *)sym;
}

static inline void __add_stmt(ParserState *ps, Stmt *stmt)
{
  Vector_Append(&ps->stmts, stmt);
}

static void __new_var(ParserState *ps, Ident *id, TypeDesc *desc, int konst)
{
  VarSymbol *sym;
  if (konst)
    sym = STable_Add_Const(ps->u->stbl, id->name, desc);
  else
    sym = STable_Add_Var(ps->u->stbl, id->name, desc);

  if (sym != NULL) {
    if (konst)
      Log_Debug("add const '%s' successfully", id->name);
    else
      Log_Debug("add var '%s' successfully", id->name);
    sym->parent = ps->u->sym;
  } else {
    Syntax_Error(ps, &id->pos, "Symbol '%s' is duplicated", id->name);
  }
}

void Parser_New_Variables(ParserState *ps, Stmt *stmt)
{
  if (stmt == NULL)
    return;

  if (stmt->kind == VAR_KIND) {
    VarDeclStmt *varStmt = (VarDeclStmt *)stmt;
    __add_stmt(ps, stmt);
    __new_var(ps, &varStmt->id, varStmt->desc, varStmt->konst);
  } else if (stmt->kind == LIST_KIND) {
    ListStmt *listStmt = (ListStmt *)stmt;
    Stmt *s;
    VarDeclStmt *varStmt;
    Vector_ForEach(s, listStmt->vec) {
      assert(s->kind == VAR_KIND);
      __add_stmt(ps, s);
      varStmt = (VarDeclStmt *)s;
      __new_var(ps, &varStmt->id, varStmt->desc, varStmt->konst);
    }
    Vector_Free_Self(listStmt->vec);
    mm_free(listStmt);
  } else {
    assert(stmt->kind == VARLIST_EXPR_KIND);
    __add_stmt(ps, stmt);
    VarListDeclStmt *varsStmt = (VarListDeclStmt *)stmt;
    Ident *id;
    Vector_ForEach(id, varsStmt->ids) {
      __new_var(ps, id, varsStmt->desc, varsStmt->konst);
    }
  }
}

static int __validate_count(ParserState *ps, int lsz, int rsz)
{
  if (lsz < rsz) {
    /* var a = foo(), 100; whatever foo() is single or multi values */
    Syntax_Error(ps, NULL, "extra expression in var declaration");
    return 0;
  }

  if (lsz > rsz) {
    /*
     * if exprs > 1, it has an error
     * if exprs == 1, it's partially ok and must be a multi-values exprs
     * if exprs == 0, it's ok
    */
    if (rsz > 1) {
      Syntax_Error(ps, NULL, "missing expression in var declaration");
      return 0;
    }
  }

  /* if ids is equal with exprs, it MAYBE ok and will be checked in later */
  return 1;
}

Stmt *__Parser_Do_Variables(ParserState *ps, Vector *ids, TypeDesc *desc,
                            Vector *exps, int k)
{
  int isz = Vector_Size(ids);
  int esz = Vector_Size(exps);
  if (!__validate_count(ps, isz, esz)) {
    /* FIXME: */
    return NULL;
  }

  if (isz == esz) {
    Ident *id;
    Expr *exp;
    Stmt *stmt;

    if (isz == 1) {
      /* only one variable */
      id = Vector_Get(ids, 0);
      exp = Vector_Get(exps, 0);
      TYPE_INCREF(desc);
      stmt = __Stmt_From_VarDecl(id, desc, exp, k);
      Free_Ident(id);
    } else {
      /* count of left ids == count of right expressions */
      Stmt *varStmt;
      stmt = Stmt_From_List(Vector_New());
      Vector_ForEach(id, ids) {
        exp = Vector_Get(exps, i);
        TYPE_INCREF(desc);
        varStmt = __Stmt_From_VarDecl(id, desc, exp, k);
        Free_Ident(id);
        Vector_Append(((ListStmt *)stmt)->vec, varStmt);
      }
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
    TYPE_INCREF(desc);
    return __Stmt_From_VarListDecl(ids, desc, e, k);
  }

  /* count of right expressions is 0 */
  assert(exps == NULL);

  if (isz == 1) {
    Ident *id = Vector_Get(ids, 0);
    Vector_Free_Self(ids);
    TYPE_INCREF(desc);
    Stmt *stmt = __Stmt_From_VarDecl(id, desc, NULL, k);
    Free_Ident(id);
    return stmt;
  } else {
    ListStmt *listStmt = (ListStmt *)Stmt_From_List(Vector_New());
    Ident *id;
    Stmt *varStmt;
    Vector_ForEach(id, ids) {
      TYPE_INCREF(desc);
      varStmt = __Stmt_From_VarDecl(id, desc, NULL, k);
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
  if (!__validate_count(ps, isz, esz))
    return NULL;

  if (isz == esz) {
    /* count of left ids == count of right expressions */
    ListStmt *listStmt = (ListStmt *)Stmt_From_List(Vector_New());

    BaseExpr *e;
    Expr *right;
    Ident ident;
    Stmt *varStmt;
    Vector_ForEach(e, ids) {
      assert(e->kind == ID_KIND);
      right = Vector_Get(exps, i);
      assert(right);
      ident.name = e->id;
      ident.pos = e->pos;
      varStmt = __Stmt_From_VarDecl(&ident, NULL, right, 0);
      Vector_Append(listStmt->vec, varStmt);
    }
    Vector_Free(ids, free_expr_func, NULL);
    Vector_Free_Self(exps);
    return (Stmt *)listStmt;
  }

  assert(isz > esz && esz == 1);

  /* count of right expressions is 1 */
  Vector *_ids = Vector_New();
  BaseExpr *e;
  Vector_ForEach(e, ids) {
    assert(e->kind == ID_KIND);
    Vector_Append(_ids, e->id);
  }
  Expr *right = Vector_Get(exps, 0);

  Vector_Free(ids, free_expr_func, NULL);
  Vector_Free_Self(exps);
  return __Stmt_From_VarListDecl(_ids, NULL, right, 0);
}

Stmt *Parser_Do_Assignments(ParserState *ps, Vector *left, Vector *right)
{
  int lsz = Vector_Size(left);
  int rsz = Vector_Size(right);
  if (!__validate_count(ps, lsz, rsz))
    return NULL;

  if (lsz == rsz) {
    /* count of left expressions == count of right expressions */
    ListStmt *listStmt = (ListStmt *)Stmt_From_List(Vector_New());

    Expr *lexp, *rexp;
    Stmt *assignStmt;
    Vector_ForEach(lexp, left) {
      rexp = Vector_Get(right, i);
      assignStmt = Stmt_From_Assign(OP_ASSIGN, lexp, rexp);
      Vector_Append(listStmt->vec, assignStmt);
    }
    Vector_Free_Self(left);
    Vector_Free_Self(right);
    return (Stmt *)listStmt;
  }

  assert(lsz > rsz && rsz == 1);

  Expr *e = Vector_Get(right, 0);
  Vector_Free_Self(right);
  return Stmt_From_AssignList(left, e);
}

static void __parse_funcdecl(ParserState *ps, Stmt *stmt)
{
  assert(stmt->kind == FUNC_KIND);
  FuncDeclStmt *funcStmt = (FuncDeclStmt *)stmt;
  TypeDesc *proto;
  FuncSymbol *sym;
  Vector *pdesc = NULL;
  Vector *rdesc = NULL;
  IdType *idType;

  if (funcStmt->args != NULL) {
    pdesc = Vector_New();
    Vector_ForEach(idType, funcStmt->args) {
      TYPE_INCREF(idType->type.desc);
      Vector_Append(pdesc, idType->type.desc);
    }
  }
  if (funcStmt->rets != NULL) {
    rdesc = Vector_New();
    Vector_ForEach(idType, funcStmt->rets) {
      TYPE_INCREF(idType->type.desc);
      Vector_Append(rdesc, idType->type.desc);
    }
  }
  proto = TypeDesc_Get_Proto(pdesc, rdesc);

  sym = STable_Add_Func(ps->u->stbl, funcStmt->id.name, proto);
  if (sym != NULL) {
    Log_Debug("add func '%s' successfully", funcStmt->id.name);
    sym->parent = ps->u->sym;
  } else {
    Syntax_Error(ps, &funcStmt->id.pos,
                 "Symbol '%s' is duplicated.", funcStmt->id.name);
  }
}

static void __parse_proto(ParserState *ps, ProtoDeclStmt *stmt)
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

  Symbol *sym;
  TypeDesc *proto = TypeDesc_Get_Proto(pdesc, rdesc);
  if (stmt->native)
    sym = (Symbol *)STable_Add_NFunc(ps->u->stbl, stmt->id.name, proto);
  else
    sym = (Symbol *)STable_Add_IFunc(ps->u->stbl, stmt->id.name, proto);

  if (sym != NULL) {
    Log_Debug("add %s '%s' successfully",
              (stmt->native ? "nfunc" : "proto"), stmt->id.name);
    sym->parent = ps->u->sym;
  } else {
    Syntax_Error(ps, &stmt->id.pos, "Symbol '%s' is duplicated", stmt->id.name);
    TYPE_DECREF(proto);
  }
}

void Parser_New_Function(ParserState *ps, Stmt *stmt)
{
  if (stmt == NULL)
    return;
  __add_stmt(ps, stmt);
  if (stmt->native) {
    assert(stmt->kind == PROTO_KIND);
    __parse_proto(ps, (ProtoDeclStmt *)stmt);
  } else {
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
  mm_free(stmt);
}

void Parser_New_ClassOrTrait(ParserState *ps, Stmt *stmt)
{
  if (stmt == NULL)
    return;
  __add_stmt(ps, stmt);

  KlassStmt *klsStmt = (KlassStmt *)stmt;
  ClassSymbol *sym;
  if (stmt->kind == CLASS_KIND) {
    sym = STable_Add_Class(ps->u->stbl, klsStmt->id.name);
    Log_Debug("add class '%s' successfully", sym->name);
  } else {
    assert(stmt->kind == TRAIT_KIND);
    sym = STable_Add_Trait(ps->u->stbl, klsStmt->id.name);
    Log_Debug("add trait '%s' successfully", sym->name);
  }

  sym->parent = ps->u->sym;
  Parser_Enter_Scope(ps, SCOPE_CLASS);
  /* ClassSymbol */
  ps->u->sym = (Symbol *)sym;
  ps->u->stbl = sym->stbl;

  Stmt *s;
  Vector_ForEach(s, klsStmt->body) {
    if (s->kind == VAR_KIND) {
      VarDeclStmt *varStmt = (VarDeclStmt *)s;
      assert(varStmt->konst == 0);
      __new_var(ps, &varStmt->id, varStmt->desc, 0);
    } else if (s->kind == FUNC_KIND) {
      __parse_funcdecl(ps, s);
    } else {
      assert(s->kind == PROTO_KIND);
      ProtoDeclStmt *proto = (ProtoDeclStmt *)s;
      if (s->native) {
        assert(sym->kind == SYM_CLASS);
        Log_Debug("add func '%s' to '%s'", proto->id.name, sym->name);
      } else {
        Log_Debug("add proto '%s' to '%s'", proto->id.name, sym->name);
        assert(sym->kind == SYM_TRAIT);
      }
      __parse_proto(ps, proto);
    }
  }

  Parser_Exit_Scope(ps);
}

TypeDesc *Parser_New_KlassType(ParserState *ps, char *id, char *klazz)
{
  char *path = NULL;
  if (id != NULL) {
    Symbol *sym = STable_Get(ps->extstbl, id);
    if (sym == NULL) {
      Log_Error("cannot find package: '%s'", id);
      return NULL;
    }
    assert(sym->kind == SYM_IMPORT);
    sym->refcnt++;
    Import *import = ((ImportSymbol *)sym)->import;
    path = import->path;
  }
  return TypeDesc_Get_Klass(path, klazz);
}

static void print_error(ParserState *ps, Position *pos, char *fmt, va_list ap)
{
  fprintf(stderr, "%s:%d:%d: \x1b[31merror:\x1b[0m ", ps->filename, pos->row, pos->col);
  vfprintf(stderr, fmt, ap);
  puts(""); /* newline */
}

void Syntax_Error(ParserState *ps, Position *pos, char *fmt, ...)
{
  if (++ps->errnum >= MAX_ERRORS) {
    fprintf(stderr, "Too many errors.\n");
    exit(-1);
  }

  if (ps->line.errors > 0)
    return;

  ps->line.errors++;

  va_list ap;
  va_start(ap, fmt);
  print_error(ps, pos, fmt, ap);
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
    linebuf->len = 0;
    linebuf->row++;
    linebuf->col = 0;
    linebuf->errors = 0;
  }

  int sz = min(linebuf->lineleft, size);
  memcpy(buf, linebuf->buf, sz);
  linebuf->lineleft -= sz;
  return sz;
}

void Lexer_DoUserAction(ParserState *ps, char *text)
{
  LineBuffer *linebuf = &ps->line;
  linebuf->col += linebuf->len;
  strncpy(linebuf->token, text, TOKEN_MAX_LEN);
  linebuf->len = strlen(text);
}
