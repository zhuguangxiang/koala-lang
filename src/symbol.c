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

#include "symbol.h"
#include "codegen.h"
#include "hashfunc.h"
#include "log.h"
#include "mem.h"

static void *__symbol_new(SymKind kind, char *name, int size)
{
  Symbol *sym = mm_alloc(size);
  sym->name = strdup(name);
  sym->kind = kind;
  Init_HashNode(&sym->hnode, sym);
  return sym;
}

static void __symbol_free(Symbol *sym)
{
  free(sym->name);
  mm_free(sym);
}

struct genkimageinfo {
  KImage *image;
  char *classname;
};

static Symbol *__const_new(char *name)
{
  return __symbol_new(SYM_CONST, name, sizeof(VarSymbol));
}

static void __const_free(Symbol *sym)
{
  VarSymbol *varSym = (VarSymbol *)sym;
  TypeDesc_Free(varSym->desc);
  __symbol_free(sym);
}

static void __const_show(Symbol *sym)
{
  char buf[64];
  VarSymbol *varSym = (VarSymbol *)sym;
  printf("const %s ", varSym->name);
  TypeDesc_ToString(varSym->desc, buf);
  printf("%s;\n", buf); /* with newline */
}

static void __const_gen(Symbol *sym, void *arg)
{
  struct genkimageinfo *info = arg;
  assert(info->classname == NULL);
  VarSymbol *varSym = (VarSymbol *)sym;
  Log_Debug("const %s:", varSym->name);
  KImage_Add_Var(info->image, varSym->name, varSym->desc, 1);
}

static Symbol *__var_new(char *name)
{
  return __symbol_new(SYM_VAR, name, sizeof(VarSymbol));
}

static void __var_free(Symbol *sym)
{
  VarSymbol *varSym = (VarSymbol *)sym;
  TypeDesc_Free(varSym->desc);
  __symbol_free(sym);
}

static void __var_show(Symbol *sym)
{
  VarSymbol *varSym = (VarSymbol *)sym;
  printf("var %s ", varSym->name);
  if (varSym->desc != NULL) {
    char buf[64];
    TypeDesc_ToString(varSym->desc, buf);
    printf("%s;\n", buf); /* with newline */
  } else {
    printf("<undefined-type>;\n");
  }
}

static void __var_gen(Symbol *sym, void *arg)
{
  struct genkimageinfo *info = arg;
  VarSymbol *varSym = (VarSymbol *)sym;

  if (info->classname != NULL) {
    Log_Debug("  var '%s'", sym->name);
    KImage_Add_Field(info->image, info->classname, varSym->name, varSym->desc);
  } else {
    Log_Debug("var %s:", varSym->name);
    KImage_Add_Var(info->image, varSym->name, varSym->desc, 0);
  }
}

static Symbol *__func_new(char *name)
{
  return __symbol_new(SYM_FUNC, name, sizeof(FuncSymbol));
}

static void __func_free(Symbol *sym)
{
  FuncSymbol *funcSym = (FuncSymbol *)sym;
  TypeDesc_Free(funcSym->desc);
  /* FIXME */
  Vector_Fini(&funcSym->locvec, NULL, NULL);
  __symbol_free(sym);
}

static void __func_show(Symbol *sym)
{
  printf("func %s();", sym->name);
  /* FIXME
    proto_show(sym->proto);
  */
  puts(""); /* with newline */
}

static void __add_locvar(KImage *image, int index, Vector *vec, char *fmt)
{
  VarSymbol *varSym;
  Vector_ForEach(varSym, vec) {
    assert(varSym->kind == SYM_VAR);
    Log_Debug(fmt, varSym->name);
    KImage_Add_LocVar(image, varSym->name, varSym->desc, varSym->index, index);
  }
}

