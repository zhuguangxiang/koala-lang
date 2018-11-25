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

#ifndef _KOALA_CODEOBJECT_H_
#define _KOALA_CODEOBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  /* koala code */
  CODE_KLANG = 1,
  /* clang code */
  CODE_CLANG = 2,
} CodeKind;

typedef struct codeobject {
  OBJECT_HEAD
  /* code prototype */
  TypeDesc *proto;
  /* one ob CODE_XXX */
  CodeKind kind;
  union {
    cfunc cl;
    struct {
      /* for const access, not free it */
      Object *consts;
      /* local variables */
      Vector locvec;
      /* codes' size */
      int size;
      /* instructions */
      uint8 *codes;
    } kl;
  };
} CodeObject;

extern Klass Code_Klass;
/* initialize Code class */
void Init_Code_Klass(void);
/* finalize Code class */
void Fini_Code_Klass(void);
/* new koala code object */
Object *KLang_Code_New(uint8 *codes, int size, TypeDesc *proto);
/* new clang code object */
Object *CLang_Code_New(cfunc cf, TypeDesc *proto);
/* free code object */
void CodeObject_Free(Object *ob);
/* is koala code object? */
#define IS_KLANG_CODE(code) (((CodeObject *)(code))->kind == CODE_KLANG)
/* is clang code object? */
#define IS_CLANG_CODE(code) (((CodeObject *)(code))->kind == CODE_CLANG)
/* add local variables into the koala code object */
int KCode_Add_LocVar(Object *ob, char *name, TypeDesc *desc, int pos);
/* get the (koala & clang) code's arg's number */
int Code_Get_NrArgs(Object *ob);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_CODEOBJECT_H_ */
