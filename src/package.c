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

#include "package.h"
#include "tupleobject.h"
#include "stringobject.h"
#include "mem.h"

Package *Package_New(char *name)
{
  Package *pkg = mm_alloc(sizeof(Package));
  Init_Object_Head(pkg, &Package_Klass);
  pkg->name = AtomString_New(name);
  return pkg;
}

static void pkg_freefunc(HashNode *hnode, void *arg)
{
  MemberDef *m = container_of(hnode, MemberDef, hnode);
  MemberDef_Free(m);
}

void Package_Free(Package *pkg)
{
  HashTable_Free(pkg->table, pkg_freefunc, NULL);
  Tuple_Free(pkg->consts);
  mm_free(pkg);
}

static HashTable *__get_table(Package *pkg)
{
  if (!pkg->table)
    pkg->table = MemberDef_Build_HashTable();
  return pkg->table;
}

int Package_Add_Var(Package *pkg, char *name, TypeDesc *desc, int k)
{
  MemberDef *m = MemberDef_Var_New(__get_table(pkg), name, desc, k);
  if (m) {
    m->offset = pkg->varcnt++;
    return 0;
  }
  return -1;
}

int Package_Add_Func(Package *pkg, char *name, Object *code)
{
  MemberDef *m = MemberDef_Code_New(__get_table(pkg), name, code);
  if (m) {
    m->code = code;
    if (IS_KLANG_CODE(code)) {
      ((CodeObject *)code)->kl.consts = pkg->consts;
    }
    return 0;
  }
  return -1;
}

int Package_Add_Klass(Package *pkg, Klass *klazz, int trait)
{
  MemberDef *m;
  if (trait)
    m = MemberDef_Trait_New(klazz->name);
  else
    m = MemberDef_Class_New(klazz->name);
  int res = HashTable_Insert(__get_table(pkg), &m->hnode);
  if (res) {
    MemberDef_Free(m);
    return -1;
  }
  m->klazz = klazz;
  klazz->pkg = pkg;
  klazz->consts = pkg->consts;
  return 0;
}

MemberDef *Package_Find(Package *pkg, char *name)
{
  MemberDef *m = MemberDef_Find(__get_table(pkg), name);
  if (!m) {
    Log_Error("cannot find symbol '%s'", name);
  }
  return m;
}

int Package_Add_CFunctions(Package *pkg, FuncDef *funcs)
{
  int res;
  FuncDef *f = funcs;
  Object *code;
  while (f->name) {
    code = FuncDef_Build_Code(f);
    res = Package_Add_Func(pkg, f->name, code);
    assert(!res);
    ++f;
  }
  return 0;
}

static Object *package_tostring(Object *ob)
{
  OB_ASSERT_KLASS(ob, Package_Klass);
  Package *pkg = (Package *)ob;
  return MemberDef_HashTable_ToString(pkg->table);
}

Klass Package_Klass = {
  OBJECT_HEAD_INIT(&Klass_Klass)
  .name = "Package",
  .basesize = sizeof(Package),
  .ob_str = package_tostring
};

static Object *__package_display(Object *ob, Object *args)
{
  assert(!args);
  Object *s = package_tostring(ob);
  printf("%s\n", String_RawString(s));
  OB_DECREF(s);
  return NULL;
}

static FuncDef package_funcs[] = {
  {"Display", NULL, NULL, __package_display},
  {NULL}
};

void Init_Package_Klass(void)
{
  Klass_Add_CFunctions(&Package_Klass, package_funcs);
}

void Fini_Package_Klass(void)
{
  Fini_Klass(&Package_Klass);
}
