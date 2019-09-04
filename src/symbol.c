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
  symbol *s1 = k1;
  symbol *s2 = k2;
  return !strcmp(s1->name, s2->name);
}

symtable *stable_new(void)
{
  symtable *stbl = kmalloc(sizeof(symtable));
  hashmap_init(&stbl->table, symbol_equal);
  /* [0]: module or class self */
  stbl->varindex = 1;
  return stbl;
}

static void _symbol_free_(void *e, void *arg)
{
  symbol *sym = e;
  symbol_decref(sym);
}

void stable_free(symtable *stbl)
{
  if (stbl == NULL)
    return;
  hashmap_fini(&stbl->table, _symbol_free_, NULL);
  kfree(stbl);
}

symbol *stable_get(symtable *stbl, char *name)
{
  if (stbl == NULL)
    return NULL;
  symbol key = {.name = name};
  hashmap_entry_init(&key, strhash(name));
  return hashmap_get(&stbl->table, &key);
}

symbol *symbol_new(char *name, SymKind kind)
{
  symbol *sym = kmalloc(sizeof(symbol));
  sym->name = name;
  sym->kind = kind;
  hashmap_entry_init(sym, strhash(name));
  sym->refcnt = 1;
  return sym;
}

int stable_add_symbol(symtable *stbl, symbol *sym)
{
  if (sym == NULL) {
    warn("null pointer");
    return -1;
  }

  if (hashmap_add(&stbl->table, sym) < 0) {
    error("add symbol '%s' failed", sym->name);
    symbol_decref((symbol *)sym);
    return -1;
  }

  ++sym->refcnt;
  return 0;
}

symbol *stable_add_const(symtable *stbl, char *name, typedesc *desc)
{
  symbol *sym = symbol_new(name, SYM_CONST);
  if (stable_add_symbol(stbl, sym))
    return NULL;
  sym->desc = desc_incref(desc);
  sym->var.index = stbl->varindex++;
  symbol_decref(sym);
  return sym;
}

symbol *stable_add_var(symtable *stbl, char *name, typedesc *desc)
{
  symbol *sym = symbol_new(name, SYM_VAR);
  if (stable_add_symbol(stbl, sym))
    return NULL;
  sym->desc = desc_incref(desc);
  sym->var.index = stbl->varindex++;
  symbol_decref(sym);
  return sym;
}

symbol *stable_add_func(symtable *stbl, char *name, typedesc *proto)
{
  symbol *sym = symbol_new(name, SYM_FUNC);
  if (stable_add_symbol(stbl, sym))
    return NULL;
  sym->desc = desc_incref(proto);
  symbol_decref(sym);
  return sym;
}

static inline void symbol_free(symbol *sym)
{
  desc_decref(sym->desc);

  switch (sym->kind) {
  case SYM_CONST:
    panic("SYM_CONST not implemented");
    break;
  case SYM_VAR:
    debug("[symbol Freed] var '%s'", sym->name);
    break;
  case SYM_FUNC:
    debug("[symbol Freed] func '%s'", sym->name);
    break;
  case SYM_CLASS:
    debug("[symbol Freed] class '%s'", sym->name);
    stable_free(sym->klass.stbl);
    vector_reverse_iterator(iter, &sym->klass.bases);
    symbol *tmp;
    iter_for_each(&iter, tmp) {
      symbol_decref(tmp);
    }
    vector_fini(&sym->klass.bases, NULL, NULL);
    break;
  case SYM_TRAIT:
    panic("SYM_TRAIT not implemented");
    break;
  case SYM_ENUM:
    panic("SYM_ENUM not implemented");
    break;
  case SYM_EVAL:
    panic("SYM_EVAL not implemented");
    break;
  case SYM_IFUNC:
    panic("SYM_IFUNC not implemented");
    break;
  case SYM_NFUNC:
    panic("SYM_NFUNC not implemented");
    break;
  case SYM_AFUNC:
    panic("SYM_MOD not implemented");
    break;
  case SYM_MOD:
    debug("[symbol Freed] module '%s'", sym->name);
    break;
  case SYM_REF:
    panic("SYM_REF not implemented");
    break;
  default:
    panic("invalide symbol '%s' kind %d", sym->name, sym->kind);
    break;
  }

  kfree(sym);
}

void symbol_decref(symbol *sym)
{
  if (sym == NULL)
    return;

  --sym->refcnt;
  if (sym->refcnt == 0) {
    symbol_free(sym);
  } else if (sym->refcnt < 0) {
    panic("sym '%s' refcnt %d error", sym->name, sym->refcnt);
  } else {
    /* empty */
  }
}

static symbol *load_field(Object *ob)
{
  FieldObject *fo = (FieldObject *)ob;
  debug("load field '%s'", fo->name);
  symbol *sym = symbol_new(fo->name, SYM_VAR);
  sym->desc = desc_incref(fo->desc);
  return sym;
}

static symbol *load_method(Object *ob)
{
  MethodObject *meth = (MethodObject *)ob;
  debug("load method '%s'", meth->name);
  symbol *sym = symbol_new(meth->name, SYM_FUNC);
  sym->desc = desc_incref(meth->desc);
  return sym;
}

static symbol *load_type(Object *ob)
{
  TypeObject *type = (TypeObject *)ob;
  ModuleObject *mob = (ModuleObject *)type->owner;
  if (type->mtbl == NULL) {
    warn("mtbl of type '%s' is null", type->name);
    return NULL;
  }

  debug("load type '%s'", type->name);
  symtable *stbl = stable_new();
  hashmap_iterator(iter, type->mtbl);
  struct mnode *node;
  Object *tmp;
  symbol *sym;
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
    symbol_decref(sym);
  }

  symbol *clsSym = symbol_new(type->name, SYM_CLASS);
  clsSym->desc = desc_from_klass(mob->path, type->name, NULL);
  clsSym->klass.stbl = stbl;

  TypeObject *item;
  vector_reverse_iterator(iter2, &type->lro);
  iter_for_each(&iter2, item) {
    if (item == type)
      continue;
    sym = load_type((Object *)item);
    if (sym != NULL) {
      ++sym->refcnt;
      vector_push_back(&clsSym->klass.bases, sym);
      symbol_decref(sym);
    }
  }

  return clsSym;
}

symtable *stable_from_mobject(Object *ob)
{
  ModuleObject *mo = (ModuleObject *)ob;
  if (mo->mtbl == NULL)
    panic("mtbl of mobject '%s' is null", mo->name);

  symtable *stbl = stable_new();
  hashmap_iterator(iter, mo->mtbl);
  struct mnode *node;
  Object *tmp;
  symbol *sym;
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
    symbol_decref(sym);
  }
  typedesc *desc = desc_from_base('s');
  stable_add_var(stbl, "__name__", desc);
  desc_decref(desc);
  return stbl;
}

symbol *klass_find_member(symbol *clsSym, char *name)
{
  if (clsSym->kind != SYM_CLASS) {
    error("sym '%s' is not class", clsSym->name);
    return NULL;
  }

  symbol *sym = stable_get(clsSym->klass.stbl, name);
  if (sym != NULL)
    return sym;

  vector_reverse_iterator(iter, &clsSym->klass.bases);
  symbol *item;
  iter_for_each(&iter, item) {
    sym = klass_find_member(item, name);
    if (sym != NULL)
      return sym;
  }

  return NULL;
}
