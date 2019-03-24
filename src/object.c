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

#include "hashfunc.h"
#include "stringex.h"
#include "mem.h"
#include "log.h"
#include "stringobject.h"
#include "codeobject.h"
#include "intobject.h"
#include "tupleobject.h"

LOGGER(0)

MNode *MNode_New(MemberKind kind, char *name, TypeDesc *desc)
{
  MNode *m = Malloc(sizeof(MNode));
  Init_HashNode(&m->hnode, m);
  m->kind = kind;
  m->name = AtomString_New(name).str;
  TYPE_INCREF(desc);
  m->desc = desc;
  return m;
}

void MNode_Free(MNode *m)
{
  TYPE_DECREF(m->desc);
  Mfree(m);
}

MNode *MNode_Find(HashTable *table, char *name)
{
  MNode key = {.name = name};
  HashNode *hnode = HashTable_Find(table, &key);
  return hnode ? container_of(hnode, MNode, hnode) : NULL;
}

Object *MTable_ToString(HashTable *table)
{
  int count = table->nr_nodes;
  Object *tuple = Tuple_New(count);

  HashNode *hnode;
  HashTable_ForEach(hnode, table) {
    MNode *m = container_of(hnode, MNode, hnode);
    Tuple_Append(tuple, String_New(m->name));
  }

  //Object *so = Tuple_ToString(tuple, NULL);
  OB_DECREF(tuple);
  return NULL;
}

/* hash function of MNode */
static uint32 __m_hash_func(MNode *m)
{
  return hash_string(m->name);
}

/* equal function of MNode  */
static int __m_equal_func(MNode *m1, MNode *m2)
{
  return !strcmp(m1->name, m2->name);
}

void Init_MTable(HashTable *table)
{
  HashTable_Init(table, (hashfunc)__m_hash_func, (equalfunc)__m_equal_func);
}

static void free_m_func(HashNode *hnode, void *arg)
{
  MNode *m = container_of(hnode, MNode, hnode);
  switch (m->kind) {
  case CONST_KIND:
    Log_Debug("const   \x1b[34m%-12s\x1b[0m decreased", m->name);
    OB_DECREF(m->value);
    break;
  case VAR_KIND:
    Log_Debug("var     \x1b[34m%-12s\x1b[0m decreased", m->name);
    break;
  case FUNC_KIND:
    Log_Debug("func    \x1b[34m%-12s\x1b[0m decreased", m->name);
    OB_DECREF(m->code);
    break;
  case PROTO_KIND:
    assert(0);
    break;
  case CLASS_KIND:
    OB_DECREF(m->klazz);
    break;
  default:
    assert(0);
    break;
  }
  MNode_Free(m);
}

void Fini_MTable(HashTable *table)
{
  HashTable_Fini(table, free_m_func, NULL);
}

static LRONode *LRONode_New(int offset, Klass *klazz)
{
  LRONode *node = Malloc(sizeof(LRONode));
  node->offset = offset;
  node->klazz  = klazz;
  return node;
}

static int lro_find(Vector *lro, Klass *klazz)
{
  LRONode *node;
  Vector_ForEach(node, lro) {
    if (node->klazz == klazz)
      return 1;
  }
  return 0;
}

static void lro_debug(Klass *klazz)
{
  Log_Puts("----------line-order----------");
  char *fmt;
  LRONode *node;
  Vector_ForEach_Reverse(node, &klazz->lro) {
    fmt = (i > 0) ? "%s -> " : "%s";
    Log_Printf(fmt, node->klazz->name);
  }
  Log_Puts("\n------------------------------");
}

#define LRO_BUILD_ONE(_klazz, _base)                      \
({                                                        \
  LRONode *lro, *node;                                    \
  Vector_ForEach(node, &(_base)->lro) {                   \
    if (!lro_find(&(_klazz)->lro, node->klazz)) {         \
      lro = LRONode_New(offset, OB_INCREF(node->klazz));  \
      Vector_Append(&(_klazz)->lro, lro);                 \
      offset += node->klazz->nrvars;                      \
    }                                                     \
  }                                                       \
})

