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

LOGGER(0)

static void *__symbol_new(SymKind kind, char *name, int size)
{
  Symbol *sym = Malloc(size);
  sym->name = name;
  sym->kind = kind;
  Init_HashNode(&sym->hnode, sym);
  sym->refcnt = 1;
  return sym;
}

static void __symbol_free(Symbol *sym)
{
  Mfree(sym);
}

struct gen_image_s {
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
  TYPE_DECREF(varSym->desc);
  __symbol_free(sym);
}

static void __const_show(Symbol *sym)
{
  VarSymbol *varSym = (VarSymbol *)sym;
  Log_Printf("const %s ", varSym->name);
  if (varSym->desc != NULL) {
    char buf[64];
    TypeDesc_ToString(varSym->desc, buf);
    Log_Printf("%s;\n", buf); /* with newline */
  } else {
    Log_Puts("<undefined-type>;"); /* with newline */
  }
}

static void __const_gen(Symbol *sym, void *arg)
{
  struct gen_image_s *info = arg;
  assert(info->classname == NULL);
  VarSymbol *varSym = (VarSymbol *)sym;
  __const_show(sym);
  KImage_Add_Var(info->image, varSym->name, varSym->desc, 1);
}

static Symbol *__var_new(char *name)
{
  return __symbol_new(SYM_VAR, name, sizeof(VarSymbol));
}

static void __var_free(Symbol *sym)
{
  VarSymbol *varSym = (VarSymbol *)sym;
  TYPE_DECREF(varSym->desc);
  __symbol_free(sym);
}

static void __var_show(Symbol *sym)
{
  VarSymbol *varSym = (VarSymbol *)sym;
  Log_Printf("var %s ", varSym->name);
  if (varSym->desc != NULL) {
    char buf[64];
    TypeDesc_ToString(varSym->desc, buf);
    Log_Printf("%s;\n", buf); /* with newline */
  } else {
    Log_Printf("<undefined-type>;\n");
  }
}

static void __var_gen(Symbol *sym, void *arg)
{
  struct gen_image_s *info = arg;
  VarSymbol *varSym = (VarSymbol *)sym;
  if (info->classname != NULL) {
    KImage_Add_Field(info->image, info->classname, varSym->name, varSym->desc);
  } else {
    KImage_Add_Var(info->image, varSym->name, varSym->desc, 0);
  }
}

static Symbol *__func_new(char *name)
{
  return __symbol_new(SYM_FUNC, name, sizeof(FuncSymbol));
}

static void __symbol_free_fn(Symbol *sym, void *arg);

static void __func_free(Symbol *sym)
{
  FuncSymbol *funcSym = (FuncSymbol *)sym;
  TYPE_DECREF(funcSym->desc);
  Vector_Fini(&funcSym->locvec, (vec_finifunc)__symbol_free_fn, NULL);
  CodeBlock_Free(funcSym->code);
  __symbol_free(sym);
}

static void __func_show(Symbol *sym)
{
  FuncSymbol *funcSym = (FuncSymbol *)sym;
  ProtoDesc *proto = (ProtoDesc *)funcSym->desc;
  char buf[64];
  ProtoVec_ToString(proto->arg, buf);
  Log_Printf("func %s(%s)", sym->name, buf);
  int sz = Vector_Size(proto->ret);
  if (sz > 0) {
    ProtoVec_ToString(proto->ret, buf);
    if (sz > 1)
      Log_Printf(" (%s);\n", buf);
    else
      Log_Printf(" %s;\n", buf);
  } else {
    Log_Puts(";"); /* with newline */
  }
}

static void __add_locvar(KImage *image, int index, Vector *vec, char *fmt)
{
  VarSymbol *varSym;
  Vector_ForEach(varSym, vec) {
    assert(varSym->kind == SYM_VAR);
    Log_Printf(fmt, varSym->name);
    KImage_Add_LocVar(image, varSym->name, varSym->desc, varSym->index, index);
  }
}