static void __func_gen(Symbol *sym, void *arg)
{
  struct genkimageinfo *info = arg;
  FuncSymbol *funcSym = (FuncSymbol *)sym;
  uint8 *code = NULL;
  int size = CodeBlock_To_RawCode(info->image, funcSym->code, &code);
  int locvars = Vector_Size(&funcSym->locvec);
  int index;

  if (info->classname != NULL) {
    Log_Debug("  func %s:", funcSym->name);
    index = KImage_Add_Method(info->image, info->classname, funcSym->name,
                              funcSym->desc, code, size);
    if (locvars <= 0) {
      Log_Debug("    no vars");
    } else {
      char *fmt = "    var '%s'";
      __add_locvar(info->image, index, &funcSym->locvec, fmt);
    }
  } else {
    Log_Debug("func %s:", funcSym->name);
    index = KImage_Add_Func(info->image, funcSym->name,
                            funcSym->desc, code, size);
    if (locvars <= 0) {
      Log_Debug("  no vars");
    } else {
      char *fmt = "  var '%s'";
      __add_locvar(info->image, index, &funcSym->locvec, fmt);
    }
  }
}

static Symbol *__alias_new(char *name)
{
  return __symbol_new(SYM_ALIAS, name, sizeof(AliasSymbol));
}

static void __alias_free(Symbol *sym)
{
}

static void __alias_show(Symbol *sym)
{
}

static void __alias_gen(Symbol *sym, void *arg)
{
}

static Symbol *__class_new(char *name)
{
  return __symbol_new(SYM_CLASS, name, sizeof(ClassSymbol));
}

static void __class_free(Symbol *sym)
{
}

static void __class_show(Symbol *sym)
{
  printf("class %s;\n", sym->name);
}

static void __gen_image_func(Symbol *sym, void *arg);

static void __class_gen(Symbol *sym, void *arg)
{
  struct genkimageinfo *info = arg;
  ClassSymbol *clsSym = (ClassSymbol *)sym;
  Log_Debug("class %s:", clsSym->name);

  char *path = NULL;
  char *type = NULL;
  TypeDesc *desc = clsSym->super;
  if (desc != NULL) {
    path = desc->klass.path.str;
    type = desc->klass.type.str;
  }
  KImage_Add_Class(info->image, clsSym->name, path, type, &clsSym->traits);

  struct genkimageinfo info2 = {info->image, clsSym->name};
  STable_Visit(clsSym->stbl, __gen_image_func, &info2);
}

static Symbol *__trait_new(char *name)
{
  return __symbol_new(SYM_TRAIT, name, sizeof(ClassSymbol));
}

static void __trait_free(Symbol *sym)
{
}

static void __trait_show(Symbol *sym)
{
  printf("trait %s;\n", sym->name);
}

static void __trait_gen(Symbol *sym, void *arg)
{
  struct genkimageinfo *info = arg;
  ClassSymbol *clsSym = (ClassSymbol *)sym;
  Log_Debug("trait %s:", clsSym->name);
  KImage_Add_Trait(info->image, clsSym->name, &clsSym->traits);

  struct genkimageinfo info2 = {info->image, clsSym->name};
  STable_Visit(clsSym->stbl, __gen_image_func, &info2);
}

static Symbol *__ifunc_new(char *name)
{
  return __symbol_new(SYM_IFUNC, name, sizeof(IFuncSymbol));
}

static void __ifunc_free(Symbol *sym)
{
}

static void __ifunc_show(Symbol *sym)
{
  printf("ifunc %s();\n", sym->name);
}

static void __ifunc_gen(Symbol *sym, void *arg)
{
  struct genkimageinfo *info = arg;
  IFuncSymbol *ifnSym = (IFuncSymbol *)sym;
  Log_Debug("  func %s;", ifnSym->name);
  KImage_Add_IMeth(info->image, info->classname, ifnSym->name, ifnSym->desc);
}

static Symbol *__import_new(char *name)
{
  return __symbol_new(SYM_IMPORT, name, sizeof(ImportSymbol));
}

static void __import_free(Symbol *sym)
{
  __symbol_free(sym);
}