static void lro_build(Klass *klazz)
{
  Klass *base = klazz->base;
  Vector *traits = klazz->traits;
  int offset = 0;
  LRONode *lro;
  LRONode *node;

  /* base klass */
  if (base != NULL) {
    LRO_BUILD_ONE(klazz, base);
  }

  /* trait klasses */
  Klass *line;
  Vector_ForEach(line, traits) {
    LRO_BUILD_ONE(klazz, line);
  }

  /* self klass */
  lro = LRONode_New(offset, klazz);
  Vector_Append(&klazz->lro, lro);
  klazz->totalvars = offset;
}

void Init_Klass(Klass *klazz, Klass *base, Vector *traits)
{
  Init_MTable(&klazz->mtbl);
  Vector_Init(&klazz->lro);
  klazz->base = OB_INCREF(base);
  klazz->traits = traits;
  lro_build(klazz);
  lro_debug(klazz);
}

void Fini_Klass(Klass *klazz)
{
  Log_Debug("------------------------------");
  LRONode *lro;
  Vector_ForEach(lro, &klazz->lro) {
    if (lro->klazz != klazz)
      OB_DECREF(lro->klazz);
    Mfree(lro);
  }
  Vector_Fini_Self(&klazz->lro);
  Fini_MTable(&klazz->mtbl);
  Log_Debug("klass   \x1b[32m%-12s\x1b[0m finalized", klazz->name);
}

static void klass_free(Object *ob)
{
  Klass *klazz = (Klass *)ob;
  Log_Debug("------------------------------");
  Fini_Klass(klazz);
  Mfree(klazz);
}

static Object *klass_tostring(Object *ob, Object *args)
{
  Klass *klazz = (Klass *)ob;
  Object *tuple = Tuple_New(klazz->mtbl.nr_nodes);
  HashNode *hnode;
  HashTable_ForEach(hnode, &klazz->mtbl) {
    MNode *m = HashNode_Entry(hnode, MNode, hnode);
    Tuple_Append(tuple, String_New(m->name));
  }

  //Object *so = Tuple_ToString(tuple, NULL);
  OB_DECREF(tuple);
  return NULL;
}

Klass Class_Klass = {
  OBJECT_HEAD_INIT(&Class_Klass)
  .name = "Class",
  .ob_free = klass_free,
};

Klass Any_Klass = {
  OBJECT_HEAD_INIT(&Class_Klass)
  .name = "Any",
  .flags = OB_FLAGS_TRAIT
};

static CFuncDef class_funcs[] = {
  {"__display__", "Lfmt.Formatter;", NULL, NULL},
  {NULL}
};

void Init_Class_Klass(void)
{
  Init_Klass_Self(&Class_Klass);
  Klass_Add_CMethods(&Class_Klass, class_funcs);
}

void Fini_Class_Klass(void)
{
  OB_ASSERT_REFCNT(&Class_Klass, 1);
  Fini_Klass(&Class_Klass);
}

static void object_mark(Object *ob)
{
  /* FIXME */
  UNUSED_PARAMETER(ob);
}

static Object *object_alloc(Klass *klazz)
{
  int size = sizeof(Object) + sizeof(Object *) * klazz->totalvars;
  Object *ob = GCalloc(size);
  Init_Object_Head(ob, klazz);
  return ob;
}

static void object_free(Object *ob)
{
  Klass *klazz = OB_KLASS(ob);
  Object **value = (Object **)(ob + 1);
  int size = klazz->totalvars;
  for (int i = 0; i < size; i++)
    OB_DECREF(value[i]);
  Log_Debug("object \x1b[34m%s@%x\x1b[0m freed",
            OB_KLASS(ob)->name, (int)(intptr_t)ob);
  GCfree(ob);
}

Klass *__Klass_New(char *name, Klass *base, Vector *traits)
{
  Klass *klazz = Malloc(sizeof(Klass));
  Init_Object_Head(klazz, &Class_Klass);
  klazz->name = AtomString_New(name).str;
  klazz->ob_alloc = object_alloc;
  klazz->ob_free  = object_free;
  klazz->ob_mark  = object_mark;
  Init_Klass(klazz, base, traits);
  return klazz;
}

