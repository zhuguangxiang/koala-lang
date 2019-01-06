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
#include "globalstate.h"
#include "packageobject.h"

IdType *New_IdType(char *id, TypeDesc *desc)
{
  IdType *idType = mm_alloc(sizeof(IdType));
  idType->id = id;
  TYPE_INCREF(desc);
  idType->desc = desc;
  return idType;
}

void Free_IdType(IdType *idtype)
{
  TypeDesc_Free(idtype->desc);
  mm_free(idtype);
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

Expr *Expr_From_Char(uint32 val)
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

Expr *Expr_From_Attriubte(char *id, Expr *left)
{
  AttributeExpr *attrExp = mm_alloc(sizeof(AttributeExpr));
  attrExp->kind = ATTRIBUTE_KIND;
  attrExp->id = id;
  attrExp->left = left;
  return (Expr *)attrExp;
}

Expr *Expr_From_SubScript(Expr *index, Expr *left)
{
  SubScriptExpr *subExp = mm_alloc(sizeof(SubScriptExpr));
  subExp->kind = SUBSCRIPT_KIND;
  subExp->index = index;
  subExp->left = left;
  return (Expr *)subExp;
}

Expr *Expr_From_Call(Vector *args, Expr *left)
{
  CallExpr *callExp = mm_alloc(sizeof(CallExpr));
  callExp->kind = CALL_KIND;
  callExp->args = args;
  callExp->left = left;
  return (Expr *)callExp;
}

void Expr_Free(Expr *exp)
{
  if (exp != NULL)
    mm_free(exp);
}

static void free_vardecl_stmt(Stmt *stmt)
{
  VarDeclStmt *varStmt = (VarDeclStmt *)stmt;
  TypeDesc_Free(varStmt->desc);
  Expr_Free(varStmt->exp);
  mm_free(stmt);
}

static void free_varlistdecl_stmt(Stmt *stmt)
{
  VarListDeclStmt *varListStmt = (VarListDeclStmt *)stmt;
  Vector_Free(varListStmt->ids, NULL, NULL);
  TypeDesc_Free(varListStmt->desc);
  mm_free(stmt);
}

static void free_assign_stmt(Stmt *stmt)
{
  mm_free(stmt);
}

static void free_assignlist_stmt(Stmt *stmt)
{
  AssignListStmt *assignListStmt = (AssignListStmt *)stmt;
  Vector_Free(assignListStmt->left, NULL, NULL);
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
  mm_free(stmt);
}

static void free_return_stmt(Stmt *stmt)
{
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
  TypeDesc_Free(aliasStmt->desc);
  mm_free(stmt);
}

static void free_typedesc_func(void *item, void *arg)
{
  TypeDesc_Free(item);
}

static void free_class_stmt(Stmt *stmt)
{
  ClassStmt *clsStmt = (ClassStmt *)stmt;
  TypeDesc_Free(clsStmt->super);
  Vector_Free(clsStmt->traits, free_typedesc_func, NULL);
  Vector_Free(clsStmt->body, Free_Stmt_Func, NULL);
  mm_free(stmt);
}

static void free_trait_stmt(Stmt *stmt)
{
  TraitStmt *traitStmt = (TraitStmt *)stmt;
  Vector_Free(traitStmt->traits, free_typedesc_func, NULL);
  Vector_Free(traitStmt->body, Free_Stmt_Func, NULL);
  mm_free(stmt);
}

static void free_proto_stmt(Stmt *stmt)
{
  ProtoDeclStmt *protoStmt = (ProtoDeclStmt *)stmt;
  mm_free(stmt);
}

static void (*__free_stmt_funcs[])(Stmt *) = {
  NULL,
  free_vardecl_stmt,
  free_varlistdecl_stmt,
  free_assign_stmt,
  free_assignlist_stmt,
  free_funcdecl_stmt,
  free_expr_stmt,
  free_return_stmt,
  free_list_stmt,
  NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL,
  free_alias_stmt,
  free_class_stmt,
  free_trait_stmt,
  free_proto_stmt
};

void Free_Stmt_Func(void *item, void *arg)
{
  Stmt *stmt = item;
  assert(stmt->kind >= 1 && stmt->kind < nr_elts(__free_stmt_funcs));
  void (*__free_stmt_func)(Stmt *) = __free_stmt_funcs[stmt->kind];
  __free_stmt_func(stmt);
}

Stmt *__Stmt_From_VarDecl(char *id, TypeDesc *desc, Expr *exp, int k)
{
  VarDeclStmt *varStmt = mm_alloc(sizeof(VarDeclStmt));
  varStmt->kind = VAR_KIND;
  varStmt->id = id;
  varStmt->desc = desc;
  varStmt->exp = exp;
  varStmt->konst = k;
  return (Stmt *)varStmt;
}

Stmt *__Stmt_From_VarListDecl(Vector *ids, TypeDesc *desc, Expr *exp, int k)
{
  VarListDeclStmt *varListStmt = mm_alloc(sizeof(VarListDeclStmt));
  varListStmt->kind = VARLIST_KIND;
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

Stmt *Stmt_From_FuncDecl(char *id, Vector *args, Vector *rets, Vector *stmts)
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

Stmt *Stmt_From_TypeAlias(char *id, TypeDesc *desc)
{
  TypeAliasStmt *aliasStmt = mm_alloc(sizeof(TypeAliasStmt));
  aliasStmt->kind = TYPEALIAS_KIND;
  aliasStmt->id = id;
  aliasStmt->desc = desc;
  return (Stmt *)aliasStmt;
}

Stmt *Stmt_From_Class(TypeDesc *super, Vector *traits)
{
  ClassStmt *clsStmt = mm_alloc(sizeof(ClassStmt));
  clsStmt->kind = CLASS_KIND;
  clsStmt->super = super;
  clsStmt->traits = traits;
  return (Stmt *)clsStmt;
}

Stmt *Stmt_From_Trait(char *id, Vector *traits, Vector *body)
{
  TraitStmt *traitStmt = mm_alloc(sizeof(TraitStmt));
  traitStmt->kind = TRAIT_KIND;
  traitStmt->id = id;
  traitStmt->traits = traits;
  traitStmt->body = body;
  return (Stmt *)traitStmt;
}

Stmt *Stmt_From_ProtoDecl(char *id, Vector *args, Vector *rets)
{
  ProtoDeclStmt *protoStmt = mm_alloc(sizeof(ProtoDeclStmt));
  protoStmt->kind = PROTO_KIND;
  protoStmt->id = id;
  protoStmt->args = args;
  protoStmt->rets = rets;
  return (Stmt *)protoStmt;
}

int Parser_Set_Package(ParserState *ps, char *pkgname, YYLTYPE *loc)
{
  ps->pkgname = pkgname;

  PkgInfo *pkg = ps->pkg;
  if (pkg->pkgname == NULL) {
    pkg->pkgname = pkgname;
    return 0;
  }

  if (strcmp(pkg->pkgname, pkgname)) {
    Parser_Synatx_Error(ps, loc, "found packages %s(%s) and %s(%s)",
      pkg->pkgname, pkg->lastfile, pkgname, ps->filename);
    return -1;
  }

  return 0;
}

typedef struct import {
  HashNode hnode;
  char *path;
  SymbolTable *stbl;
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

static Import *__new_import(ParserState *ps, char *path, SymbolTable *stbl)
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
  SymbolTable *stbl;
};

static struct extpkg *new_extpkg(char *name, SymbolTable *stbl)
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

static void compile_pkg(char *path, struct options *opts)
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
  SymbolTable *stbl = arg;
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

static SymbolTable *pkg_to_stbl(Object *ob)
{
  PackageObject *pkg = (PackageObject *)ob;
  SymbolTable *stbl = STable_New();
  HashTable_Visit(pkg->table, __to_stbl_fn, stbl);
  return stbl;
}

static struct extpkg *load_extpkg(ParserState *ps, char *path)
{
  Object *pkg = Koala_Get_Package(path);
  if (pkg == NULL) {
    compile_pkg(path, ps->pkg->opts);
    pkg = Koala_Load_Package(path);
    if (pkg == NULL)
      return NULL;
  }

  SymbolTable *stbl = pkg_to_stbl(pkg);
  if (stbl == NULL)
    return NULL;

  return new_extpkg(Package_Name(pkg), stbl);
}

Symbol *Parser_New_Import(ParserState *ps, char *id, char *path,
  YYLTYPE *idloc, YYLTYPE *pathloc)
{
  Import *import =  __find_import(ps, path);
  if (import != NULL) {
    Parser_Synatx_Error(ps, pathloc,
      "Package '%s' is imported duplicately.", path);
    return NULL;
  }

  ImportSymbol *sym;
  if (id != NULL) {
    sym = (ImportSymbol *)STable_Get(ps->extstbl, id);
    if (sym != NULL) {
      Parser_Synatx_Error(ps, idloc, "Symbol '%s' is duplicated.", path);
      return NULL;
    }
  }

  struct extpkg *extpkg = load_extpkg(ps, path);
  if (extpkg == NULL) {
    Parser_Synatx_Error(ps, pathloc, "Package '%s' is loaded failure.", path);
    return NULL;
  }

  /* use package-name as imported-name if imported-name is not set */
  if (id == NULL)
    id = extpkg->name;

  /* save external symbol table and free struct extpkg */
  SymbolTable *extstbl = extpkg->stbl;
  free_extpkg(extpkg);

  sym = (ImportSymbol *)STable_Get(ps->extstbl, id);
  if (sym != NULL) {
    free_extpkg(extpkg);
    Parser_Synatx_Error(ps, idloc, "Symbol '%s' is duplicated.", path);
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

static void __new_var(ParserState *ps, char *id, TypeDesc *desc, int konst)
{
  VarSymbol *sym;
  if (konst)
    sym = STable_Add_Const(ps->u->stbl, id, desc);
  else
    sym = STable_Add_Var(ps->u->stbl, id, desc);

  if (sym != NULL) {
    if (konst)
      Log_Debug("add const '%s' successfully", id);
    else
      Log_Debug("add var '%s' successfully", id);
    sym->parent = ps->u->sym;
  } else {
    Parser_Synatx_Error(ps, NULL, "Symbol '%s' is duplicated", id);
  }
}

void Parser_New_Variables(ParserState *ps, Stmt *stmt)
{
  if (stmt == NULL)
    return;

  if (stmt->kind == VAR_KIND) {
    VarDeclStmt *varStmt = (VarDeclStmt *)stmt;
    __add_stmt(ps, stmt);
    __new_var(ps, varStmt->id, varStmt->desc, varStmt->konst);
  } else if (stmt->kind == LIST_KIND) {
    ListStmt *listStmt = (ListStmt *)stmt;
    Stmt *s;
    VarDeclStmt *varStmt;
    Vector_ForEach(s, listStmt->vec) {
      assert(s->kind == VAR_KIND);
      __add_stmt(ps, s);
      varStmt = (VarDeclStmt *)s;
      __new_var(ps, varStmt->id, varStmt->desc, varStmt->konst);
    }
    Vector_Free(listStmt->vec, NULL, NULL);
    mm_free(listStmt);
  } else {
    assert(stmt->kind == VARLIST_KIND);
    __add_stmt(ps, stmt);
    VarListDeclStmt *varsStmt = (VarListDeclStmt *)stmt;
    char *id;
    Vector_ForEach(id, varsStmt->ids) {
      __new_var(ps, id, varsStmt->desc, varsStmt->konst);
    }
  }
}

static int __validate_count(ParserState *ps, int lsz, int rsz)
{
  if (lsz < rsz) {
    /* var a = foo(), 100; whatever foo() is single or multi values */
    Parser_Synatx_Error(ps, NULL, "extra expression in var declaration");
    return 0;
  }

  if (lsz > rsz) {
    /*
     * if exprs > 1, it has an error
     * if exprs == 1, it's partially ok and must be a multi-values exprs
     * if exprs == 0, it's ok
    */
    if (rsz > 1) {
      Parser_Synatx_Error(ps, NULL, "missing expression in var declaration");
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
  if (!__validate_count(ps, isz, esz))
    return NULL;

  if (isz == esz) {
    /* count of left ids == count of right expressions */
    ListStmt *listStmt = (ListStmt *)Stmt_From_List(Vector_New());

    char *id;
    Expr *exp;
    Stmt *varStmt;
    Vector_ForEach(id, ids) {
      exp = Vector_Get(exps, i);
      TYPE_INCREF(desc);
      varStmt = __Stmt_From_VarDecl(id, desc, exp, k);
      Vector_Append(listStmt->vec, varStmt);
    }
    Vector_Free(ids, NULL, NULL);
    Vector_Free(exps, NULL, NULL);
    return (Stmt *)listStmt;
  }

  assert(isz > esz && esz >=0 && esz <= 1);

  /* count of right expressions is 1 */
  if (esz == 1) {
    Expr *e = Vector_Get(exps, 0);
    Vector_Free(exps, NULL, NULL);
    TYPE_INCREF(desc);
    return __Stmt_From_VarListDecl(ids, desc, e, k);
  }

  /* count of right expressions is 0 */
  assert(exps == NULL);

  if (isz == 1) {
    char *id = Vector_Get(ids, 0);
    Vector_Free(ids, NULL, NULL);
    TYPE_INCREF(desc);
    return __Stmt_From_VarDecl(id, desc, NULL, k);
  } else {
    ListStmt *listStmt = (ListStmt *)Stmt_From_List(Vector_New());
    char *id;
    Stmt *varStmt;
    Vector_ForEach(id, ids) {
      TYPE_INCREF(desc);
      varStmt = __Stmt_From_VarDecl(id, desc, NULL, k);
      Vector_Append(listStmt->vec, varStmt);
    }
    Vector_Free(ids, NULL, NULL);
    return (Stmt *)listStmt;
  }
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

    char *id;
    Expr *lexp, *rexp;
    Stmt *assignStmt;
    Vector_ForEach(lexp, left) {
      rexp = Vector_Get(right, i);
      assignStmt = Stmt_From_Assign(OP_ASSIGN, lexp, rexp);
      Vector_Append(listStmt->vec, assignStmt);
    }
    Vector_Free(left, NULL, NULL);
    Vector_Free(right, NULL, NULL);
    return (Stmt *)listStmt;
  }

  assert(lsz > rsz && rsz == 1);

  Expr *e = Vector_Get(right, 0);
  Vector_Free(right, NULL, NULL);
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
      TYPE_INCREF(idType->desc);
      Vector_Append(pdesc, idType->desc);
    }
  }
  if (funcStmt->rets != NULL) {
    rdesc = Vector_New();
    Vector_ForEach(idType, funcStmt->rets) {
      TYPE_INCREF(idType->desc);
      Vector_Append(rdesc, idType->desc);
    }
  }
  proto = TypeDesc_Get_Proto(pdesc, rdesc);

  sym = STable_Add_Func(ps->u->stbl, funcStmt->id, proto);
  if (sym != NULL) {
    Log_Debug("add func '%s' successfully", funcStmt->id);
    sym->parent = ps->u->sym;
  } else {
    Parser_Synatx_Error(ps, NULL, "Symbol '%s' is duplicated.", funcStmt->id);
  }
}

void Parser_New_Function(ParserState *ps, Stmt *stmt)
{
  if (stmt == NULL)
    return;
  __add_stmt(ps, stmt);
  __parse_funcdecl(ps, stmt);
}

void Parser_New_TypeAlias(ParserState *ps, Stmt *stmt)
{
  assert(stmt->kind == TYPEALIAS_KIND);
  TypeAliasStmt *aliasStmt = (TypeAliasStmt *)stmt;
  STable_Add_Alias(ps->u->stbl, aliasStmt->id, aliasStmt->desc);
  Log_Debug("add typealias '%s' successful", aliasStmt->id);
  mm_free(stmt);
}

static void __parse_proto(ParserState *ps, ProtoDeclStmt *stmt)
{
  Vector *pdesc = NULL;
  Vector *rdesc = NULL;
  IdType *idType;

  if (stmt->args != NULL) {
    pdesc = Vector_New();
    Vector_ForEach(idType, stmt->args) {
      TYPE_INCREF(idType->desc);
      Vector_Append(pdesc, idType->desc);
    }
  }
  if (stmt->rets != NULL) {
    rdesc = Vector_New();
    Vector_ForEach(idType, stmt->rets) {
      TYPE_INCREF(idType->desc);
      Vector_Append(rdesc, idType->desc);
    }
  }

  TypeDesc *proto = TypeDesc_Get_Proto(pdesc, rdesc);
  IFuncSymbol *sym = STable_Add_IFunc(ps->u->stbl, stmt->id, proto);
  if (sym != NULL) {
    Log_Debug("add ifunc '%s' successfully", stmt->id);
    sym->parent = ps->u->sym;
  } else {
    Parser_Synatx_Error(ps, NULL, "Symbol '%s' is duplicated", stmt->id);
    TypeDesc_Free(proto);
  }
}

void Parser_New_ClassOrTrait(ParserState *ps, Stmt *stmt)
{
  __add_stmt(ps, stmt);

  Vector *body;
  ClassSymbol *sym;
  if (stmt->kind == CLASS_KIND) {
    ClassStmt *clsStmt = (ClassStmt *)stmt;
    sym = STable_Add_Class(ps->u->stbl, clsStmt->id);
    body = clsStmt->body;
    Log_Debug("add class '%s' successfully", sym->name);
  } else {
    assert(stmt->kind == TRAIT_KIND);
    TraitStmt *traitStmt = (TraitStmt *)stmt;
    sym = STable_Add_Trait(ps->u->stbl, traitStmt->id);
    body = traitStmt->body;
    Log_Debug("add trait '%s' successfully", sym->name);
  }

  sym->parent = ps->u->sym;
  Parser_Enter_Scope(ps, SCOPE_CLASS);
  /* ClassSymbol */
  Set_Unit_STable(ps->u, sym->stbl);
  Set_Unit_Symbol(ps->u, sym);
  if (body != NULL) {
    Stmt *s;
    Vector_ForEach(s, body) {
      if (s->kind == VAR_KIND) {
        VarDeclStmt *varStmt = (VarDeclStmt *)s;
        assert(varStmt->konst == 0);
        __new_var(ps, varStmt->id, varStmt->desc, 0);
      } else if (s->kind == FUNC_KIND) {
        __parse_funcdecl(ps, s);
      } else {
        assert(s->kind == PROTO_KIND);
        assert(sym->kind == SYM_TRAIT);
        __parse_proto(ps, (ProtoDeclStmt *)s);
      }
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

void Parser_SetLineInfo(ParserState *ps, LineInfo *line)
{
  LineBuffer *linebuf = &ps->line;
  line->line = strdup(linebuf->buf);
  line->row = linebuf->row;
  line->col = linebuf->col;
}

static void print_error(ParserState *ps, YYLTYPE *loc, char *fmt, va_list ap)
{
  fprintf(stderr, "%s:%d:%d: error: ", ps->filename,
    loc_row(loc), loc_col(loc));
  vfprintf(stderr, fmt, ap);
  puts(""); /* newline */
}

void Parser_Synatx_Error(ParserState *ps, YYLTYPE *loc, char *fmt, ...)
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
  print_error(ps, loc, fmt, ap);
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
