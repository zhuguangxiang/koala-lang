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

Package *Pkg_New(char *name)
{
  Package *pkg = Malloc(sizeof(Package));
  Init_Object_Head(pkg, &Pkg_Klass);
  pkg->name = AtomString_New(name).str;
  Init_MTable(&pkg->mtbl);
  return pkg;
}

void Pkg_Free(Object *ob)
{
  Log_Debug("--------------------------");
  OB_ASSERT_KLASS(ob, Pkg_Klass);
  Package *pkg = (Package *)ob;
  Fini_MTable(&pkg->mtbl);
  OB_DECREF(pkg->consts);
  Log_Debug("package \x1b[32m%s\x1b[0m freed", pkg->name);
  Mfree(pkg);
}

static Object *pkg_tostring(Object *ob, Object *args)
{
  OB_ASSERT_KLASS(ob, Pkg_Klass);
  Package *pkg = (Package *)ob;
  return MTable_ToString(&pkg->mtbl);
}

Klass Pkg_Klass = {
  OBJECT_HEAD_INIT(&Klass_Klass)
  .name = "Package",
  .ob_free = Pkg_Free,
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

void Init_Pkg_Klass(void)
{
  Init_Klass(&Pkg_Klass, NULL);
  Klass_Add_CFunctions(&Pkg_Klass, pkg_funcs);
}

void Fini_Pkg_Klass(void)
{
  Fini_Klass(&Pkg_Klass);
}

int Pkg_Add_Const(Package *pkg, char *name, TypeDesc *desc, Object *val)
{
  MNode *m = MNode_New(CONST_KIND, name, desc);
  if (m != NULL) {
    HashTable_Insert(&pkg->mtbl, &m->hnode);
    OB_INCREF(val);
    m->value = val;
    return 0;
  }
  return -1;
}

int Pkg_Add_Var(Package *pkg, char *name, TypeDesc *desc)
{
  MNode *m = MNode_New(VAR_KIND, name, desc);
  if (m != NULL) {
    HashTable_Insert(&pkg->mtbl, &m->hnode);
    m->offset = pkg->nrvars++;
    return 0;
  }
  return -1;
}

int Pkg_Add_Func(Package *pkg, Object *code)
{
  CodeObject *co = (CodeObject *)code;
  MNode *m = MNode_New(FUNC_KIND, co->name, co->proto);
  if (m != NULL) {
    HashTable_Insert(&pkg->mtbl, &m->hnode);
    OB_INCREF(code);
    m->code = code;
    if (IsKCode(code)) {
      OB_INCREF(pkg->consts);
      co->codeinfo->consts = pkg->consts;
    }
    return 0;
  }
  return -1;
}

int Pkg_Add_Klass(Package *pkg, Klass *klazz, int trait)
{
  MemberKind kind = trait ? TRAIT_KIND : CLASS_KIND;
  MNode *m = MNode_New(kind, klazz->name, NULL);
  if (m != NULL) {
    HashTable_Insert(&pkg->mtbl, &m->hnode);
    OB_INCREF(klazz);
    m->klazz = klazz;
    klazz->owner = (Object *)pkg;
    klazz->consts = pkg->consts;
  }
  return 0;
}

Klass *Pkg_Get_Klass(Package *pkg, char *name, int trait)
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

int Pkg_Add_CFunctions(Package *pkg, CFunctionDef *functions)
{
  int res;
  CFunctionDef *f = functions;
  Object *code;
  while (f->name != NULL) {
    code = Code_From_CFunction(f);
    res = Pkg_Add_Func(pkg, code);
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
  case CONST_INT:
    ob = Integer_New(*(int64 *)data);
    break;
  case CONST_FLOAT:
    //ob = Float_New(item->fval);
    break;
  case CONST_BOOL:
    //ob = Bool_New(item->bval);
    break;
  case CONST_STRING:
    ob = String_New(data);
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
  if (konst)
    Pkg_Add_Const(arg, name, desc, NULL);
  else
    Pkg_Add_Var(arg, name, desc);
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
  if (index == info->index)
    Code_Add_LocVar(info->code, name, desc, pos);
}

static void __getfuncfn(char *name, TypeDesc *desc, int index, int type,
                        uint8 *codes, int size,void *arg)
{
  struct funcinfo *info = arg;
  Object *ob = Code_New(name, desc, codes, size);
  Pkg_Add_Func(info->pkg, ob);
  info->code = ob;
  info->index = index;
  KImage_Get_LocVars(info->image, __getlocvarfunc, arg);
}

static inline void load_functions(Package *pkg, KImage *image)
{
  struct funcinfo arg = {pkg, image, NULL, -1};
  KImage_Get_Funcs(image, __getfuncfn, &arg);
}

Package *Pkg_From_Image(KImage *image)
{
  char *name  = image->header.pkgname;
  Package *pkg = Pkg_New(name);
  load_consts(pkg, image);
  load_variables(pkg, image);
  load_functions(pkg, image);
  //load_traits(pkg, image->table);
  //load_classes(pkg, image->table);
  return pkg;
}