static void __func_gen(Symbol *sym, void *arg)
{
  struct gen_image_s *info = arg;
  FuncSymbol *funcSym = (FuncSymbol *)sym;
  uint8 *code = NULL;
  int size = CodeBlock_To_RawCode(info->image, funcSym->code, &code);
  int locvars = Vector_Size(&funcSym->locvec);
  int index;

  if (info->classname != NULL) {
    Log_Printf("  func %s:\n", funcSym->name);
    index = KImage_Add_Method(info->image, info->classname, funcSym->name,
                              funcSym->desc, code, size);
    if (locvars <= 0) {
      Log_Printf("    no vars\n");
    } else {
      char *fmt = "    var '%s'\n";
      __add_locvar(info->image, index, &funcSym->locvec, fmt);
    }
  } else {
    Log_Printf("func %s:\n", funcSym->name);
    index = KImage_Add_Func(info->image, funcSym->name,
                            funcSym->desc, code, size);
    if (locvars <= 0) {
      Log_Printf("  no vars\n");
    } else {
      char *fmt = "  var '%s'\n";
      __add_locvar(info->image, index, &funcSym->locvec, fmt);
    }
  }
}

static Symbol *__alias_new(char *name)
{
  return __symbol_new(SYM_ALIAS, name, sizeof(Symbol));
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
  ClassSymbol *clsSym = (ClassSymbol *)sym;
  /* FIXME */
  Vector_Fini(&clsSym->supers, NULL, NULL);
  STable_Free(clsSym->stbl);
  __symbol_free(sym);
}

static void __class_show(Symbol *sym)
{
  Log_Printf("class %s;\n", sym->name);
}

static void __gen_image_func(Symbol *sym, void *arg);

static void __class_gen(Symbol *sym, void *arg)
{
  struct gen_image_s *info = arg;
  ClassSymbol *clsSym = (ClassSymbol *)sym;
  Log_Debug("class %s:", clsSym->name);

  KImage_Add_Class(info->image, clsSym->name, &clsSym->supers);

  struct gen_image_s info2 = {info->image, clsSym->name};
  STable_Visit(clsSym->stbl, __gen_image_func, &info2);
}

static Symbol *__trait_new(char *name)
{
  return __symbol_new(SYM_TRAIT, name, sizeof(ClassSymbol));
}

static void __trait_free(Symbol *sym)
{
  ClassSymbol *clsSym = (ClassSymbol *)sym;
  /* FIXME */
  Vector_Fini(&clsSym->supers, NULL, NULL);
  STable_Free(clsSym->stbl);
  __symbol_free(sym);
}

static void __trait_show(Symbol *sym)
{
  Log_Printf("trait %s;\n", sym->name);
}

static void __trait_gen(Symbol *sym, void *arg)
{
  struct gen_image_s *info = arg;
  ClassSymbol *clsSym = (ClassSymbol *)sym;
  Log_Debug("trait %s:", clsSym->name);

  KImage_Add_Trait(info->image, clsSym->name, &clsSym->supers);

  struct gen_image_s info2 = {info->image, clsSym->name};
  STable_Visit(clsSym->stbl, __gen_image_func, &info2);
}

static Symbol *__ifunc_new(char *name)
{
  return __symbol_new(SYM_IFUNC, name, sizeof(Symbol));
}

static void __ifunc_free(Symbol *sym)
{
  TYPE_DECREF(sym->desc);
  __symbol_free(sym);
}

static void __ifunc_show(Symbol *sym)
{
  ProtoDesc *proto = (ProtoDesc *)sym->desc;
  char buf[64];
  ProtoVec_ToString(proto->arg, buf);
  if (sym->kind == SYM_IFUNC)
    Log_Printf("iterface func %s%s", sym->name, buf);
  else
    Log_Printf("native func %s(%s)", sym->name, buf);
  int sz = Vector_Size(proto->ret);
  if (sz > 0) {
    ProtoVec_ToString(proto->ret, buf);
    if (sz > 1)
      Log_Printf(" (%s);\n", buf);
    else
      Log_Printf(" %s;\n", buf);
  } else {
    Log_Puts(";"); /* with newline */
  }
}

static void __ifunc_gen(Symbol *sym, void *arg)
{
  struct gen_image_s *info = arg;
  if (sym->kind == SYM_IFUNC) {
    Log_Debug("  iterface func %s;", sym->name);
    KImage_Add_IMeth(info->image, info->classname, sym->name, sym->desc);
  } else {
    Log_Debug("  native func %s", sym->name);
    KImage_Add_NFunc(info->image, info->classname, sym->name, sym->desc);
  }
}

