/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "symbol.h"
#include "memory.h"
#include "log.h"
#include "moduleobject.h"
#include "fieldobject.h"
#include "methodobject.h"

static int symbol_equal(void *k1, void *k2)
{
  Symbol *s1 = k1;
  Symbol *s2 = k2;
  return !strcmp(s1->name, s2->name);
}

STable *stable_new(void)
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
  symbol_decref(sym);
}

void stable_free(STable *stbl)
{
  if (stbl == NULL)
    return;
  hashmap_fini(&stbl->table, _symbol_free_, NULL);
  kfree(stbl);
}

Symbol *stable_get(STable *stbl, char *name)
{
  if (stbl == NULL)
    return NULL;
  Symbol key = {.name = name};
  hashmap_entry_init(&key, strhash(name));
  return hashmap_get(&stbl->table, &key);
}

Symbol *symbol_new(char *name, SymKind kind)
{
  Symbol *sym = kmalloc(sizeof(Symbol));
  sym->name = name;
  sym->kind = kind;
  hashmap_entry_init(sym, strhash(name));
  sym->refcnt = 1;
  return sym;
}

int stable_add_symbol(STable *stbl, Symbol *sym)
{
  if (sym == NULL) {
    warn("null pointer");
    return -1;
  }

  if (hashmap_add(&stbl->table, sym) < 0) {
    error("add symbol '%s' failed", sym->name);
    symbol_decref((Symbol *)sym);
    return -1;
  }
  return 0;
}

Symbol *stable_add_const(STable *stbl, char *name, TypeDesc *desc)
{
  Symbol *sym = symbol_new(name, SYM_CONST);
  if (stable_add_symbol(stbl, sym))
    return NULL;
  sym->desc = TYPE_INCREF(desc);
  sym->var.index = stbl->varindex++;
  return sym;
}

Symbol *stable_add_var(STable *stbl, char *name, TypeDesc *desc)
{
  Symbol *sym = symbol_new(name, SYM_VAR);
  if (stable_add_symbol(stbl, sym))
    return NULL;
  sym->desc = TYPE_INCREF(desc);
  sym->var.index = stbl->varindex++;
  return sym;
}

Symbol *stable_add_func(STable *stbl, char *name, TypeDesc *proto)
{
  Symbol *sym = symbol_new(name, SYM_FUNC);
  if (stable_add_symbol(stbl, sym))
    return NULL;
  sym->desc = TYPE_INCREF(proto);
  return sym;
}

static inline void symbol_free(Symbol *sym)
{
  if (sym->refcnt > 0)
    panic("refcnt of symbol '%s' is not 0", sym->name);
  switch (sym->kind) {
  case SYM_CONST:
    break;
  case SYM_VAR:
    kfree(sym);
    break;
  case SYM_FUNC:
    break;
  case SYM_CLASS:
    break;
  case SYM_TRAIT:
    break;
  case SYM_ENUM:
    break;
  case SYM_EVAL:
    break;
  case SYM_IFUNC:
    break;
  case SYM_NFUNC:
    break;
  case SYM_AFUNC:
    break;
  case SYM_MOD:
    break;
  case SYM_REF:
    break;
  default:
    panic("invalide branch %d", sym->kind);
    break;
  }
}

void symbol_decref(Symbol *sym)
{
  if (--sym->refcnt <= 0) {
    symbol_free(sym);
  } else {
    debug("refcnt of symbol '%s' is %d", sym->name, sym->refcnt);
  }
}

static Symbol *load_field(Object *ob)
{
  FieldObject *fo = (FieldObject *)ob;
  Symbol *sym = symbol_new(fo->name, SYM_VAR);
  sym->desc = TYPE_INCREF(fo->desc);
  return sym;
}

static Symbol *load_method(Object *ob)
{
  MethodObject *meth = (MethodObject *)ob;
  Symbol *sym = symbol_new(meth->name, SYM_FUNC);
  sym->desc = TYPE_INCREF(meth->desc);
  return sym;
}

static Symbol *load_type(Object *ob)
{
  TypeObject *type = (TypeObject *)ob;
  if (type->mtbl == NULL) {
    warn("mtbl of type '%s' is null", type->name);
    return NULL;
  }

  STable *stbl = stable_new();
  HASHMAP_ITERATOR(iter, type->mtbl);
  struct mnode *node;
  Object *tmp;
  Symbol *sym;
  iter_for_each(&iter, node) {
    tmp = node->obj;
    if (Field_Check(tmp)) {
      sym = load_field(tmp);
    } else if (Method_Check(tmp)) {
      sym = load_method(tmp);
    } else {
      panic("object of '%s'?", OB_TYPE(tmp)->name);
    }
    stable_add_symbol(stbl, sym);
  }

  Symbol *clsSym = symbol_new(type->name, SYM_CLASS);
  clsSym->klass.stbl = stbl;

  TypeObject *item;
  VECTOR_REVERSE_ITERATOR(iter2, &type->lro);
  iter_for_each(&iter2, item) {
    if (item == type)
      continue;
    sym = load_type((Object *)item);
    vector_push_back(&clsSym->klass.supers, sym);
  }

  return clsSym;
}

STable *stable_from_mobject(Object *ob)
{
  ModuleObject *mo = (ModuleObject *)ob;
  if (mo->mtbl == NULL)
    panic("mtbl of mobject '%s' is null", mo->name);

  STable *stbl = stable_new();
  HASHMAP_ITERATOR(iter, mo->mtbl);
  struct mnode *node;
  Object *tmp;
  Symbol *sym;
  iter_for_each(&iter, node) {
    tmp = node->obj;
    if (Type_Check(tmp)) {
      sym = load_type(tmp);
    } else if (Field_Check(tmp)) {
      sym = load_field(tmp);
    } else if (Method_Check(tmp)) {
      sym = load_method(tmp);
    } else {
      panic("object of '%s'?", OB_TYPE(tmp)->name);
    }
    stable_add_symbol(stbl, sym);
  }
  TypeDesc *desc = desc_getbase('s');
  stable_add_var(stbl, "__name__", desc);
  TYPE_DECREF(desc);
  return stbl;
}

Symbol *klass_find_member(Symbol *clsSym, char *name)
{
  if (clsSym->kind != SYM_CLASS) {
    error("sym '%s' is not class", clsSym->name);
    return NULL;
  }

  Symbol *sym = stable_get(clsSym->klass.stbl, name);
  if (sym != NULL)
    return sym;

  VECTOR_REVERSE_ITERATOR(iter, &clsSym->klass.supers);
  Symbol *item;
  iter_for_each(&iter, item) {
    sym = klass_find_member(item, name);
    if (sym != NULL)
      return sym;
  }

  return NULL;
}
