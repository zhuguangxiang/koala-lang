/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "symbol.h"
#include "codegen.h"
#include "memory.h"
#include "log.h"

static inline void *symbol_new(SymKind kind, char *name, int size)
{
  Symbol *sym = kmalloc(size);
  sym->name = name;
  sym->kind = kind;
  hashmap_entry_init(sym, strhash(name));
  sym->refcnt = 1;
  return sym;
}

static inline void symbol_free(Symbol *sym)
{
  panic(sym->refcnt, "refcnt of symbol '%s' is not 0", sym->name);
  kfree(sym);
}

typedef struct {
  Image *image;
  char *clazz;
} GenImage;

static Symbol *const_new(char *name)
{
  return symbol_new(SYM_CONST, name, sizeof(VarSymbol));
}

static void const_free(Symbol *sym)
{
  debug("const '%s' is freed", sym->name);
  VarSymbol *var = (VarSymbol *)sym;
  TYPE_DECREF(var->desc);
  symbol_free(sym);
}

static void const_show(Symbol *sym)
{
  VarSymbol *var = (VarSymbol *)sym;
  printf("const %s ", var->name);
  typedesc_show(var->desc);
}

static void const_gen(Symbol *sym, void *arg)
{
  GenImage *gen = arg;
  VarSymbol *var = (VarSymbol *)sym;
  const_show(sym);
  Image_Add_Const(gen->image, var->name, var->desc, &var->value);
}

static Symbol *var_new(char *name)
{
  return symbol_new(SYM_VAR, name, sizeof(VarSymbol));
}

static void var_free(Symbol *sym)
{
  debug("var '%s' is freed", sym->name);
  VarSymbol *var = (VarSymbol *)sym;
  TYPE_DECREF(var->desc);
  symbol_free(sym);
}

static void var_show(Symbol *sym)
{
  VarSymbol *var = (VarSymbol *)sym;
  printf("var %s ", var->name);
  typedesc_show(var->desc);
}

static void var_gen(Symbol *sym, void *arg)
{
  GenImage *gen = arg;
  VarSymbol *var = (VarSymbol *)sym;
  var_show(sym);
  if (gen->clazz != NULL) {
    Image_Add_Field(gen->image, gen->clazz, var->name, var->desc);
  } else {
    Image_Add_Var(gen->image, var->name, var->desc);
  }
}

static Symbol *func_new(char *name)
{
  return symbol_new(SYM_FUNC, name, sizeof(FuncSymbol));
}

static void _locvar_free_(void *item, void *arg)
{
  Symbol_DecRef(item);
}

static void func_free(Symbol *sym)
{
  debug("func '%s' is freed", sym->name);
  FuncSymbol *func = (FuncSymbol *)sym;
  TYPE_DECREF(func->desc);
  vector_fini(&func->locvec, _locvar_free_, NULL);
  CodeBlock_Free(func->code);
  symbol_free(sym);
}

static void func_show(Symbol *sym)
{
  FuncSymbol *func = (FuncSymbol *)sym;
  printf("func %s", sym->name);
  typedesc_show(func->desc);
}

static void add_locvar(Image *image, int index, Vector *vec, char *fmt)
{
  VarSymbol *var;
  VECTOR_ITERATOR(iter, vec);
  iter_for_each(&iter, var) {
    panic(var->kind != SYM_VAR, "symbol '%s' is not a variable", var->name);
    printf(fmt, var->name);
    Image_Add_LocVar(image, var->name, var->desc, var->index, index);
  }
}

static void func_gen(Symbol *sym, void *arg)
{
  GenImage *gen = arg;
  FuncSymbol *func = (FuncSymbol *)sym;
  uint8_t *code = NULL;
  int size = CodeBlock_To_RawCode(gen->image, func->code, &code);
  int locvars = vector_size(&func->locvec);
  int index;

  if (gen->clazz != NULL) {
    printf("  func %s:\n", func->name);
    index = Image_Add_Method(gen->image, gen->clazz, func->name,
                             func->desc, code, size, locvars);
    if (locvars <= 0) {
      puts("    no vars");
    } else {
      add_locvar(gen->image, index, &func->locvec, "    var '%s'\n");
    }
  } else {
    printf("func %s:\n", func->name);
    index = Image_Add_Func(gen->image, func->name,
                           func->desc, code, size, locvars);
    if (locvars <= 0) {
      puts("  no vars");
    } else {
      add_locvar(gen->image, index, &func->locvec, "  var '%s'\n");
    }
  }
}

