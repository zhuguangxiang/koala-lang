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

#include "packageobject.h"
#include "tupleobject.h"
#include "stringobject.h"
#include "intobject.h"
#include "mem.h"
#include "log.h"

LOGGER(0)

PackageObject *Package_New(char *name)
{
  PackageObject *pkg = Malloc(sizeof(PackageObject));
  Init_Object_Head(pkg, &Package_Klass);
  pkg->name = AtomString_New(name);
  return pkg;
}

static void pkg_freefunc(HashNode *hnode, void *arg)
{
  MemberDef *m = container_of(hnode, MemberDef, hnode);
  switch (m->kind) {
    case MEMBER_VAR: {
      Log_Debug("var '%s' is freed", m->name);
      break;
    }
    case MEMBER_CODE: {
      Log_Debug("func '%s' is freed", m->name);
      CodeObject_Free(m->code);
      break;
    }
    case MEMBER_CLASS: {
      Log_Debug("class '%s' is freed", m->name);
      break;
    }
    default: {
      assert(0);
      break;
    }
  }
  MemberDef_Free(m);
}

void Package_Free(PackageObject *pkg)
{
  HashTable_Free(pkg->table, pkg_freefunc, NULL);
  Tuple_Free(pkg->consts);
  Mfree(pkg);
}

static HashTable *__get_table(PackageObject *pkg)
{
  if (!pkg->table)
    pkg->table = MemberDef_Build_HashTable();
  return pkg->table;
}

int Package_Add_Var(PackageObject *pkg, char *name, TypeDesc *desc, int k)
{
  MemberDef *m = MemberDef_Var_New(__get_table(pkg), name, desc, k);
  if (m) {
    m->offset = pkg->varcnt++;
    return 0;
  }
  return -1;
}

int Package_Add_Func(PackageObject *pkg, char *name, Object *code)
{
  MemberDef *m = MemberDef_Code_New(__get_table(pkg), name, code);
  if (m) {
    m->code = code;
    if (CODE_IS_K(code)) {
      ((CodeObject *)code)->kf.consts = pkg->consts;
    }
    return 0;
  }
  return -1;
}

int Package_Add_Klass(PackageObject *pkg, Klass *klazz, int trait)
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

MemberDef *Package_Find_MemberDef(PackageObject *pkg, char *name)
{
  MemberDef *m = MemberDef_Find(__get_table(pkg), name);
  if (!m) {
    Log_Error("cannot find symbol '%s'", name);
  }
  return m;
}

int Package_Add_CFunctions(PackageObject *pkg, FuncDef *funcs)
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

static void __getconstfunc(int type, void *data, int index, void *arg)
{
  Object *ob;
  Object *tuple = arg;
  switch (type) {
    case CONST_INT: {
      int64 ival = *(int64 *)data;
      ob = Integer_New(ival);
      break;
    }
    case CONST_FLOAT: {
      //ob = Float_New(item->fval);
      break;
    }
    case CONST_BOOL: {
      //ob = Bool_New(item->bval);
      break;
    }
    case CONST_STRING: {
      ob = String_New(data);
      break;
    }
    default: {
      assert(0);
      break;
    }
  }
  Tuple_Set(tuple, index, ob);
}

static void load_consts(PackageObject *pkg, KImage *image)
{
  int size = KImage_Count_Consts(image);
  Object *tuple = Tuple_New(size);
  KImage_Get_Consts(image, __getconstfunc, tuple);
  pkg->consts = tuple;
}

static void __getvarfunc(char *name, TypeDesc *desc, int konst, void *arg)
{
  Package_Add_Var(arg, name, desc, konst);
}

static inline void load_variables(PackageObject *pkg, KImage *image)
{
  KImage_Get_Vars(image, __getvarfunc, pkg);
}

struct funcinfo {
  PackageObject *pkg;
  KImage *image;
  Object *code;
  int index;
};

static void __getlocvarfunc(char *name, TypeDesc *desc, int pos,
                            int index, void *arg)
{
  struct funcinfo *info = arg;
  if (index == info->index)
    KCode_Add_LocVar(info->code, name, desc, pos);
}

static void __getfuncfunc(char *name, TypeDesc *desc, uint8 *codes, int size,
                          int index, void *arg)
{
  struct funcinfo *info = arg;
  Object *ob = KCode_New(codes, size, desc);
  Package_Add_Func(info->pkg, name, ob);
  info->code = ob;
  info->index = index;
  KImage_Get_LocVars(info->image, __getlocvarfunc, arg);
}

static inline void load_functions(PackageObject *pkg, KImage *image)
{
  struct funcinfo arg = {pkg, image, NULL, -1};
  KImage_Get_Funcs(image, __getfuncfunc, &arg);
}

PackageObject *Package_From_Image(KImage *image, char *name)
{
  Log_Debug("new package '%s' from image", name);
  PackageObject *pkg = Package_New(name);
  load_consts(pkg, image);
  load_variables(pkg, image);
  load_functions(pkg, image);
  //load_traits(pkg, image->table);
  //load_classes(pkg, image->table);
  return pkg;
}

static Object *package_tostring(Object *ob)
{
  OB_ASSERT_KLASS(ob, Package_Klass);
  PackageObject *pkg = (PackageObject *)ob;
  return MemberDef_HashTable_ToString(pkg->table);
}

static void package_free(Object *ob)
{
  OB_ASSERT_KLASS(ob, Package_Klass);
  PackageObject *pkg = (PackageObject *)ob;
  Package_Free(pkg);
}

Klass Package_Klass = {
  OBJECT_HEAD_INIT(&Klass_Klass)
  .name = "PackageObject",
  .basesize = sizeof(PackageObject),
  .ob_free = package_free,
  .ob_str = package_tostring
};

static Object *__package_display(Object *ob, Object *args)
{
  assert(!args);
  Object *s = package_tostring(ob);
  Log_Printf("%s\n", String_RawString(s));
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
