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
#include "mem.h"
#include "log.h"
#include "stringobject.h"
#include "codeobject.h"
#include "tupleobject.h"

static void klass_mark(Object *ob)
{
  OB_ASSERT_KLASS(ob, Klass_Klass);
  //FIXME
}

static void klass_free(Object *ob)
{
  OB_ASSERT_KLASS(ob, Klass_Klass);
  //FXIME
}

static Object *klass_tostring(Object *ob)
{
  OB_ASSERT_KLASS(ob, Klass_Klass);
  Klass *klazz = (Klass *)ob;
  return MemberDef_HashTable_ToString(klazz->table);
}

Klass Klass_Klass = {
  OBJECT_HEAD_INIT(&Klass_Klass)
  .name = "Klass",
  .basesize = sizeof(Klass),
  .ob_mark  = klass_mark,
  .ob_free  = klass_free,
  .ob_str = klass_tostring,
};

Klass Any_Klass = {
  OBJECT_HEAD_INIT(&Klass_Klass)
  .name = "Any",
};

Klass Trait_Klass = {
  OBJECT_HEAD_INIT(&Klass_Klass)
  .name = "Trait",
  .basesize = sizeof(Klass),
  .ob_mark  = klass_mark,
  .ob_free  = klass_free,
};

Object *FuncDef_Build_Code(FuncDef *f)
{
  Vector *rdesc = String_To_TypeDescList(f->rdesc);
  Vector *pdesc = String_To_TypeDescList(f->pdesc);
  TypeDesc *proto = TypeDesc_New_Proto(pdesc, rdesc);
  return CCode_New(f->fn, proto);
}

int Klass_Add_CFunctions(Klass *klazz, FuncDef *funcs)
{
  int res;
  FuncDef *f = funcs;
  Object *code;

  while (f->name) {
    code = FuncDef_Build_Code(f);
    res = Klass_Add_Method(klazz, f->name, code);
    assert(!res);
    ++f;
  }
  return 0;
}

static void object_mark(Object *ob)
{
  //FIXME
  UNUSED_PARAMETER(ob);
}

static int objectsize(Klass *klazz)
{
  /* Base */
  int size = klazz->offset;

  /* Self */
  size += klazz->itemsize;

  Log_Debug("class(trait) '%s' has %d fields", klazz->name, size);

  return sizeof(Object) + sizeof(Object *) * size;
}

static Object *object_alloc(Klass *klazz)
{
  Object *ob = gc_alloc(objectsize(klazz));
  Init_Object_Head(ob, klazz);
  return ob;
}

static void object_free(Object *ob)
{
  Log_Debug("object of '%s' is freed", OB_KLASS(ob)->name);
  gc_free(ob);
}

static uint32 object_hash(Object *ob)
{
  return hash_uint32((intptr_t)ob, 32);
}

static int object_equal(Object *ob1, Object *ob2)
{
  return ob1 == ob2;
}

static Object *object_tostring(Object *ob)
{
  char buf[64];
  snprintf(buf, 63, "%s@%x", OB_KLASS(ob)->name, (uint16)(intptr_t)ob);
  return String_New(buf);
}

static LRONode *LRONode_New(int offset, Klass *klazz)
{
  LRONode *node = mm_alloc(sizeof(LRONode));
  node->offset = offset;
  node->klazz  = klazz;
  return node;
}

static int lro_find(Vector *lros, Klass *lro)
{
  LRONode *node;
  Vector_ForEach(node, lros) {
    if (node->klazz == lro)
      return 1;
  }
  return 0;
}

static int build_one_lro(Klass *klazz, Klass *base, int offset)
{
  LRONode *node;
  Vector_ForEach(node, &base->lro) {
    if (!lro_find(&klazz->lro, node->klazz)) {
      Vector_Append(&klazz->lro, LRONode_New(offset, node->klazz));
      offset += node->klazz->itemsize;
    }
  }

  if (!lro_find(&klazz->lro, base)) {
    Vector_Append(&klazz->lro, LRONode_New(offset, base));
    offset += base->itemsize;
  }

  return offset;
}

static int lro_build(Klass *klazz, Klass *base, Vector *traits)
{
  int offset = 0;
  if (base)
    offset = build_one_lro(klazz, base, offset);
  Klass *line;
  Vector_ForEach(line, traits) {
    /* must be Trait_Klass, means a trait not a class */
    OB_ASSERT_KLASS(line, Trait_Klass);
    offset = build_one_lro(klazz, line, offset);
  }
  return offset;
}

static void lro_debug(Klass *klazz)
{
  puts("++++++++++++++++line-order++++++++++++++++");
  LRONode *node;
  printf("  %s ->", klazz->name);
  Vector_ForEach_Reverse(node, &klazz->lro)
    printf("  %s ->", node->klazz->name);
  puts("  Any");
  puts("++++++++++++++++++++++++++++++++++++++++++");
}

