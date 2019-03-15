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
#include "intobject.h"
#include "mem.h"
#include "log.h"

LOGGER(0)

Package *Package_New(char *name)
{
  Package *pkg = Malloc(sizeof(Package));
  Init_Object_Head(pkg, &Package_Klass);
  pkg->name = AtomString_New(name).str;
  Init_MTable(&pkg->mtbl);
  return pkg;
}

void Package_Free(Object *ob)
{
  Log_Debug("------------------------------");
  OB_ASSERT_KLASS(ob, Package_Klass);
  Package *pkg = (Package *)ob;
  Fini_MTable(&pkg->mtbl);
  OB_DECREF(pkg->consts);
  Log_Debug("package \x1b[32m%-12s\x1b[0m freed", pkg->name);
  Mfree(pkg);
}

static Object *pkg_tostring(Object *ob, Object *args)
{
  OB_ASSERT_KLASS(ob, Package_Klass);
  Package *pkg = (Package *)ob;
  return MTable_ToString(&pkg->mtbl);
}

Klass Package_Klass = {
  OBJECT_HEAD_INIT(&Klass_Klass)
  .name = "Package",
  .ob_free = Package_Free,
  .ob_str = pkg_tostring
};

static Object *__pkg_display(Object *ob, Object *args)
{
  Object *s = pkg_tostring(ob, NULL);
  Log_Printf("%s\n", String_Raw(s));
  OB_DECREF(s);
  return NULL;
}

static CFunctionDef pkg_funcs[] = {
  {"Display", NULL, NULL, __pkg_display},
  {NULL}
};

void Init_Package_Klass(void)
{
  Init_Klass(&Package_Klass, NULL);
  Klass_Add_CFunctions(&Package_Klass, pkg_funcs);
}

void Fini_Package_Klass(void)
{
  Fini_Klass(&Package_Klass);
}

int Package_Add_Const(Package *pkg, char *name, TypeDesc *desc, Object *val)
{
  MNode *m = MNode_New(CONST_KIND, name, desc);
  if (m != NULL) {
    HashTable_Insert(&pkg->mtbl, &m->hnode);
    m->value = OB_INCREF(val);
    return 0;
  }
  return -1;
}

int Package_Add_Var(Package *pkg, char *name, TypeDesc *desc)
{
  MNode *m = MNode_New(VAR_KIND, name, desc);
  if (m != NULL) {
    HashTable_Insert(&pkg->mtbl, &m->hnode);
    m->offset = pkg->nrvars++;
    return 0;
  }
  return -1;
}

int Package_Add_Func(Package *pkg, Object *code)
{
  CodeObject *co = (CodeObject *)code;
  MNode *m = MNode_New(FUNC_KIND, co->name, co->proto);
  if (m != NULL) {
    HashTable_Insert(&pkg->mtbl, &m->hnode);
    m->code = OB_INCREF(code);
    if (IsKCode(code)) {
      co->codeinfo->consts = OB_INCREF(pkg->consts);
    }
    return 0;
  }
  return -1;
}

int Package_Add_Klass(Package *pkg, Klass *klazz, int trait)
{
  MemberKind kind = trait ? TRAIT_KIND : CLASS_KIND;
  MNode *m = MNode_New(kind, klazz->name, NULL);
  if (m != NULL) {
    HashTable_Insert(&pkg->mtbl, &m->hnode);
    m->klazz = OB_INCREF(klazz);
    klazz->owner = (Object *)pkg;
    klazz->consts = pkg->consts;
  }
  return 0;
}

Klass *Package_Get_Klass(Package *pkg, char *name, int trait)
{
  MNode *m = MNode_Find(&pkg->mtbl, name);
  if (m == NULL)
    return NULL;
  if (!trait && m->kind != CLASS_KIND)
    return NULL;
  if (trait && m->kind != TRAIT_KIND)
    return NULL;
  return m->klazz;
}

int Package_Add_CFunctions(Package *pkg, CFunctionDef *functions)
{
  int res;
  CFunctionDef *f = functions;
  Object *code;
  while (f->name != NULL) {
    code = Code_From_CFunction(f);
    res = Package_Add_Func(pkg, code);
    assert(!res);
    OB_DECREF(code);
    ++f;
  }
  return 0;
}

static void __getconstfunc(int type, void *data, int index, void *arg)
{
  Object *ob;
  Object *tuple = arg;
  switch (type) {
  case CONST_INT:
    ob = Integer_New(*(int64 *)data);
    Log_Debug("load const_int: %lld", Integer_Raw(ob));
    break;
  case CONST_FLOAT:
    //ob = Float_New(item->fval);
    break;
  case CONST_BOOL:
    //ob = Bool_New(item->bval);
    break;
  case CONST_STRING:
    ob = String_New(data);
    Log_Debug("load const_str: %s", String_Raw(ob));
    break;
  default:
    assert(0);
    break;
  }
  Tuple_Set(tuple, index, ob);
}

static void load_consts(Package *pkg, KImage *image)
{
  int size = KImage_Count_Consts(image);
  Object *tuple = Tuple_New(size);
  KImage_Get_Consts(image, __getconstfunc, tuple);
  pkg->consts = tuple;
}

static void __getvarfunc(char *name, TypeDesc *desc, int konst, void *arg)
{
  if (konst) {
    Log_Debug("load constant: %s", name);
    Package_Add_Const(arg, name, desc, NULL);
  } else {
    Log_Debug("load variable: %s", name);
    Package_Add_Var(arg, name, desc);
  }
}

static inline void load_variables(Package *pkg, KImage *image)
{
  KImage_Get_Vars(image, __getvarfunc, pkg);
}

struct funcinfo {
  Package *pkg;
  KImage *image;
  Object *code;
  int index;
};

static
void __getlocvarfunc(char *name, TypeDesc *desc, int pos, int index, void *arg)
{
  struct funcinfo *info = arg;
  if (index == info->index) {
    Log_Debug("load locvar: %s", name);
    Code_Add_LocVar(info->code, name, desc, pos);
  }
}

static void __getfuncfn(char *name, TypeDesc *desc, int index, int type,
                        uint8 *codes, int size,void *arg)
{
  if (codes != NULL) {
    Log_Debug("load function: %s", name);
    struct funcinfo *info = arg;
    Object *co = Code_New(name, desc, codes, size);
    Package_Add_Func(info->pkg, co);
    info->code = co;
    info->index = index;
    KImage_Get_LocVars(info->image, __getlocvarfunc, arg);
    OB_DECREF(co);
  }
}

static inline void load_functions(Package *pkg, KImage *image)
{
  struct funcinfo arg = {pkg, image, NULL, -1};
  KImage_Get_Funcs(image, __getfuncfn, &arg);
}

Package *Package_From_Image(KImage *image)
{
  char *name  = image->header.pkgname;
  Log_Debug("\x1b[34m------LOAD '%s' PACKAGE------------------\x1b[0m", name);
  Package *pkg = Package_New(name);
  load_consts(pkg, image);
  load_variables(pkg, image);
  load_functions(pkg, image);
  //load_traits(pkg, image->table);
  //load_classes(pkg, image->table);
  Log_Debug("\x1b[34m------END OF LOAD '%s' PACKAGE-----------\x1b[0m", name);
  return pkg;
}
