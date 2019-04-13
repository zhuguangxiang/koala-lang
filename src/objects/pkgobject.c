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

#include "pkgobject.h"
#include "tupleobject.h"
#include "stringobject.h"
#include "intobject.h"
#include "codeobject.h"
#include "pkgnode.h"
#include "stringbuf.h"
#include "env.h"
#include "mem.h"
#include "log.h"

LOGGER(0)

/* package tree */
static PkgState pkgstate;
/*
 * global variables' pool(tuple's vector)
 * access by index stored in Package
 */
static VECTOR(variables);

Object *New_Package(char *name)
{
  PkgObject *pkg = Malloc(sizeof(PkgObject));
  Init_Object_Head(pkg, &Pkg_Klass);
  pkg->name = AtomString_New(name).str;
  Init_MTable(&pkg->mtbl);
  pkg->index = -1;
  return (Object *)pkg;
}

int Install_Package(char *path, Object *ob)
{
  OB_ASSERT_KLASS(ob, Pkg_Klass);
  PkgObject *pkg = (PkgObject *)ob;
  PkgNode *node = Add_PkgNode(&pkgstate, path, ob);
  if (node == NULL) {
    Log_Debug("add package '%s' failed", pkg->name);
    return -1;
  }

  if (pkg->nrvars != 0) {
    /* update package slot index */
    pkg->index = Vector_Size(&variables);
    Vector_Append(&variables, Tuple_New(pkg->nrvars));
  }
  return 0;
}

Object *Find_Package(char *path)
{
  PkgNode *node = Find_PkgNode(&pkgstate, path);
  if (node != NULL) {
    assert(node->leaf);
    return node->data;
  }

  DeclareStringBuf(buf);
  KImage *image;
  Object *pkg = NULL;
  Vector *vec = Env_Get_Vec(KOALA_PATH);
  char *pre;
  Vector_ForEach(pre, vec) {
    StringBuf_Format_CStr(buf, "#/#.klc", pre, path);
    image = KImage_Read_File(buf.data, 0);
    FiniStringBuf(buf);
    if (image != NULL) {
      pkg = Pkg_From_Image(image);
      Install_Package(path, pkg);
      KImage_Free(image);
      break;
    }
  }

  return pkg;
}

static void free_package(Object *ob)
{
  Log_Debug("------------------------------");
  OB_ASSERT_KLASS(ob, Pkg_Klass);
  PkgObject *pkg = (PkgObject *)ob;
  Fini_MTable(&pkg->mtbl);
  OB_DECREF(pkg->consts);
  Log_Debug("package \x1b[32m%-12s\x1b[0m freed", pkg->name);
  Mfree(pkg);
}

static Object *pkg_tostring(Object *ob, Object *args)
{
  OB_ASSERT_KLASS(ob, Pkg_Klass);
  PkgObject *pkg = (PkgObject *)ob;
  return MTable_ToString(&pkg->mtbl);
}

Klass Pkg_Klass = {
  OBJECT_HEAD_INIT(&Class_Klass)
  .name = "Package",
  .ob_free = free_package,
  //.ob_str = pkg_tostring
};

static Object *__pkg_display(Object *ob, Object *args)
{
  Object *s = pkg_tostring(ob, NULL);
  Log_Printf("%s\n", String_Raw(s));
  OB_DECREF(s);
  return NULL;
}

static CFuncDef pkg_funcs[] = {
  {"display", NULL, NULL, __pkg_display},
  {NULL}
};

void Init_Package_Klass(void)
{
  Init_PkgState(&pkgstate);
  Init_Klass_Self(&Pkg_Klass);
  Klass_Add_CMethods(&Pkg_Klass, pkg_funcs);
}

static void free_variables(void)
{
  Object *ob;
  Vector_ForEach(ob, &variables) {
    OB_ASSERT_KLASS(ob, Tuple_Klass);
    OB_DECREF(ob);
  }
  Vector_Fini_Self(&variables);
}

