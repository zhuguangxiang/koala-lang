/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_EVAL_H_
#define _KOALA_EVAL_H_

#include "object.h"
#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_STACK_SIZE 64
#define MAX_FRAME_DEPTH 64

/* forward declaration */
struct frame;

/* one Task has one KoalaState */
typedef struct koalastate {
  /* linked in global kslist */
  struct list_head ksnode;
  /* call frame */
  struct frame *frame;
  /* frame depth */
  int depth;
  /* store per-task state*/
  Object *dict;
  /* stack top */
  int top;
  /* objects stack */
  Object *stack[MAX_STACK_SIZE];
} KoalaState;

Object *Koala_EvalCode(Object *self, Object *ob, Object *args);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_EVAL_H_ */
