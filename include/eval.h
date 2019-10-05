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

#ifndef _KOALA_EVAL_H_
#define _KOALA_EVAL_H_

#include "codeobject.h"
#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_STACK_SIZE 64
#define MAX_FRAME_DEPTH 64

/* forward declaration */
typedef struct callframe CallFrame;
typedef struct koalastate KoalaState;

struct callframe {
  /* back call frame */
  CallFrame *back;
  /* KoalaState */
  KoalaState *ks;
  /* code */
  CodeObject *code;
  /* code index */
  int index;
  /* free variables */
  Vector *freevars;
  /* local variables' size */
  int size;
  /* local variables's array */
  Object *locvars[1];
};

/* one Task has one KoalaState */
struct koalastate {
  /* linked in global kslist */
  struct list_head ksnode;
  /* call frame */
  CallFrame *frame;
  /* frame depth */
  int depth;
  /* store per-task state*/
  Object *dict;
  /* stack top */
  int top;
  /* objects stack */
  Object *stack[MAX_STACK_SIZE];
};

Object *koala_evalcode(Object *self, Object *ob, Object *args);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_EVAL_H_ */
