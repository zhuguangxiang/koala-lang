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
  return k1 == k2 || !strcmp(s1->name, s2->name);
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
    warn("add symbol '%s' failed", sym->name);
    symbol_decref((Symbol *)sym);
    return -1;
  }

  ++sym->refcnt;
  return 0;
}

Symbol *stable_remove(STable *stbl, char *name)
{
  expect(stbl != NULL);
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
  expect(stbl != NULL);
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
  expect(stbl != NULL);
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
  expect(stbl != NULL);
  Symbol *sym = symbol_new(name, SYM_FUNC);
  if (stable_add_symbol(stbl, sym))
    return NULL;
  sym->desc = TYPE_INCREF(proto);
  symbol_decref(sym);
  return sym;
}

Symbol *stable_add_ifunc(STable *stbl, char *name, TypeDesc *proto)
{
  expect(stbl != NULL);
  Symbol *sym = symbol_new(name, SYM_IFUNC);
  if (stable_add_symbol(stbl, sym))
    return NULL;
  sym->desc = TYPE_INCREF(proto);
  symbol_decref(sym);
  return sym;
}

Symbol *stable_add_class(STable *stbl, char *name)
{
  expect(stbl != NULL);
  Symbol *sym = symbol_new(name, SYM_CLASS);
  if (stable_add_symbol(stbl, sym))
    return NULL;
  sym->desc = desc_from_klass(NULL, name);
  sym->type.stbl = stable_new();
  symbol_decref(sym);
  return sym;
}

Symbol *stable_add_trait(STable *stbl, char *name)
{
  expect(stbl != NULL);
  Symbol *sym = symbol_new(name, SYM_TRAIT);
  if (stable_add_symbol(stbl, sym))
    return NULL;
  sym->desc = desc_from_klass(NULL, name);
  sym->type.stbl = stable_new();
  symbol_decref(sym);
  return sym;
}

Symbol *stable_add_enum(STable *stbl, char *name)
{
  expect(stbl != NULL);
  Symbol *sym = symbol_new(name, SYM_ENUM);
  if (stable_add_symbol(stbl, sym))
    return NULL;
  sym->desc = desc_from_klass(NULL, name);
  sym->type.stbl = stable_new();
  symbol_decref(sym);
  return sym;
}

Symbol *stable_add_label(STable *stbl, char *name)
{
  expect(stbl != NULL);
  Symbol *sym = symbol_new(name, SYM_LABEL);
  if (stable_add_symbol(stbl, sym))
    return NULL;
  symbol_decref(sym);
  return sym;
}