#if 0
static Symbol *__class_new(char *name)
{
  return __symbol_new(SYM_CLASS, name, sizeof(ClassSymbol));
}

static void __class_free(Symbol *sym)
{
  debug("class/trait '%s' is freed", sym->name);
  ClassSymbol *clsSym = (ClassSymbol *)sym;
  TYPE_DECREF(clsSym->desc);
  /* FIXME */
  Vector_Fini(&clsSym->supers, NULL, NULL);
  STable_Free(clsSym->stbl);
  __symbol_free(sym);
}

static void __class_show(Symbol *sym)
{
  printf("class %s;\n", sym->name);
}

static void __gen_image_func(Symbol *sym, void *arg);

static void __class_gen(Symbol *sym, void *arg)
{
  struct gen_image_s *info = arg;
  ClassSymbol *clsSym = (ClassSymbol *)sym;
  printf("class %s:\n", clsSym->name);

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
  printf("trait %s;\n", sym->name);
}

static void __trait_gen(Symbol *sym, void *arg)
{
  struct gen_image_s *info = arg;
  ClassSymbol *clsSym = (ClassSymbol *)sym;
  printf("trait %s:\n", clsSym->name);

  KImage_Add_Trait(info->image, clsSym->name, &clsSym->supers);

  struct gen_image_s info2 = {info->image, clsSym->name};
  STable_Visit(clsSym->stbl, __gen_image_func, &info2);
}

static Symbol *__enum_new(char *name)
{
  return __symbol_new(SYM_ENUM, name, sizeof(EnumSymbol));
}

static void __enum_free(Symbol *sym)
{
  debug("enum '%s' is freed", sym->name);
  EnumSymbol *eSym = (EnumSymbol *)sym;
  TYPE_DECREF(eSym->desc);
  STable_Free(eSym->stbl);
  __symbol_free(sym);
}

static void __enum_show(Symbol *sym)
{
  printf("enum %s;\n", sym->name);
}

static void __enum_gen(Symbol *sym, void *arg)
{
  struct gen_image_s *info = arg;
  EnumSymbol *eSym = (EnumSymbol *)sym;
  printf("enum %s:\n", eSym->name);

  KImage_Add_Enum(info->image, eSym->name);

  struct gen_image_s info2 = {info->image, eSym->name};
  STable_Visit(eSym->stbl, __gen_image_func, &info2);
}

static Symbol *__eval_new(char *name)
{
  return __symbol_new(SYM_EVAL, name, sizeof(EnumValSymbol));
}

static void free_typedesc_func(void *item, void *arg)
{
  TYPE_DECREF(item);
}

static void __eval_free(Symbol *sym)
{
  debug("enum '%s' is freed", sym->name);
  EnumValSymbol *evSym = (EnumValSymbol *)sym;
  TYPE_DECREF(evSym->desc);
  Vector_Free(evSym->types, free_typedesc_func, NULL);
  __symbol_free(sym);
}

static void __eval_show(Symbol *sym)
{
  printf("  eval %s;\n", sym->name);
}

static void __eval_gen(Symbol *sym, void *arg)
{
  struct gen_image_s *info = arg;
  if (info->classname != NULL) {
    EnumValSymbol *evSym = (EnumValSymbol *)sym;
    __eval_show(sym);
    KImage_Add_EVal(info->image, info->classname, evSym->name,
                    evSym->types, 0);
  }
}

static Symbol *__ifunc_new(char *name)
{
  return __symbol_new(SYM_IFUNC, name, sizeof(Symbol));
}

static void __ifunc_free(Symbol *sym)
{
  debug("ifunc/nfunc '%s' is freed", sym->name);
  TYPE_DECREF(sym->desc);
  __symbol_free(sym);
}

static void __ifunc_show(Symbol *sym)
{
  ProtoDesc *proto = (ProtoDesc *)sym->desc;
  DeclareStringBuf(buf);
  Proto_Print(proto, &buf);
  if (sym->kind == SYM_IFUNC)
    printf("interface func %s%s;\n", sym->name, buf.data);
  else
    printf("native func %s%s;\n", sym->name, buf.data);
  FiniStringBuf(buf);
}