static Symbol *__nfunc_new(char *name)
{
  return __symbol_new(SYM_NFUNC, name, sizeof(Symbol));
}

static Symbol *__afunc_new(char *name)
{
  return __symbol_new(SYM_AFUNC, name, sizeof(AFuncSymbol));
}

static void __afunc_free(Symbol *sym)
{
  AFuncSymbol *afnSym = (AFuncSymbol *)sym;
  TYPE_DECREF(afnSym->desc);
  Vector_Fini(&afnSym->locvec, (vec_finifunc)__symbol_free_fn, NULL);
  Vector_Fini(&afnSym->uplocvec, (vec_finifunc)__symbol_free_fn, NULL);
  CodeBlock_Free(afnSym->code);
  __symbol_free(sym);
}

static void __afunc_show(Symbol *sym)
{
  AFuncSymbol *afnSym = (AFuncSymbol *)sym;
  ProtoDesc *proto = (ProtoDesc *)afnSym->desc;
  char buf[64];
  ProtoVec_ToString(proto->arg, buf);
  Log_Printf("anonymous func %s%s", sym->name, buf);
  int sz = Vector_Size(proto->ret);
  if (sz > 0) {
    ProtoVec_ToString(proto->ret, buf);
    if (sz > 1)
      Log_Printf(" (%s);\n", buf);
    else
      Log_Printf(" %s;\n", buf);
  } else {
    Log_Puts(";"); /* with newline */
  }
}

static void __afunc_gen(Symbol *sym, void *arg)
{
  struct gen_image_s *info = arg;
  AFuncSymbol *afnSym = (AFuncSymbol *)sym;
  Log_Debug("  anonymous func %s", afnSym->name);
  KImage_Add_IMeth(info->image, info->classname, afnSym->name, afnSym->desc);
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
  {__nfunc_new, __ifunc_free, __ifunc_show, __ifunc_gen}, /* SYM_NFUNC  */
  {__afunc_new, __afunc_free, __afunc_show, __afunc_gen}, /* SYM_AFUNC  */
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
  if (--sym->refcnt <= 0) {
    assert(sym->refcnt >= 0);
    struct symbol_operations *ops = &symops[sym->kind];
    ops->__symbol_free(sym);
  }
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
  Remove_HashNode(&sym->hnode);
  Symbol_Free(sym);
}

STable *STable_New(void)
{
  STable *stbl = Malloc(sizeof(STable));
  HashTable_Init(&stbl->table, symbol_hash, symbol_equal);
  /* [0]: module or class self */
  stbl->varindex = 1;
  return stbl;
}

void __STable_Free(STable *stbl, symbol_visit_func visit, void *arg)
{
  if (stbl == NULL)
    return;

  if (visit == NULL) {
    visit = __symbol_free_fn;
    arg = NULL;
  }

  STable_Visit(stbl, visit, arg);
  HashTable_Fini(&stbl->table, NULL, NULL);
  Mfree(stbl);
}

Symbol *STable_Add_Const(STable *stbl, char *name, TypeDesc *desc)
{
  VarSymbol *sym = (VarSymbol *)Symbol_New(SYM_CONST, name);
  if (HashTable_Insert(&stbl->table, &sym->hnode) < 0) {
    Symbol_Free((Symbol *)sym);
    return NULL;
  }
  TYPE_INCREF(desc);
  sym->desc = desc;
  sym->index = stbl->varindex++;
  return (Symbol *)sym;
}

Symbol *STable_Add_Var(STable *stbl, char *name, TypeDesc *desc)
{
  VarSymbol *sym = (VarSymbol *)Symbol_New(SYM_VAR, name);
  if (HashTable_Insert(&stbl->table, &sym->hnode) < 0) {
    Symbol_Free((Symbol *)sym);
    return NULL;
  }
  TYPE_INCREF(desc);
  sym->desc = desc;
  sym->index = stbl->varindex++;
  return (Symbol *)sym;
}

Symbol *STable_Add_Func(STable *stbl, char *name, TypeDesc *proto)
{
  FuncSymbol *sym = (FuncSymbol *)Symbol_New(SYM_FUNC, name);
  if (HashTable_Insert(&stbl->table, &sym->hnode) < 0) {
    Symbol_Free((Symbol *)sym);
    return NULL;
  }
  Vector_Init(&sym->locvec);
  TYPE_INCREF(proto);
  sym->desc = proto;
  return (Symbol *)sym;
}

