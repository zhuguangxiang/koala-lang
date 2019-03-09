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

#ifndef _KOALA_PACKAGE_H_
#define _KOALA_PACKAGE_H_

#include "codeobject.h"
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
} Package;

extern Klass Pkg_Klass;
void Init_Pkg_Klass(void);
void Fini_Pkg_Klass(void);
Package *Pkg_New(char *name);
void Pkg_Free_Func(void *pkg, void *arg);
int Pkg_Add_Var(Package *pkg, char *name, TypeDesc *desc);
int Pkg_Add_Const(Package *pkg, char *name, TypeDesc *desc, Object *val);
int Pkg_Add_Func(Package *pkg, Object *code);
int Pkg_Add_Klass(Package *pkg, Klass *klazz, int trait);
#define Pkg_Add_Class(pkg, klazz) Pkg_Add_Klass(pkg, klazz, 0)
#define Pkg_Add_Trait(pkg, klazz) Pkg_Add_Klass(pkg, klazz, 1)
int Pkg_Add_CFunctions(Package *pkg, CFunctionDef *functions);
Package *Pkg_From_Image(KImage *image);
#define Pkg_Name(pkg) (((Package *)pkg)->name)

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_PACKAGE_H_ */
