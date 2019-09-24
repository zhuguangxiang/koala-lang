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

#include "parser.h"
#include "memory.h"
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
  return stbl;
}

static void _symbol_free_(void *e, void *arg)
{
  Symbol *sym = e;
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

  ++sym->refcnt;
  return 0;
}

Symbol *stable_remove(STable *stbl, char *name)
{
  Symbol key = {.name = name};
  hashmap_entry_init(&key, strhash(name));
  Symbol *sym = hashmap_remove(&stbl->table, &key);
  if (sym != NULL && sym->kind == SYM_VAR) {
    --stbl->varindex;
  }
  return sym;
}

Symbol *stable_add_const(STable *stbl, char *name, TypeDesc *desc)
{
  Symbol *sym = symbol_new(name, SYM_CONST);
  if (stable_add_symbol(stbl, sym))
    return NULL;
  sym->desc = TYPE_INCREF(desc);
  sym->var.index = ++stbl->varindex;
  symbol_decref(sym);
  return sym;
}

Symbol *stable_add_var(STable *stbl, char *name, TypeDesc *desc)
{
  Symbol *sym = symbol_new(name, SYM_VAR);
  if (stable_add_symbol(stbl, sym))
    return NULL;
  sym->desc = TYPE_INCREF(desc);
  sym->var.index = ++stbl->varindex;
  symbol_decref(sym);
  return sym;
}

Symbol *stable_add_func(STable *stbl, char *name, TypeDesc *proto)
{
  Symbol *sym = symbol_new(name, SYM_FUNC);
  if (stable_add_symbol(stbl, sym))
    return NULL;
  sym->desc = TYPE_INCREF(proto);
  symbol_decref(sym);
  return sym;
}

static void symbol_free(Symbol *sym)
{
  TYPE_DECREF(sym->desc);

  switch (sym->kind) {
  case SYM_CONST:
    panic("SYM_CONST not implemented");
    break;
  case SYM_VAR:
    debug("[Symbol Freed] var '%s'", sym->name);
    break;
  case SYM_FUNC:
    debug("[Symbol Freed] func '%s'", sym->name);
    codeblock_free(sym->func.codeblock);
    Symbol *locsym;
    vector_for_each(locsym, &sym->func.locvec) {
      symbol_decref(locsym);
    }
    vector_fini(&sym->func.locvec);
    break;
  case SYM_CLASS:
    debug("[Symbol Freed] class '%s'", sym->name);
    stable_free(sym->klass.stbl);
    Symbol *tmp;
    vector_for_each_reverse(tmp, &sym->klass.bases) {
      symbol_decref(tmp);
    }
    vector_fini(&sym->klass.bases);
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
  case SYM_ANONY:
    panic("SYM_ANONY not implemented");
    break;
  case SYM_MOD:
    debug("[Symbol Freed] module '%s'", sym->name);
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

void symbol_decref(Symbol *sym)
{
  if (sym == NULL)
    return;

  --sym->refcnt;
  if (sym->refcnt == 0) {
    symbol_free(sym);
  } else if (sym->refcnt < 0) {
    panic("symbol '%s' refcnt %d error", sym->name, sym->refcnt);
  } else {
    /* empty */
  }
}

static Symbol *load_field(Object *ob)
{
  FieldObject *fo = (FieldObject *)ob;
  debug("load field '%s'", fo->name);
  Symbol *sym = symbol_new(fo->name, SYM_VAR);
  sym->desc = TYPE_INCREF(fo->desc);
  return sym;
}

static Symbol *load_method(Object *ob)
{
  MethodObject *meth = (MethodObject *)ob;
  debug("load method '%s'", meth->name);
  Symbol *sym = symbol_new(meth->name, SYM_FUNC);
  sym->desc = TYPE_INCREF(meth->desc);
  return sym;
}

static Symbol *load_type(Object *ob)
{
  TypeObject *type = (TypeObject *)ob;
  ModuleObject *mob = (ModuleObject *)type->owner;
  if (type->mtbl == NULL) {
    warn("mtbl of type '%s' is null", type->name);
    return NULL;
  }

  debug("load type '%s'", type->name);
  STable *stbl = stable_new();
  HASHMAP_ITERATOR(iter, type->mtbl);
  struct mnode *node;
  Object *tmp;
  Symbol *sym;
  iter_for_each(&iter, node) {
    tmp = node->obj;
    if (field_check(tmp)) {
      sym = load_field(tmp);
    } else if (Method_Check(tmp)) {
      sym = load_method(tmp);
    } else {
      panic("object of '%s'?", OB_TYPE(tmp)->name);
    }
    stable_add_symbol(stbl, sym);
    symbol_decref(sym);
  }

  Symbol *clsSym = symbol_new(type->name, SYM_CLASS);
  clsSym->desc = TYPE_INCREF(type->desc);
  clsSym->klass.stbl = stbl;

  TypeObject *item;
  vector_for_each_reverse(item, &type->lro) {
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

STable *stable_from_mobject(Object *ob)
{
  ModuleObject *mo = (ModuleObject *)ob;
  expect(mo->mtbl != NULL);

  STable *stbl = stable_new();
  HASHMAP_ITERATOR(iter, mo->mtbl);
  struct mnode *node;
  Object *tmp;
  Symbol *sym;
  iter_for_each(&iter, node) {
    tmp = node->obj;
    if (Type_Check(tmp)) {
      sym = load_type(tmp);
    } else if (field_check(tmp)) {
      sym = load_field(tmp);
    } else if (Method_Check(tmp)) {
      sym = load_method(tmp);
    } else {
      panic("object of '%s'?", OB_TYPE(tmp)->name);
    }
    stable_add_symbol(stbl, sym);
    symbol_decref(sym);
  }
  TypeDesc *desc = desc_from_base('s');
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

  Symbol *item;
  vector_for_each_reverse(item, &clsSym->klass.bases) {
    sym = klass_find_member(item, name);
    if (sym != NULL)
      return sym;
  }

  return NULL;
}
