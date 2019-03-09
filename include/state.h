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

#ifndef _KOALA_STATE_H_
#define _KOALA_STATE_H_

#include "object.h"
#include "package.h"
#include "stack.h"
#include "options.h"
#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_STACK_SIZE 8
#define MAX_FRAME_DEPTH 16

/* one Task has one KoalaState */
typedef struct koalastate {
  /* linked in global kslist */
  struct list_head ksnode;

  /* top frame of func call stack */
  void *frame;
  int depth;

  /* store private data per-task */
  Object *map;

  /* calculating stack
   * 1.intermediate result of current frame
   * 2.passing arguments and results of function call
   */
  Stack stack;
  /* objects array for stack */
  Object *objs[MAX_STACK_SIZE];
} KoalaState;

void Koala_Initialize(void);
void Koala_Finalize(void);
int Koala_Add_Package(char *path, Package *pkg);
Package *Koala_Get_Package(char *path);
int Koala_Set_Value(Package *pkg, char *name, Object *value);
Object *Koala_Get_Value(Package *pkg, char *name);
Object *Koala_Get_Function(Package *pkg, char *name);
void Koala_RunTask(Object *code, Object *ob, Object *args);
void Koala_RunCode(Object *code, Object *ob, Object *args);
void Koala_Main(Options *opts);
#define Current_State() ((KoalaState *)task_get_private())

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_STATE_H_ */