struct symbol_operations {
  Symbol *(*__symbol_new)(char *name);
  void (*__symbol_free)(Symbol *sym);
  void (*__symbol_show)(Symbol *sym);
  void (*__symbol_gen)(Symbol *sym, void *arg);
} symops[] = {
  {NULL, NULL, NULL, NULL},                               /* INVALID    */
  {__const_new, __const_free, __const_show, __const_gen}, /* SYM_CONST  */
  {__var_new, __var_free, __var_show, __var_gen},         /* SYM_VAR    */
  {__func_new, __func_free, __func_show, __func_gen},     /* SYM_FUNC   */
  {__alias_new, __alias_free, __alias_show, __alias_gen}, /* SYM_ALIAS  */
  {__class_new, __class_free, __class_show, __class_gen}, /* SYM_CLASS  */
  {__trait_new, __trait_free, __trait_show, __trait_gen}, /* SYM_TRAIT  */
  {__ifunc_new, __ifunc_free, __ifunc_show, __ifunc_gen}, /* SYM_IFUNC  */
  {__import_new, __import_free, NULL, NULL}               /* SYM_IMPORT */
};

Symbol *Symbol_New(SymKind kind, char *name)
{
  assert(kind >= SYM_CONST && kind <= SYM_IMPORT);
  struct symbol_operations *ops = &symops[kind];
  return ops->__symbol_new(name);
}

void Symbol_Free(Symbol *sym)
{
  assert(sym->kind >= SYM_CONST && sym->kind <= SYM_IMPORT);
  struct symbol_operations *ops = &symops[sym->kind];
  ops->__symbol_free(sym);
}

static uint32 symbol_hash(void *k)
{
  Symbol *s = k;
  return hash_string(s->name);
}

static int symbol_equal(void *k1, void *k2)
{
  Symbol *s1 = k1;
  Symbol *s2 = k2;
  return !strcmp(s1->name, s2->name);
}

static void __symbol_free_fn(Symbol *sym, void *arg)
{
  UNUSED_PARAMETER(arg);
  Symbol_Free(sym);
}

SymbolTable *STable_New(void)
{
  SymbolTable *stbl = mm_alloc(sizeof(SymbolTable));
  HashTable_Init(&stbl->table, symbol_hash, symbol_equal);
  return stbl;
}

void STable_Free(SymbolTable *stbl, symbol_visit_func visit, void *arg)
{
  if (stbl == NULL)
    return;

  if (visit == NULL) {
    visit = __symbol_free_fn;
    arg = NULL;
  }

  STable_Visit(stbl, visit, arg);
  mm_free(stbl);
}

VarSymbol *STable_Add_Const(SymbolTable *stbl, char *name, TypeDesc *desc)
{
  VarSymbol *sym = (VarSymbol *)Symbol_New(SYM_CONST, name);
  if (HashTable_Insert(&stbl->table, &sym->hnode) < 0) {
    Symbol_Free((Symbol *)sym);
    return NULL;
  }
  sym->desc = desc;
  sym->index = stbl->varindex++;
  return sym;
}

VarSymbol *STable_Add_Var(SymbolTable *stbl, char *name, TypeDesc *desc)
{
  VarSymbol *sym = (VarSymbol *)Symbol_New(SYM_VAR, name);
  if (HashTable_Insert(&stbl->table, &sym->hnode) < 0) {
    Symbol_Free((Symbol *)sym);
    return NULL;
  }
  sym->desc = desc;
  sym->index = stbl->varindex++;
  return sym;
}

FuncSymbol *STable_Add_Func(SymbolTable *stbl, char *name, TypeDesc *proto)
{
  FuncSymbol *sym = (FuncSymbol *)Symbol_New(SYM_FUNC, name);
  if (HashTable_Insert(&stbl->table, &sym->hnode) < 0) {
    Symbol_Free((Symbol *)sym);
    return NULL;
  }
  Vector_Init(&sym->locvec);
  sym->desc = proto;
  return sym;
}