static void free_pkgnode_func(void *ob, void *arg)
{
  OB_DECREF(ob);
}

void Fini_Package_Klass(void)
{
  free_variables();
  Fini_PkgState(&pkgstate, free_pkgnode_func, NULL);
  Fini_Klass(&Pkg_Klass);
}

int Pkg_Add_Var(Object *ob, char *name, TypeDesc *desc)
{
  OB_ASSERT_KLASS(ob, Pkg_Klass);
  PkgObject *pkg = (PkgObject *)ob;
  MNode *m = MNode_New(VAR_KIND, name, desc);
  if (m != NULL) {
    HashTable_Insert(&pkg->mtbl, &m->hnode);
    m->offset = pkg->nrvars++;
    return 0;
  }
  return -1;
}

int Pkg_Add_Const(Object *ob, char *name, TypeDesc *desc, Object *val)
{
  OB_ASSERT_KLASS(ob, Pkg_Klass);
  PkgObject *pkg = (PkgObject *)ob;
  MNode *m = MNode_New(CONST_KIND, name, desc);
  if (m != NULL) {
    HashTable_Insert(&pkg->mtbl, &m->hnode);
    m->value = OB_INCREF(val);
    return 0;
  }
  return -1;
}

int Pkg_Add_Func(Object *ob, Object *code)
{
  OB_ASSERT_KLASS(ob, Pkg_Klass);
  PkgObject *pkg = (PkgObject *)ob;
  OB_ASSERT_KLASS(code, Code_Klass);
  CodeObject *co = (CodeObject *)code;
  MNode *m = MNode_New(FUNC_KIND, co->name, co->proto);
  if (m != NULL) {
    HashTable_Insert(&pkg->mtbl, &m->hnode);
    co->owner = (Object *)pkg;
    m->code = OB_INCREF(code);
    if (IsKCode(code)) {
      co->codeinfo->consts = OB_INCREF(pkg->consts);
    }
    return 0;
  }
  return -1;
}

int Pkg_Add_Class(Object *ob, Klass *klazz)
{
  OB_ASSERT_KLASS(ob, Pkg_Klass);
  PkgObject *pkg = (PkgObject *)ob;
  MNode *m = MNode_New(CLASS_KIND, klazz->name, NULL);
  if (m != NULL) {
    HashTable_Insert(&pkg->mtbl, &m->hnode);
    m->klazz = OB_INCREF(klazz);
    klazz->owner = (Object *)pkg;
    klazz->consts = OB_INCREF(pkg->consts);
  }
  return 0;
}

void Pkg_Set_Value(Object *ob, char *name, Object *value)
{
  OB_ASSERT_KLASS(ob, Pkg_Klass);
  PkgObject *pkg = (PkgObject *)ob;
  int index = pkg->index;
  MNode *m = MNode_Find(&pkg->mtbl, name);
  assert(m != NULL && m->kind == VAR_KIND);
  Log_Debug("set_var: '%s'(%d) in '%s'(%d)",
            name, m->offset, pkg->name, index);
  Object *tuple = Vector_Get(&variables, index);
  Tuple_Set(tuple, m->offset, value);
}

Object *Pkg_Get_Value(Object *ob, char *name)
{
  OB_ASSERT_KLASS(ob, Pkg_Klass);
  PkgObject *pkg = (PkgObject *)ob;
  int index = pkg->index;
  MNode *m = MNode_Find(&pkg->mtbl, name);
  assert(m != NULL);
  if (m->kind == VAR_KIND) {
    Log_Debug("get_var: '%s'(%d) in '%s'(%d)",
              name, m->offset, pkg->name, index);
    Object *tuple = Vector_Get(&variables, index);
    return Tuple_Get(tuple, m->offset);
  } else {
    assert(m->kind == CONST_KIND);
    Log_Debug("get_const: '%s' in '%s'", name, pkg->name);
    return m->value;
  }
}