Klass *Klass_New(char *name, Klass *base, Vector *traits, Klass *type)
{
  Klass *klazz = gc_alloc(sizeof(Klass));
  Init_Object_Head(klazz, type);
  klazz->name = strdup(name);
  klazz->basesize = sizeof(Object);
  klazz->itemsize = 0;

  klazz->ob_mark  = object_mark;
  klazz->ob_alloc = object_alloc;
  klazz->ob_free  = object_free,
  klazz->ob_hash  = object_hash;
  klazz->ob_equal = object_equal;
  klazz->ob_str   = object_tostring;

  Vector_Init(&klazz->lro);
  klazz->offset = lro_build(klazz, base, traits);
  lro_debug(klazz);

  return klazz;
}

static void free_lronode_func(void *item, void *arg)
{
  mm_free(item);
}

static void free_memberdef_func(HashNode *hnode, void *arg)
{
  MemberDef *m = container_of(hnode, MemberDef, hnode);
  switch (m->kind) {
    case MEMBER_VAR: {
      Log_Debug("field '%s' is freed", m->name);
      break;
    }
    case MEMBER_CODE: {
      Log_Debug("method '%s' is freed", m->name);
      CodeObject_Free(m->code);
      break;
    }
    default: {
      assert(0);
      break;
    }
  }
  MemberDef_Free(m);
}

void Klass_Free(Klass *klazz)
{
  Fini_Klass(klazz);
  Log_Debug("Klass '%s' is freed", klazz->name);
  free(klazz->name);
  gc_free(klazz);
}

void Fini_Klass(Klass *klazz)
{
  Log_Debug("Klass '%s' is finalized", klazz->name);
  Vector_Fini(&klazz->lro, free_lronode_func, NULL);
  if (klazz->table)
    HashTable_Free(klazz->table, free_memberdef_func, NULL);
}

static HashTable *__get_table(Klass *klazz)
{
  if (!klazz->table)
    klazz->table = MemberDef_Build_HashTable();
  return klazz->table;
}

int Klass_Add_Field(Klass *klazz, char *name, TypeDesc *desc)
{
  assert(OB_KLASS(klazz) == &Klass_Klass ||
         OB_KLASS(klazz) == &Trait_Klass);

  MemberDef *m = MemberDef_Var_New(__get_table(klazz), name, desc, 0);
  if (m) {
    m->offset = klazz->itemsize++;
    return 0;
  }
  return -1;
}

int Klass_Add_Method(Klass *klazz, char *name, Object *code)
{
  assert(OB_KLASS(klazz) == &Klass_Klass ||
         OB_KLASS(klazz) == &Trait_Klass);

  MemberDef *m = MemberDef_Code_New(__get_table(klazz), name, code);
  if (m) {
    m->code = code;
    if (CODE_IS_K(code))
      ((CodeObject *)code)->kf.consts = klazz->consts;
    return 0;
  }
  return -1;
}

int Klass_Add_Proto(Klass *klazz, char *name, TypeDesc *proto)
{
  assert(OB_KLASS(klazz) == &Klass_Klass ||
         OB_KLASS(klazz) == &Trait_Klass);

  MemberDef *member = MemberDef_Proto_New(name, proto);
  int res = HashTable_Insert(__get_table(klazz), &member->hnode);
  if (res) {
    MemberDef_Free(member);
    return -1;
  }
  return 0;
}

static int __get_lronode_index(Vector *lros, Klass *klazz)
{
  LRONode *item;
  Vector_ForEach_Reverse(item, lros) {
    if (item->klazz == klazz)
      return i;
  }
  return -1;
}

static MemberDef *__get_memberdef_lronode(Object *ob, char *name,
                                          Klass *klazz, LRONode *lronode)
{
  assert(OB_KLASS(OB_KLASS(ob)) == &Klass_Klass ||
         OB_KLASS(OB_KLASS(ob)) == &Trait_Klass);

  Klass *base = OB_KLASS(ob);
  LRONode *node;
  MemberDef *m;
  char *dot = strrchr(name, '.');
  char *mname = name;

  if (base == klazz && !dot) {
    /* search self */
    m = MemberDef_Find(__get_table(base), mname);
    if (m) {
      lronode->offset = klazz->offset;
      lronode->klazz = klazz;
      return m;
    }
  }

  int lro_index = Vector_Size(&base->lro) - 1;

  if (base != klazz) {
    lro_index = __get_lronode_index(&base->lro, klazz);
    if (dot)
      lro_index = lro_index - 1;
  }

  if (dot) {
    assert(!memcmp(name, "super", 5));
    mname = dot + 1;
  }

  while (lro_index >= 0) {
    node = Vector_Get(&base->lro, lro_index);
    assert(node);
    m = MemberDef_Find(__get_table(node->klazz), mname);
    if (m) {
      *lronode = *node;
      break;
    }
    --lro_index;
  }
  return m;
}