void symbol_free(Symbol *sym)
{
  TYPE_DECREF(sym->desc);

  switch (sym->kind) {
  case SYM_CONST:
    debug("[Symbol Freed] const '%s'", sym->name);
    break;
  case SYM_VAR:
    debug("[Symbol Freed] var '%s'", sym->name);
    break;
  case SYM_FUNC: {
    debug("[Symbol Freed] func '%s'", sym->name);
    codeblock_free(sym->func.codeblock);
    Symbol *locsym;
    vector_for_each(locsym, &sym->func.locvec) {
      symbol_decref(locsym);
    }
    vector_fini(&sym->func.locvec);
    vector_fini(&sym->func.freevec);
    break;
  }
  case SYM_CLASS:
  case SYM_TRAIT:
  case SYM_ENUM:
    debug("[Symbol Freed] class/trait/enum '%s'", sym->name);
    stable_free(sym->type.stbl);
    Symbol *tmp;
    vector_for_each_reverse(tmp, &sym->type.lro) {
      symbol_decref(tmp);
    }
    vector_fini(&sym->type.lro);
    vector_fini(&sym->type.traits);
    break;
  case SYM_LABEL:
    debug("[Symbol Freed] enum label '%s'", sym->name);
    free_descs(sym->label.types);
    break;
  case SYM_IFUNC:
    debug("[Symbol Freed] ifunc '%s'", sym->name);
    break;
  case SYM_ANONY: {
    debug("[Symbol Freed] anonymous '%s'", sym->name);
    codeblock_free(sym->anony.codeblock);
    Symbol *locsym;
    vector_for_each(locsym, &sym->anony.locvec) {
      symbol_decref(locsym);
    }
    vector_fini(&sym->anony.locvec);
    vector_fini(&sym->anony.freevec);
    vector_fini(&sym->anony.upvec);
    break;
  }
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

Symbol *load_field(Object *ob)
{
  FieldObject *fo = (FieldObject *)ob;
  debug("load field '%s'", fo->name);
  Symbol *sym = symbol_new(fo->name, SYM_VAR);
  sym->desc = TYPE_INCREF(fo->desc);
  return sym;
}

Symbol *load_method(Object *ob)
{
  MethodObject *meth = (MethodObject *)ob;
  debug("load method '%s'", meth->name);
  Symbol *sym = symbol_new(meth->name, SYM_FUNC);
  sym->desc = TYPE_INCREF(meth->desc);
  return sym;
}

Symbol *load_type(Object *ob)
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
    } else if (method_check(tmp)) {
      sym = load_method(tmp);
    } else {
      panic("object of '%s'?", OB_TYPE(tmp)->name);
    }
    stable_add_symbol(stbl, sym);
    symbol_decref(sym);
  }

  Symbol *clsSym = symbol_new(type->name, SYM_CLASS);
  clsSym->desc = TYPE_INCREF(type->desc);
  clsSym->type.stbl = stbl;

  TypeObject *item;
  vector_for_each_reverse(item, &type->lro) {
    if (item == type)
      continue;
    sym = load_type((Object *)item);
    if (sym != NULL) {
      ++sym->refcnt;
      vector_push_back(&clsSym->type.lro, sym);
      symbol_decref(sym);
    }
  }

  return clsSym;
}

void fill_locvars(Symbol *sym, Vector *vec)
{
  Vector *locvec = NULL;
  SymKind kind = sym->kind;
  if (kind == SYM_FUNC) {
    locvec = &sym->func.locvec;
  } else if (kind == SYM_ANONY) {
    locvec = &sym->anony.locvec;
  } else {
    panic("getlocvars invalid symbol kind %d", kind);
  }

  LocVar *var;
  Symbol *varsym;
  vector_for_each(varsym, locvec) {
    expect(varsym->kind == SYM_VAR);
    var = locvar_new(varsym->name, varsym->desc, varsym->var.index);
    vector_push_back(vec, var);
  }
}

void free_locvars(Vector *locvec)
{
  LocVar *var;
  vector_for_each(var, locvec) {
    locvar_free(var);
  }
}

static void fill_codeinfo(Symbol *sym, Image *image, CodeInfo *ci)
{
  ByteBuffer buf;
  uint8_t *code = NULL;
  int size;

  bytebuffer_init(&buf, 32);
  code_gen(sym->func.codeblock, image, &buf);
  size = bytebuffer_toarr(&buf, (char **)&code);
  ci->name = sym->name;
  ci->desc = sym->desc;
  ci->codes = code;
  ci->size = size;
  ci->freevec = &sym->func.freevec;
  bytebuffer_fini(&buf);
}