Symbol *STable_Add_Alias(STable *stbl, char *name, TypeDesc *desc)
{
  Symbol *sym = Symbol_New(SYM_ALIAS, name);
  if (HashTable_Insert(&stbl->table, &sym->hnode) < 0) {
    Symbol_Free(sym);
    return NULL;
  }
  TYPE_INCREF(desc);
  sym->desc = desc;
  return sym;
}

Symbol *STable_Add_Class(STable *stbl, char *name)
{
  ClassSymbol *sym = (ClassSymbol *)Symbol_New(SYM_CLASS, name);
  if (HashTable_Insert(&stbl->table, &sym->hnode) < 0) {
    Symbol_Free((Symbol *)sym);
    return NULL;
  }
  Vector_Init(&sym->supers);
  sym->stbl = STable_New();
  return (Symbol *)sym;
}

Symbol *STable_Add_Trait(STable *stbl, char *name)
{
  ClassSymbol *sym = (ClassSymbol *)Symbol_New(SYM_TRAIT, name);
  if (HashTable_Insert(&stbl->table, &sym->hnode) < 0) {
    Symbol_Free((Symbol *)sym);
    return NULL;
  }
  Vector_Init(&sym->supers);
  sym->stbl = STable_New();
  return (Symbol *)sym;
}

Symbol *STable_Add_Proto(STable *stbl, char *name, int k, TypeDesc *desc)
{
  Symbol *sym = Symbol_New(k, name);
  if (HashTable_Insert(&stbl->table, &sym->hnode) < 0) {
    Symbol_Free(sym);
    return NULL;
  }
  TYPE_INCREF(desc);
  sym->desc = desc;
  return sym;
}

#define ANONY_PREFIX "__anonoy_"
static uint32 anony_number = 0xbeaf;

Symbol *STable_Add_Anonymous(STable *stbl, TypeDesc *desc)
{
  char name[32];
  snprintf(name, 31, ANONY_PREFIX "%u", ++anony_number);
  AFuncSymbol *sym = (AFuncSymbol *)Symbol_New(SYM_AFUNC, name);
  if (HashTable_Insert(&stbl->table, &sym->hnode) < 0) {
    Symbol_Free((Symbol *)sym);
    return NULL;
  }
  TYPE_INCREF(desc);
  sym->desc = desc;
  Vector_Init(&sym->locvec);
  Vector_Init(&sym->uplocvec);
  return (Symbol *)sym;
}

Symbol *STable_Add_Import(STable *stbl, char *name)
{
  ImportSymbol *sym = (ImportSymbol *)Symbol_New(SYM_IMPORT, name);
  if (HashTable_Insert(&stbl->table, &sym->hnode) < 0) {
    Symbol_Free((Symbol *)sym);
    return NULL;
  }
  return (Symbol *)sym;
}

Symbol *STable_Get(STable *stbl, char *name)
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

KImage *Gen_Image(STable *stbl, char *pkgname)
{
  Log_Debug("\x1b[34m----STARTING IMAGE----\x1b[0m");
  KImage *image = KImage_New(pkgname);
  struct gen_image_s info = {image, NULL};
  STable_Visit(stbl, __gen_image_func, &info);
  KImage_Finish(image);
  KImage_Show(image);
  Log_Debug("\x1b[34m----END OF IMAGE------\x1b[0m");
  return image;
}

static void __symbol_show_fn(Symbol *sym, void *arg)
{
  UNUSED_PARAMETER(arg);
  assert(sym->kind >= SYM_CONST && sym->kind <= SYM_IMPORT);
  struct symbol_operations *ops = &symops[sym->kind];
  ops->__symbol_show(sym);
}

void Show_Symbol(Symbol *sym)
{
  __symbol_show_fn(sym, NULL);
}

void STable_Show(STable *stbl)
{
  STable_Visit(stbl, __symbol_show_fn, NULL);
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

void STable_Visit(STable *stbl, symbol_visit_func fn, void *arg)
{
  struct visit_entry entry = {fn, arg};
  HashTable_Visit(&stbl->table, __symbol_visit_fn, &entry);
}
