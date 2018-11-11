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
#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * one package represents a klc file.
 * a klc can be compiled from multi kl files.
 */
typedef struct package {
  OBJECT_HEAD
  /* the package's name */
  char *name;
  /* variable, function, class and trait hash table */
  HashTable *table;
  /* const pool of this package */
  Object *consts;
  /* count of variables in the package */
  int varcnt;
  /* index of variables in global variable pool */
  int index;
} Package;

extern Klass Package_Klass;
void Init_Package_Klass(void);
void Fini_Package_Klass(void);
Package *Package_New(char *name);
void Package_Free(Package *pkg);
int Package_Add_Var(Package *pkg, char *name, TypeDesc *desc, int k);
int Package_Add_Func(Package *pkg, char *name, Object *code);
int Package_Add_Klass(Package *pkg, Klass *klazz, int trait);
#define Package_Add_Class(pkg, klazz) Package_Add_Klass(pkg, klazz, 0)
#define Package_Add_Trait(pkg, klazz) Package_Add_Klass(pkg, klazz, 1)
MemberDef *Package_Find(Package *pkg, char *name);
int Package_Add_CFunctions(Package *pkg, FuncDef *funcs);
#define Package_Name(pkg) ((pkg)->name)

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_PACKAGE_H_ */