void type_write_image(Symbol *typesym, Image *image)
{
  int size = stable_size(typesym->type.stbl);
  MbrIndex indexes[size];
  HASHMAP_ITERATOR(iter, &typesym->type.stbl->table);
  Symbol *sym;
  int j = 0;
  int index;
  iter_for_each(&iter, sym) {
    switch (sym->kind) {
    case SYM_VAR:
      index = image_add_field(image, sym->name, sym->desc);
      indexes[j].kind = MBR_FIELD;
      indexes[j].index = index;
      break;
    case SYM_FUNC: {
      VECTOR(locvec);
      CodeInfo ci = {.locvec = &locvec};
      fill_codeinfo(sym, image, &ci);
      fill_locvars(sym, &locvec);
      index = image_add_method(image, &ci);
      free_locvars(&locvec);
      vector_fini(&locvec);
      indexes[j].kind = MBR_METHOD;
      indexes[j].index = index;
      break;
    }
    case SYM_IFUNC:
      index = image_add_ifunc(image, sym->name, sym->desc);
      indexes[j].kind = MBR_IFUNC;
      indexes[j].index = index;
      break;
    case SYM_LABEL:
      index = image_add_label(image, sym->name, sym->label.types, 0);
      indexes[j].kind = MBR_LABEL;
      indexes[j].index = index;
      break;
    default:
      panic("invalid symbol kind %d in class/trait/enum", sym->kind);
      break;
    }
    ++j;
  }

  index = image_add_mbrs(image, indexes, size);

  Vector *types = vector_new();
  Symbol *s = typesym->type.base;
  if (s != NULL && !desc_isany(s->desc))
    vector_push_back(types, s->desc);

  Vector *traits = &typesym->type.traits;
  vector_for_each(s, traits) {
    if (!desc_isany(s->desc))
      vector_push_back(types, s->desc);
  }
  if (typesym->kind == SYM_CLASS) {
    image_add_class(image, typesym->name, types, index);
  } else if (typesym->kind == SYM_TRAIT) {
    image_add_trait(image, typesym->name, types, index);
  } else {
    expect(typesym->kind == SYM_ENUM);
    image_add_enum(image, typesym->name, index);
  }
  vector_free(types);
}

Symbol *type_find_mbr(Symbol *typeSym, char *name)
{
  if (typeSym->kind != SYM_CLASS && typeSym->kind != SYM_ENUM &&
      typeSym->kind != SYM_TRAIT) {
    error("sym '%s' is not class/trait/enum", typeSym->name);
    return NULL;
  }

  Symbol *sym = stable_get(typeSym->type.stbl, name);
  if (sym != NULL)
    return sym;

  Symbol *item;
  vector_for_each_reverse(item, &typeSym->type.lro) {
    sym = type_find_mbr(item, name);
    if (sym != NULL)
      return sym;
  }

  return NULL;
}

Symbol *type_find_super_mbr(Symbol *typeSym, char *name)
{
  if (typeSym->kind != SYM_CLASS && typeSym->kind != SYM_ENUM &&
      typeSym->kind != SYM_TRAIT) {
    error("sym '%s' is not class/trait/enum", typeSym->name);
    return NULL;
  }

  Symbol *sym;
  Symbol *item;
  vector_for_each_reverse(item, &typeSym->type.lro) {
    sym = type_find_mbr(item, name);
    if (sym != NULL)
      return sym;
  }

  return NULL;
}

void stable_write_image(STable *stbl, Image *image)
{
  HASHMAP_ITERATOR(iter, &stbl->table);
  Symbol *sym;
  iter_for_each(&iter, sym) {
    switch (sym->kind) {
    case SYM_VAR:
      debug("write variable '%s'", sym->name);
      image_add_var(image, sym->name, sym->desc);
      break;
    case SYM_FUNC: {
      debug("write function '%s'", sym->name);
      VECTOR(locvec);
      CodeInfo ci = {.locvec = &locvec};
      fill_codeinfo(sym, image, &ci);
      if (ci.size != 0) {
        fill_locvars(sym, &locvec);
        image_add_func(image, &ci);
      } else {
        kfree(ci.codes);
        warn("function '%s' is empty", sym->name);
      }
      free_locvars(&locvec);
      vector_fini(&locvec);
      break;
    }
    case SYM_CLASS:
    case SYM_TRAIT:
    case SYM_ENUM:
      debug("write type(class/trait/enum) '%s'", sym->name);
      type_write_image(sym, image);
      break;
    case SYM_CONST:
      debug("write constvar '%s'", sym->name);
      image_add_kvar(image, sym->name, sym->desc, &sym->k.value);
      break;
    case SYM_MOD:
      debug("skip imported module '%s'", sym->name);
      break;
    default:
      panic("invalid symbol %d write to image", sym->kind);
      break;
    }
  }
}
