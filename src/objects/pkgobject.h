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

#ifndef _KOALA_PKGOBJECT_H_
#define _KOALA_PKGOBJECT_H_

#include "object.h"
#include "image.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * one package represents a klc file.
 * a klc can be compiled from multi kl files.
 */
typedef struct pkgobject {
  OBJECT_HEAD
  /* package's name */
  char *name;
  /* member table */
  HashTable mtbl;
  /* constant pool */
  Object *consts;
  /* index of variable pool */
  int index;
  /* number of variables */
  int nrvars;
} PkgObject;

extern Klass Pkg_Klass;
#define Pkg_Name(pkg) (((PkgObject *)pkg)->name)
void Init_Package_Klass(void);
void Fini_Package_Klass(void);
Object *New_Package(char *name);
int Install_Package(char *path, Object *ob);
Object *Find_Package(char *path);
int Pkg_Add_Var(Object *ob, char *name, TypeDesc *desc);
int Pkg_Add_Const(Object *ob, char *name, TypeDesc *desc, Object *val);
int Pkg_Add_Func(Object *ob, Object *code);
int Pkg_Add_Class(Object *ob, Klass *klazz);
int Pkg_Add_Enum(Object *ob, Klass *klazz);
int Pkg_Add_EnumValue(Object *ob, char *name, Klass *klazz, Vector *types);
void Pkg_Set_Value(Object *ob, char *name, Object *value);
Object *Pkg_Get_Value(Object *ob, char *name);
Object *Pkg_Get_Func(Object *ob, char *name);
Klass *Pkg_Get_Class(Object *ob, char *name);
int Pkg_Add_CFuns(Object *ob, CFuncDef *funcs);
Object *Pkg_From_Image(KImage *image);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_PKGOBJECT_H_ */
