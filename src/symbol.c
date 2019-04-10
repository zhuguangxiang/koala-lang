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

static inline void __symbol_free(Symbol *sym)
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
  Log_Debug("const '%s' is freed", sym->name);
  VarSymbol *varSym = (VarSymbol *)sym;
  TYPE_DECREF(varSym->desc);
  __symbol_free(sym);
}

static void __const_show(Symbol *sym)
{
  VarSymbol *varSym = (VarSymbol *)sym;
  Log_Printf("const %s ", varSym->name);
  if (varSym->desc != NULL) {
    String s = TypeDesc_ToString(varSym->desc);
    Log_Printf("%s;\n", s.str); /* with newline */
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
  Log_Debug("var '%s' is freed", sym->name);
  VarSymbol *varSym = (VarSymbol *)sym;
  TYPE_DECREF(varSym->desc);
  __symbol_free(sym);
}

static void __var_show(Symbol *sym)
{
  VarSymbol *varSym = (VarSymbol *)sym;
  Log_Printf("var %s ", varSym->name);
  if (varSym->desc != NULL) {
    String s = TypeDesc_ToString(varSym->desc);
    Log_Printf("%s;\n", s.str); /* with newline */
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

static void __locvar_free_fn(void *item, void *arg)
{
  UNUSED_PARAMETER(arg);
  Symbol_Free(item);
}

static void __func_free(Symbol *sym)
{
  Log_Debug("func '%s' is freed", sym->name);
  FuncSymbol *funcSym = (FuncSymbol *)sym;
  TYPE_DECREF(funcSym->desc);
  Vector_Fini(&funcSym->locvec, __locvar_free_fn, NULL);
  CodeBlock_Free(funcSym->code);
  __symbol_free(sym);
}

static void __func_show(Symbol *sym)
{
  FuncSymbol *funcSym = (FuncSymbol *)sym;
  ProtoDesc *proto = (ProtoDesc *)funcSym->desc;
  DeclareStringBuf(buf);
  Proto_Print(proto, &buf);
  Log_Printf("func %s%s;\n", sym->name, buf.data);
  FiniStringBuf(buf);
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

static Symbol *__class_new(char *name)
{
  return __symbol_new(SYM_CLASS, name, sizeof(ClassSymbol));
}

static void __class_free(Symbol *sym)
{
  Log_Debug("class/trait '%s' is freed", sym->name);
  ClassSymbol *clsSym = (ClassSymbol *)sym;
  TYPE_DECREF(clsSym->desc);
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
  Log_Debug("ifunc/nfunc '%s' is freed", sym->name);
  TYPE_DECREF(sym->desc);
  __symbol_free(sym);
}

static void __ifunc_show(Symbol *sym)
{
  ProtoDesc *proto = (ProtoDesc *)sym->desc;
  DeclareStringBuf(buf);
  Proto_Print(proto, &buf);
  if (sym->kind == SYM_IFUNC)
    Log_Printf("interface func %s%s;\n", sym->name, buf.data);
  else
    Log_Printf("native func %s%s;\n", sym->name, buf.data);
  FiniStringBuf(buf);
}

static void __ifunc_gen(Symbol *sym, void *arg)
{
  struct gen_image_s *info = arg;
  if (sym->kind == SYM_IFUNC) {
    Log_Debug("  interface func %s;", sym->name);
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
  Log_Debug("anony '%s' is freed", sym->name);
  AFuncSymbol *afnSym = (AFuncSymbol *)sym;
  TYPE_DECREF(afnSym->desc);
  Vector_Fini(&afnSym->locvec, __locvar_free_fn, NULL);
  Vector_Fini(&afnSym->uplocvec, __locvar_free_fn, NULL);
  CodeBlock_Free(afnSym->code);
  __symbol_free(sym);
}

static void __afunc_show(Symbol *sym)
{
  AFuncSymbol *afnSym = (AFuncSymbol *)sym;
  ProtoDesc *proto = (ProtoDesc *)afnSym->desc;
  DeclareStringBuf(buf);
  Proto_Print(proto, &buf);
  Log_Printf("anonymous func %s%s;\n", sym->name, buf.data);
  FiniStringBuf(buf);
}

static void __afunc_gen(Symbol *sym, void *arg)
{
  struct gen_image_s *info = arg;
  AFuncSymbol *afnSym = (AFuncSymbol *)sym;
  Log_Debug("  anonymous func %s", afnSym->name);
  KImage_Add_IMeth(info->image, info->classname, afnSym->name, afnSym->desc);
}

static Symbol *__pkg_new(char *name)
{
  return __symbol_new(SYM_PKG, name, sizeof(PkgSymbol));
}

static void __pkg_free(Symbol *sym)
{
  Log_Debug("pkg '%s' is freed", sym->name);
  TYPE_DECREF(sym->desc);
  __symbol_free(sym);
}

static Symbol *__ref_new(char *name)
{
  return __symbol_new(SYM_REF, name, sizeof(RefSymbol));
}

static void __ref_free(Symbol *sym)
{
  Log_Debug("ref '%s' is freed", sym->name);
  TYPE_DECREF(sym->desc);
  __symbol_free(sym);
}

struct symbol_operations {
  Symbol *(*__symbol_new)(char *name);
  void (*__symbol_free)(Symbol *sym);
  void (*__symbol_show)(Symbol *sym);
  void (*__symbol_gen)(Symbol *sym, void *arg);
} symops[] = {
  {NULL, NULL, NULL, NULL},                                   /* INVALID   */
  {__const_new, __const_free, __const_show, __const_gen},     /* SYM_CONST */
  {__var_new, __var_free, __var_show, __var_gen},             /* SYM_VAR   */
  {__func_new, __func_free, __func_show, __func_gen},         /* SYM_FUNC  */
  {__class_new, __class_free, __class_show, __class_gen},     /* SYM_CLASS */
  {__trait_new, __class_free, __trait_show, __trait_gen},     /* SYM_TRAIT */
  {__ifunc_new, __ifunc_free, __ifunc_show, __ifunc_gen},     /* SYM_IFUNC */
  {__nfunc_new, __ifunc_free, __ifunc_show, __ifunc_gen},     /* SYM_NFUNC */
  {__afunc_new, __afunc_free, __afunc_show, __afunc_gen},     /* SYM_AFUNC */
  {__pkg_new,   __pkg_free, NULL, NULL},                      /* SYM_PKG   */
  {__ref_new,   __ref_free, NULL, NULL}                       /* SYM_REF   */
};

Symbol *Symbol_New(SymKind kind, char *name)
{
  assert(kind > 0 && kind < SYM_MAX);
  struct symbol_operations *ops = &symops[kind];
  return ops->__symbol_new(name);
}

void Symbol_Free(Symbol *sym)
{
  assert(sym->kind > 0 && sym->kind < SYM_MAX);
  if (--sym->refcnt <= 0) {
    assert(sym->refcnt >= 0);
    struct symbol_operations *ops = &symops[sym->kind];
    ops->__symbol_free(sym);
  } else {
    Log_Debug("sym '%s' refcnt %d", sym->name, sym->refcnt);
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

STable *STable_New(void)
{
  STable *stbl = Malloc(sizeof(STable));
  HashTable_Init(&stbl->table, symbol_hash, symbol_equal);
  /* [0]: module or class self */
  stbl->varindex = 1;
  return stbl;
}

static void __symbol_free_fn(Symbol *sym, void *arg)
{
  Log_Debug("remove sym '%s' from stbl", sym->name);
  UNUSED_PARAMETER(arg);
  Remove_HashNode(&sym->hnode);
  Symbol_Free(sym);
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

VarSymbol *STable_Add_Const(STable *stbl, char *name, TypeDesc *desc)
{
  VarSymbol *sym = (VarSymbol *)Symbol_New(SYM_CONST, name);
  if (HashTable_Insert(&stbl->table, &sym->hnode) < 0) {
    Symbol_Free((Symbol *)sym);
    return NULL;
  }
  TYPE_INCREF(desc);
  sym->desc = desc;
  sym->index = stbl->varindex++;
  return sym;
}

VarSymbol *STable_Add_Var(STable *stbl, char *name, TypeDesc *desc)
{
  VarSymbol *sym = (VarSymbol *)Symbol_New(SYM_VAR, name);
  if (HashTable_Insert(&stbl->table, &sym->hnode) < 0) {
    Symbol_Free((Symbol *)sym);
    return NULL;
  }
  TYPE_INCREF(desc);
  sym->desc = desc;
  sym->index = stbl->varindex++;
  return sym;
}

FuncSymbol *STable_Add_Func(STable *stbl, char *name, TypeDesc *proto)
{
  FuncSymbol *sym = (FuncSymbol *)Symbol_New(SYM_FUNC, name);
  if (HashTable_Insert(&stbl->table, &sym->hnode) < 0) {
    Symbol_Free((Symbol *)sym);
    return NULL;
  }
  Vector_Init(&sym->locvec);
  TYPE_INCREF(proto);
  sym->desc = proto;
  return sym;
}

ClassSymbol *STable_Add_Class(STable *stbl, char *name)
{
  ClassSymbol *sym = (ClassSymbol *)Symbol_New(SYM_CLASS, name);
  if (HashTable_Insert(&stbl->table, &sym->hnode) < 0) {
    Symbol_Free((Symbol *)sym);
    return NULL;
  }
  Vector_Init(&sym->supers);
  sym->stbl = STable_New();
  sym->desc = TypeDesc_New_Klass(NULL, name, NULL);
  TYPE_INCREF(sym->desc);
  return sym;
}

ClassSymbol *STable_Add_Trait(STable *stbl, char *name)
{
  ClassSymbol *sym = (ClassSymbol *)Symbol_New(SYM_TRAIT, name);
  if (HashTable_Insert(&stbl->table, &sym->hnode) < 0) {
    Symbol_Free((Symbol *)sym);
    return NULL;
  }
  Vector_Init(&sym->supers);
  sym->stbl = STable_New();
  sym->desc = TypeDesc_New_Klass(NULL, name, NULL);
  TYPE_INCREF(sym->desc);
  return sym;
}

Symbol *STable_Add_Proto(STable *stbl, char *name, int kind, TypeDesc *desc)
{
  Symbol *sym = Symbol_New(kind, name);
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

AFuncSymbol *STable_Add_Anonymous(STable *stbl, TypeDesc *desc)
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
  return sym;
}

PkgSymbol *STable_Add_Package(STable *stbl, char *name)
{
  PkgSymbol *sym = (PkgSymbol *)Symbol_New(SYM_PKG, name);
  if (HashTable_Insert(&stbl->table, &sym->hnode) < 0) {
    Symbol_Free((Symbol *)sym);
    return NULL;
  }
  return sym;
}

RefSymbol *STable_Add_Reference(STable *stbl, char *name)
{
  RefSymbol *sym = (RefSymbol *)Symbol_New(SYM_REF, name);
  if (HashTable_Insert(&stbl->table, &sym->hnode) < 0) {
    Symbol_Free((Symbol *)sym);
    return NULL;
  }
  return sym;
}

Symbol *STable_Get(STable *stbl, char *name)
{
  if (stbl == NULL)
    return NULL;
  Symbol key = {.name = name};
  HashNode *hnode = HashTable_Find(&stbl->table, &key);
  return hnode ? container_of(hnode, Symbol, hnode) : NULL;
}

static void __gen_image_func(Symbol *sym, void *arg)
{
  assert(sym->kind > 0 && sym->kind < SYM_MAX);
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

#define IsAccess(name) isupper(name[0])

static void __get_var_fn(char *name, TypeDesc *desc, int k, STable *stbl)
{
  if (!IsAccess(name)) {
    Log_Debug("'%s' is private", name);
    return;
  }

  String sname = AtomString_New(name);
  if (k) {
    Log_Debug("load const '%s'", name);
    STable_Add_Const(stbl, sname.str, desc);
  } else {
    Log_Debug("load var '%s'", name);
    STable_Add_Var(stbl, sname.str, desc);
  }
}

static void __get_func_fn(char *name, TypeDesc *desc, int idx, int type,
                          uint8 *code, int sz, STable *stbl)
{
#if 0
  if (!IsAccess(name)) {
    Log_Debug("'%s' is private", name);
    return;
  }
#endif
  String sname = AtomString_New(name);
  if (type == ITEM_FUNC) {
    Log_Debug("load func '%s'", name);
    STable_Add_Func(stbl, sname.str, desc);
  } else if (type == ITEM_NFUNC) {
    Log_Debug("load native func '%s'", name);
    STable_Add_NFunc(stbl, sname.str, desc);
  } else {
    assert(0);
  }
}

#define NOT_LOAD_ITEMS \
  ((1 << ITEM_CODE) | (1 << ITEM_LOCVAR) | (1 << ITEM_CONST))

int STable_From_Image(char *path, char **pkgname, STable **stbl)
{
  KImage *image = KImage_Read_File(path, NOT_LOAD_ITEMS);

  if (image == NULL) {
    *stbl = NULL;
    *pkgname = NULL;
    return -1;
  }

  STable *table = STable_New();
  KImage_Get_Vars(image, (getvarfn)__get_var_fn, table);
  KImage_Get_Funcs(image, (getfuncfn)__get_func_fn, table);
  KImage_Get_NFuncs(image, (getfuncfn)__get_func_fn, table);

  *pkgname = AtomString_New(KImage_Get_PkgName(image)).str;
  *stbl = table;
  KImage_Free(image);

  return 0;
}

static void __symbol_show_fn(Symbol *sym, void *arg)
{
  UNUSED_PARAMETER(arg);
  assert(sym->kind > 0 && sym->kind < SYM_MAX);
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
