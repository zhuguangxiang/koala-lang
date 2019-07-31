/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_MODULE_OBJECT_H_
#define _KOALA_MODULE_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct moduleobject {
  OBJECT_HEAD
  /* module name */
  char *name;
  /* number of variables */
  int nrvars;
  /* meta table */
  struct hashmap *mtbl;
  /* value objects */
  struct vector *values;
  /* constant pool */
  struct vector *consts;
} ModuleObject;

extern TypeObject Module_Type;
extern TypeObject Module_Class_Type;
#define Module_Check(ob) (OB_TYPE(ob) == &Module_Type)
Object *Module_New(char *name);
void Module_Install(char *path, Object *ob);
int Module_Add_Const(Object *self, Object *ob);
int Module_Add_Var(Object *self, Object *ob);
int Module_Add_Method(Object *self, Object *ob);
int Module_Add_Type(Object *self, char *name, TypeObject *type);
int Module_Add_CFunc(Object *self, char *name, char *ptype, char *rtype,
                     cfunc func);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_MODULE_OBJECT_H_ */
