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

#ifndef _KOALA_MODULE_OBJECT_H_
#define _KOALA_MODULE_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct moduleobject {
  OBJECT_HEAD
  /* deployed path */
  char *path;
  /* module name */
  char *name;
  /* number of variables */
  int nrvars;
  /* meta table */
  HashMap *mtbl;
  /* value objects */
  Vector values;
  /* constant pool */
  Vector *consts;
  /* dlptr */
  void *dlptr;
} ModuleObject;

extern TypeObject module_type;
extern TypeObject Module_Class_Type;
#define module_check(ob) (OB_TYPE(ob) == &module_type)
#define MODULE_NAME(ob) (((ModuleObject *)ob)->name)
void init_module_type(void);
Object *module_new(char *name);
Object *Module_Lookup(Object *ob, char *name);
void module_install(char *path, Object *ob);
Object *module_load(char *path);
void module_uninstall(char *path);

void module_add_type(Object *self, TypeObject *type);

void module_add_var(Object *self, Object *ob);
void module_add_vardef(Object *self, FieldDef *f);
void module_add_vardefs(Object *self, FieldDef *def);

void Module_Add_Func(Object *self, Object *ob);
void Module_Add_FuncDef(Object *self, MethodDef *f);
void module_add_funcdefs(Object *self, MethodDef *def);

Object *module_get(Object *self, char *name);
void module_set(Object *self, char *name, Object *val);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_MODULE_OBJECT_H_ */