Object *Pkg_Get_Func(Object *ob, char *name)
{
  OB_ASSERT_KLASS(ob, Pkg_Klass);
  PkgObject *pkg = (PkgObject *)ob;
  MNode *m = MNode_Find(&pkg->mtbl, name);
  if (m != NULL) {
    assert(m->kind == FUNC_KIND);
    Log_Debug("get_func: '%s' in '%s'", name, pkg->name);
    return m->code;
  } else {
    Log_Debug("get_func: '%s' is not found in '%s'", name, pkg->name);
    return NULL;
  }
}

Klass *Pkg_Get_Class(Object *ob, char *name)
{
  OB_ASSERT_KLASS(ob, Pkg_Klass);
  PkgObject *pkg = (PkgObject *)ob;
  MNode *m = MNode_Find(&pkg->mtbl, name);
  if (m == NULL || m->kind != CLASS_KIND)
    return NULL;
  return m->klazz;
}

int Pkg_Add_CFuns(Object *ob, CFuncDef *funcs)
{
  OB_ASSERT_KLASS(ob, Pkg_Klass);
  int res;
  CFuncDef *f = funcs;
  Object *code;
  while (f->name != NULL) {
    code = Code_From_CFunc(f);
    res = Pkg_Add_Func(ob, code);
    assert(!res);
    OB_DECREF(code);
    ++f;
  }
  return 0;
}

static void __getconst(int type, void *data, int index, void *arg)
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
  OB_DECREF(ob);
}

static void load_consts(Object *ob, KImage *image)
{
  int size = KImage_Count_Consts(image);
  Object *tuple = Tuple_New(size);
  KImage_Get_Consts(image, __getconst, tuple);
  PkgObject *pkg = (PkgObject *)ob;
  pkg->consts = tuple;
}

static void __getvar(char *name, TypeDesc *desc, int konst, void *arg)
{
  if (konst) {
    Log_Debug("load constant: %s", name);
    Pkg_Add_Const(arg, name, desc, NULL);
  } else {
    Log_Debug("load variable: %s", name);
    Pkg_Add_Var(arg, name, desc);
  }
}

static inline void load_vars(Object *pkg, KImage *image)
{
  KImage_Get_Vars(image, __getvar, pkg);
}

struct funcinfo {
  Object *pkg;
  KImage *image;
  Object *code;
  int index;
};

static void __getlocvar(char *name, TypeDesc *desc,
                        int pos, int index, void *arg)
{
  struct funcinfo *info = arg;
  if (index == info->index) {
    Log_Debug("load locvar: %s", name);
    Code_Add_LocVar(info->code, name, desc, pos);
  }
}

static void __getfunc(char *name, TypeDesc *desc, int index, int type,
                      uint8 *codes, int size,void *arg)
{
  if (codes != NULL) {
    Log_Debug("load function: %s", name);
    struct funcinfo *info = arg;
    Object *co = Code_New(name, desc, codes, size);
    Pkg_Add_Func(info->pkg, co);
    info->code = co;
    info->index = index;
    KImage_Get_LocVars(info->image, __getlocvar, arg);
    OB_DECREF(co);
  }
}

static inline void load_funcs(Object *ob, KImage *image)
{
  struct funcinfo arg = {ob, image, NULL, -1};
  KImage_Get_Funcs(image, __getfunc, &arg);
}

Object *Pkg_From_Image(KImage *image)
{
  char *name  = image->header.pkgname;
  Log_Debug("\x1b[34m------LOAD '%s' PACKAGE------------------\x1b[0m", name);
  Object *pkg = New_Package(name);
  load_consts(pkg, image);
  load_vars(pkg, image);
  load_funcs(pkg, image);
  //load_traits(pkg, image->table);
  //load_classes(pkg, image->table);
  Log_Debug("\x1b[34m------END OF LOAD '%s' PACKAGE-----------\x1b[0m", name);
  return pkg;
}