Object *Object_Get_Value(Object *ob, char *name, Klass *klazz)
{
  LRONode lronode;
  MemberDef *m = __get_memberdef_lronode(ob, name, klazz, &lronode);
  assert(m && m->kind == MEMBER_VAR);
  int index = lronode.offset + m->offset;
  Object **value = (Object **)(ob + 1);
  Log_Info("get_value '%s' index '%d' in klass '%s-%s'",
           name, index, klazz->name, lronode.klazz->name);
  return value[index];
}

Object *Object_Set_Value(Object *ob, char *name, Klass *klazz, Object *val)
{
  LRONode lronode;
  MemberDef *m = __get_memberdef_lronode(ob, name, klazz, &lronode);
  assert(m && m->kind == MEMBER_VAR);
  int index = lronode.offset + m->offset;
  Object **value = (Object **)(ob + 1);
  Log_Info("set_value '%s' index '%d' in klass '%s-%s'",
           name, index, klazz->name, lronode.klazz->name);
  Object *old = value[index];
  value[index] = val;
  return old;
}

Object *Object_Get_Method(Object *ob, char *name, Klass *klazz)
{
  LRONode lronode;
  MemberDef *m = __get_memberdef_lronode(ob, name, klazz, &lronode);
  assert(m && m->kind == MEMBER_CODE);
  Log_Info("get_method '%s' in klass '%s-%s'",
           name, klazz->name, lronode.klazz->name);
  return m->code;
}

int Object_Print(char *buf, int size, Object *ob)
{
  Object *s;
  char *str;
  int len;
  ob_strfunc tostrfunc = OB_KLASS(ob)->ob_str;
  if (tostrfunc != NULL) {
    s = tostrfunc(ob);
    str = String_RawString(s);
    len = String_Length(s);
    memcpy(buf, str, min(size, len));
    return len;
  } else {
    /* FIXME: find ToString() and call it */
    memcpy(buf, "(nil)", 5);
    return 5;
  }
}

MemberDef *MemberDef_New(int kind, char *name, TypeDesc *desc, int k)
{
  MemberDef *member = mm_alloc(sizeof(MemberDef));
  assert(member);
  Init_HashNode(&member->hnode, member);
  member->kind = kind;
  member->name = name;
  member->desc = desc;
  member->k = k;
  return member;
}

void MemberDef_Free(MemberDef *m)
{
  mm_free(m);
}

/* hash function of MemberDef, simply use it's name as key */
static uint32 __m_hash(MemberDef *m)
{
  return hash_string(m->name);
}

/* equal function of MemberDef, simply use it's name as key */
static int __m_equal(MemberDef *m1, MemberDef *m2)
{
  return !strcmp(m1->name, m2->name);
}

HashTable *MemberDef_Build_HashTable(void)
{
  return HashTable_New((hashfunc)__m_hash, (equalfunc)__m_equal);
}

MemberDef *MemberDef_Var_New(HashTable *table, char *name, TypeDesc *t, int k)
{
  MemberDef *m = MemberDef_New(MEMBER_VAR, name, t, k);
  int res = HashTable_Insert(table, &m->hnode);
  if (res) {
    MemberDef_Free(m);
    m = NULL;
  }
  return m;
}

MemberDef *MemberDef_Code_New(HashTable *table, char *name, Object *code)
{
  OB_ASSERT_KLASS(code, Code_Klass);
  CodeObject *co = (CodeObject *)code;
  MemberDef *m = MemberDef_New(MEMBER_CODE, name, co->proto, 0);
  int res = HashTable_Insert(table, &m->hnode);
  if (res) {
    MemberDef_Free(m);
    m = NULL;
  }
  return m;
}

MemberDef *MemberDef_Find(HashTable *table, char *name)
{
  MemberDef key = {.name = name};
  HashNode *hnode = HashTable_Find(table, &key);
  return hnode ? container_of(hnode, MemberDef, hnode) : NULL;
}

Object *MemberDef_HashTable_ToString(HashTable *table)
{
  int count = HashTable_Count_Nodes(table);
  Object *tuple = Tuple_New(count);

  struct list_head *pos;
  HashNode *hnode;
  int i = 0;
  list_for_each(pos, &table->head) {
    hnode = container_of(pos, HashNode, llink);
    MemberDef *m = container_of(hnode, MemberDef, hnode);
    Tuple_Set(tuple, i++, String_New(m->name));
  }

  Object *strob = OB_KLASS(tuple)->ob_str(tuple);
  OB_DECREF(tuple);
  return strob;
}
