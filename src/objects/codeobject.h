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

#ifndef _KOALA_CODE_OBJECT_H_
#define _KOALA_CODE_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct codeinfo {
  /* constant pool */
  Object *consts;
  /* local variables */
  Vector locvec;
  /* codes' size */
  int size;
  /* instructions */
  uint8 *codes;
} CodeInfo;

typedef enum {
  /* koala code */
  KCODE_KIND = 1,
  /* clang func */
  CFUNC_KIND = 2,
} CodeKind;

typedef struct codeobject {
  OBJECT_HEAD
  /* code name */
  char *name;
  /* code proto */
  TypeDesc *proto;
  /* code's owner */
  Object *owner;
  /* code kind */
  CodeKind kind;
  /* cfunction or koala code */
  union {
    cfunc_t cfunc;
    CodeInfo *codeinfo;
  };
} CodeObject;

#if 0
typedef struct closure {
  OBJECT_HEAD
  /* closure name */
  char *name;
  /* closure proto */
  TypeDesc *proto;
  /* closure's owner */
  Object *owner;
  /* closure's free var table */
  HashTable *vtbl;
  /* koala code */
  CodeInfo code;
  /* free variables */
  Object *freevars[0];
} Closure;
#endif

extern Klass Code_Klass;
/* initialize Code class */
void Init_Code_Klass(void);
/* finalize Code class */
void Fini_Code_Klass(void);
/* new koala code object */
Object *Code_New(char *name, TypeDesc *proto, uint8 *codes, int size);
/* new clang func object */
Object *Code_From_CFunction(CFunctionDef *f);
/* free code object */
void Code_Free(Object *ob);
/* is koala code object? */
#define IsKCode(ob) (((CodeObject *)(ob))->kind == KCODE_KIND)
/* is clang code object? */
#define IsCFunc(ob) (((CodeObject *)(ob))->kind == CFUNC_KIND)
/* add local variables into the koala code object */
int Code_Add_LocVar(Object *ob, char *name, TypeDesc *desc, int index);
/* get the (koala & clang) code's args number */
int Code_Get_Argc(Object *ob);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_CODE_OBJECT_H_ */