static void __ifunc_gen(Symbol *sym, void *arg)
{
  struct gen_image_s *info = arg;
  if (sym->kind == SYM_IFUNC) {
    printf("  interface func %s;\n", sym->name);
    KImage_Add_IMeth(info->image, info->classname, sym->name, sym->desc);
  } else {
    printf("  native func %s;\n", sym->name);
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
  debug("anony '%s' is freed", sym->name);
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
  printf("anonymous func %s%s;\n", sym->name, buf.data);
  FiniStringBuf(buf);
}

static void __afunc_gen(Symbol *sym, void *arg)
{
  struct gen_image_s *info = arg;
  AFuncSymbol *afnSym = (AFuncSymbol *)sym;
  printf("  anonymous func %s", afnSym->name);
  KImage_Add_IMeth(info->image, info->classname, afnSym->name, afnSym->desc);
}

static Symbol *__pkg_new(char *name)
{
  return __symbol_new(SYM_PKG, name, sizeof(PkgSymbol));
}

static void __pkg_free(Symbol *sym)
{
  debug("pkg '%s' is freed", sym->name);
  TYPE_DECREF(sym->desc);
  __symbol_free(sym);
}

static Symbol *__ref_new(char *name)
{
  return __symbol_new(SYM_REF, name, sizeof(RefSymbol));
}

static void __ref_free(Symbol *sym)
{
  debug("ref '%s' is freed", sym->name);
  TYPE_DECREF(sym->desc);
  __symbol_free(sym);
}
#endif

struct symbolops {
  Symbol *(*symbol_new)(char *name);
  void (*symbol_free)(Symbol *sym);
  void (*symbol_show)(Symbol *sym);
  void (*symbol_gen)(Symbol *sym, void *arg);
} symops[] = {
  {NULL, NULL, NULL, NULL},                           /* INVALID   */
  {const_new, const_free, const_show, const_gen},     /* SYM_CONST */
  {var_new,   var_free,   var_show,   var_gen  },     /* SYM_VAR   */
  {func_new,  func_free,  func_show,  func_gen },     /* SYM_FUNC  */
  #if 0
  {__class_new, __class_free, __class_show, __class_gen},     /* SYM_CLASS */
  {__trait_new, __class_free, __trait_show, __trait_gen},     /* SYM_TRAIT */
  {__enum_new,  __enum_free,  __enum_show,  __enum_gen },     /* SYM_ENUM  */
  {__eval_new,  __eval_free,  __eval_show,  __eval_gen },     /* SYM_EVAL  */
  {__ifunc_new, __ifunc_free, __ifunc_show, __ifunc_gen},     /* SYM_IFUNC */
  {__nfunc_new, __ifunc_free, __ifunc_show, __ifunc_gen},     /* SYM_NFUNC */
  {__afunc_new, __afunc_free, __afunc_show, __afunc_gen},     /* SYM_AFUNC */
  {__pkg_new,   __pkg_free, NULL, NULL},                      /* SYM_PKG   */
  {__ref_new,   __ref_free, NULL, NULL}                       /* SYM_REF   */
  #endif
};

Symbol *Symbol_New(SymKind kind, char *name)
{
  panic(kind <= 0 || kind >= SYM_MAX, "invalid symbol kind '%d'", kind);
  struct symbolops *ops = symops + kind;
  return ops->symbol_new(name);
}

void Symbol_DecRef(Symbol *sym)
{
  panic(sym->kind <= 0 || sym->kind >= SYM_MAX,
        "invalid symbol kind '%d'", sym->kind);
  if (--sym->refcnt <= 0) {
    panic(sym->refcnt < 0, "symbol refcnt %d < 0", sym->refcnt);
    struct symbolops *ops = symops + sym->kind;
    ops->symbol_free(sym);
  } else {
    debug("refcnt of symbol '%s' is %d", sym->name, sym->refcnt);
  }
}

static int symbol_equal(void *k1, void *k2)
{
  Symbol *s1 = k1;
  Symbol *s2 = k2;
  return !strcmp(s1->name, s2->name);
}

STable *STable_New(void)
{
  STable *stbl = kmalloc(sizeof(STable));
  hashmap_init(&stbl->table, symbol_equal);
  /* [0]: module or class self */
  stbl->varindex = 1;
  return stbl;
}

static void _symbol_free_(void *e, void *arg)
{
  Symbol *sym = e;
  debug("remove symbol '%s'", sym->name);
  Symbol_DecRef(sym);
}

void STable_Free(STable *stbl)
{
  if (stbl == NULL)
    return;

  hashmap_fini(&stbl->table, _symbol_free_, NULL);
  kfree(stbl);
}

VarSymbol *STable_Add_Const(STable *stbl, char *name, TypeDesc *desc)
{
  VarSymbol *sym = (VarSymbol *)Symbol_New(SYM_CONST, name);
  if (hashmap_add(&stbl->table, sym) < 0) {
    Symbol_DecRef((Symbol *)sym);
    return NULL;
  }
  sym->desc = TYPE_INCREF(desc);
  sym->index = stbl->varindex++;
  return sym;
}

VarSymbol *STable_Add_Var(STable *stbl, char *name, TypeDesc *desc)
{
  VarSymbol *sym = (VarSymbol *)Symbol_New(SYM_VAR, name);
  if (hashmap_add(&stbl->table, sym) < 0) {
    Symbol_DecRef((Symbol *)sym);
    return NULL;
  }
  sym->desc = TYPE_INCREF(desc);
  sym->index = stbl->varindex++;
  return sym;
}

FuncSymbol *STable_Add_Func(STable *stbl, char *name, TypeDesc *proto)
{
  FuncSymbol *sym = (FuncSymbol *)Symbol_New(SYM_FUNC, name);
  if (hashmap_add(&stbl->table, sym) < 0) {
    Symbol_DecRef((Symbol *)sym);
    return NULL;
  }
  sym->desc = TYPE_INCREF(proto);
  return sym;
}

/*
ClassSymbol *STable_Add_Class(STable *stbl, char *name)
{
  ClassSymbol *sym = (ClassSymbol *)Symbol_New(SYM_CLASS, name);
  if (HashTable_Insert(&stbl->table, &sym->hnode) < 0) {
    Symbol_Free((Symbol *)sym);
    return NULL;
  }
  Vector_Init(&sym->supers);
  sym->stbl = STable_New();
  sym->desc = TypeDesc_New_Klass(NULL, name);
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
  sym->desc = TypeDesc_New_Klass(NULL, name);
  return sym;
}

EnumSymbol *STable_Add_Enum(STable *stbl, char *name)
{
  EnumSymbol *sym = (EnumSymbol *)Symbol_New(SYM_ENUM, name);
  if (HashTable_Insert(&stbl->table, &sym->hnode) < 0) {
    Symbol_Free((Symbol *)sym);
    return NULL;
  }
  sym->stbl = STable_New();
  sym->desc = TypeDesc_New_Klass(NULL, name);
  return sym;
}

EnumValSymbol *STable_Add_EnumValue(STable *stbl, char *name)
{
  EnumValSymbol *sym = (EnumValSymbol *)Symbol_New(SYM_EVAL, name);
  if (HashTable_Insert(&stbl->table, &sym->hnode) < 0) {
    Symbol_Free((Symbol *)sym);
    return NULL;
  }
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
*/

Symbol *STable_Get(STable *stbl, char *name)
{
  if (stbl == NULL)
    return NULL;
  Symbol key = {.name = name};
  hashmap_entry_init(&key, strhash(name));
  return hashmap_get(&stbl->table, &key);
}

#if 0
static void __gen_image_func(Symbol *sym, void *arg)
{
  assert(sym->kind > 0 && sym->kind < SYM_MAX);
  struct symbol_operations *ops = &symops[sym->kind];
  ops->__symbol_gen(sym, arg);
}

KImage *Gen_Image(STable *stbl, char *pkgname)
{
  debug("\x1b[34m----STARTING IMAGE----\x1b[0m");
  KImage *image = KImage_New(pkgname);
  struct gen_image_s info = {image, NULL};
  STable_Visit(stbl, __gen_image_func, &info);
  KImage_Finish(image);
  KImage_Show(image);
  debug("\x1b[34m----END OF IMAGE------\x1b[0m");
  return image;
}

#define IsAccess(name) isupper(name[0])

static void __get_var_fn(char *name, TypeDesc *desc, int k,
                         ConstValue *val, STable *stbl)
{
  if (!IsAccess(name)) {
    debug("'%s' is private", name);
    return;
  }

  String sname = AtomString_New(name);
  if (k) {
    debug("load const '%s'", name);
    STable_Add_Const(stbl, sname.str, desc);
  } else {
    debug("load var '%s'", name);
    STable_Add_Var(stbl, sname.str, desc);
  }
}

static void __get_func_fn(char *name, TypeDesc *desc, int idx, int type,
                          uint8 *code, int sz, STable *stbl)
{
#if 0
  if (!IsAccess(name)) {
    debug("'%s' is private", name);
    return;
  }
#endif
  String sname = AtomString_New(name);
  if (type == ITEM_FUNC) {
    debug("load func '%s'", name);
    STable_Add_Func(stbl, sname.str, desc);
  } else if (type == ITEM_NFUNC) {
    debug("load native func '%s'", name);
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
#endif