AliasSymbol *STable_Add_Alias(SymbolTable *stbl, char *name, TypeDesc *desc)
{
  AliasSymbol *sym = (AliasSymbol *)Symbol_New(SYM_ALIAS, name);
  if (HashTable_Insert(&stbl->table, &sym->hnode) < 0) {
    Symbol_Free((Symbol *)sym);
    return NULL;
  }
  sym->desc = desc;
  return sym;
}

ClassSymbol *STable_Add_Class(SymbolTable *stbl, char *name)
{
  ClassSymbol *sym = (ClassSymbol *)Symbol_New(SYM_CLASS, name);
  if (HashTable_Insert(&stbl->table, &sym->hnode) < 0) {
    Symbol_Free((Symbol *)sym);
    return NULL;
  }
  Vector_Init(&sym->traits);
  sym->stbl = STable_New();
  return sym;
}

ClassSymbol *STable_Add_Trait(SymbolTable *stbl, char *name)
{
  ClassSymbol *sym = (ClassSymbol *)Symbol_New(SYM_TRAIT, name);
  if (HashTable_Insert(&stbl->table, &sym->hnode) < 0) {
    Symbol_Free((Symbol *)sym);
    return NULL;
  }
  Vector_Init(&sym->traits);
  sym->stbl = STable_New();
  return sym;
}

IFuncSymbol *STable_Add_IFunc(SymbolTable *stbl, char *name, TypeDesc *proto)
{
  IFuncSymbol *sym = (IFuncSymbol *)Symbol_New(SYM_IFUNC, name);
  if (HashTable_Insert(&stbl->table, &sym->hnode) < 0) {
    Symbol_Free((Symbol *)sym);
    return NULL;
  }
  sym->desc = proto;
  return sym;
}

ImportSymbol *STable_Add_Import(SymbolTable *stbl, char *name)
{
  ImportSymbol *sym = (ImportSymbol *)Symbol_New(SYM_IMPORT, name);
  if (HashTable_Insert(&stbl->table, &sym->hnode) < 0) {
    Symbol_Free((Symbol *)sym);
    return NULL;
  }
  return sym;
}

Symbol *STable_Get(SymbolTable *stbl, char *name)
{
  Symbol key = {.name = name};
  HashNode *hnode = HashTable_Find(&stbl->table, &key);
  return hnode ? container_of(hnode, Symbol, hnode) : NULL;
}

static void __gen_image_func(Symbol *sym, void *arg)
{
  assert(sym->kind >= SYM_CONST && sym->kind <= SYM_IMPORT);
  struct symbol_operations *ops = &symops[sym->kind];
  ops->__symbol_gen(sym, arg);
}

KImage *Gen_Image(SymbolTable *stbl, char *pkgname)
{
  printf("----------codegen------------\n");
  KImage *image = KImage_New(pkgname);
  struct genkimageinfo info = {image, NULL};
  STable_Visit(stbl, __gen_image_func, &info);
  KImage_Finish(image);
  printf("----------codegen end--------\n");
  KImage_Show(image);
  return image;
}

static void __symbol_show_fn(Symbol *sym, void *arg)
{
  UNUSED_PARAMETER(arg);
  assert(sym->kind >= SYM_CONST && sym->kind <= SYM_IMPORT);
  struct symbol_operations *ops = &symops[sym->kind];
  ops->__symbol_show(sym);
}

void STable_Show(SymbolTable *stbl)
{
  STable_Visit(stbl, __symbol_show_fn, stbl);
}

struct visit_entry {
  symbol_visit_func fn;
  void *arg;
};

static void __symbol_visit_fn(HashNode *hnode, void *arg)
{
  struct visit_entry *entry = arg;
  Symbol *sym = container_of(hnode, Symbol, hnode);
  entry->fn(sym, entry->arg);
}

void STable_Visit(SymbolTable *stbl, symbol_visit_func fn, void *arg)
{
  struct visit_entry entry = {fn, arg};
  HashTable_Visit(&stbl->table, __symbol_visit_fn, &entry);
}