int Klass_Add_Field(Klass *klazz, char *name, TypeDesc *desc)
{
  MNode *m = MNode_New(VAR_KIND, name, desc);
  if (m != NULL) {
    HashTable_Insert(&klazz->mtbl, &m->hnode);
    m->offset = klazz->nrvars++;
    klazz->totalvars++;
    return 0;
  }
  return -1;
}

int Klass_Add_Method(Klass *klazz, Object *code)
{
  CodeObject *co = (CodeObject *)code;
  MNode *m = MNode_New(FUNC_KIND, co->name, co->proto);
  if (m != NULL) {
    HashTable_Insert(&klazz->mtbl, &m->hnode);
    m->code = code;
    if (IsKCode(code)) {
      co->codeinfo->consts = klazz->consts;
    }
    return 0;
  }
  return -1;
}

int Klass_Add_Proto(Klass *klazz, char *name, TypeDesc *proto)
{
  MNode *m = MNode_New(PROTO_KIND, name, proto);
  if (m != NULL) {
    HashTable_Insert(&klazz->mtbl, &m->hnode);
    return 0;
  }
  return -1;
}

static MNode *__get_m(Object *ob, char *name, Klass **base, int *lroindex)
{
  Klass *klazz = *base;
  Vector *vec = &OB_KLASS(ob)->lro;
  LRONode *lro;
  int index = Vector_Size(vec) - 1;
  while (index >= 0) {
    lro = Vector_Get(vec, index);
    if (lro->klazz == klazz)
      break;
    index -= 1;
  }

  if (!memcmp(name, "super.", 6)) {
    index -= 1;
    name += 6;
  }

  MNode *m;
  while (index >= 0) {
    lro = Vector_Get(vec, index);
    m = MNode_Find(&lro->klazz->mtbl, name);
    if (m != NULL) {
      if (lroindex != NULL)
        *lroindex = lro->offset;
      *base = lro->klazz;
      return m;
    }
    index -= 1;
  }

  return NULL;
}

Object *Object_Get_Field(Object *ob, Klass *base, char *name)
{
  if (base == NULL)
    base = OB_KLASS(ob);
  int lroindex = -1;
  MNode *mnode = __get_m(ob, name, &base, &lroindex);
  if (mnode == NULL) {
    Log_Error("cannot find '%s', throw exception", name);
    assert(0);
    return NULL;
  }
  assert(mnode->kind == VAR_KIND);
  int index = lroindex + mnode->offset;
  Object **value = (Object **)(ob + 1);
  Log_Debug("get_field: '%s'(%d) in '%s(%s)'",
            name, index, base->name, OB_KLASS(ob)->name);
  return value[index];
}

void Object_Set_Field(Object *ob, Klass *base, char *name, Object *val)
{
  if (base == NULL)
    base = OB_KLASS(ob);
  int lroindex = -1;
  MNode *mnode = __get_m(ob, name, &base, &lroindex);
  if (mnode == NULL) {
    Log_Error("cannot find '%s', throw exception", name);
    assert(0);
    return;
  }
  assert(mnode->kind == VAR_KIND);
  int index = lroindex + mnode->offset;
  Object **value = (Object **)(ob + 1);
  Log_Debug("set_field: '%s'(%d) in '%s(%s)'",
            name, index, base->name, OB_KLASS(ob)->name);
  Object *old = value[index];
  value[index] = OB_INCREF(val);
  OB_DECREF(old);
}

Object *Object_Get_Method(Object *ob, Klass *base, char *name)
{
  if (base == NULL)
    base = OB_KLASS(ob);
  MNode *mnode = __get_m(ob, name, &base, NULL);
  if (mnode == NULL) {
    Log_Error("cannot find '%s', throw exception", name);
    assert(0);
    return NULL;
  }
  assert(mnode->kind == FUNC_KIND);
  Log_Debug("get_method: '%s' in '%s(%s)'",
            name, base->name, OB_KLASS(ob)->name);
  return mnode->code;
}

int Klass_Add_CMethods(Klass *klazz, CFuncDef *funcs)
{
  int res;
  CFuncDef *f = funcs;
  Object *code;

  while (f->name != NULL) {
    code = Code_From_CFunc(f);
    res = Klass_Add_Method(klazz, code);
    assert(!res);
    ++f;
  }
  return 0;
}
