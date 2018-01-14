
#ifndef _KOALA_ROUTINE_H_
#define _KOALA_ROUTINE_H_

#include "object.h"
#include "thread.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ROUTINE_STACK_SIZE  32

typedef struct routine {
  OBJECT_HEAD
  struct task task;
  struct list_head rt_link;
  struct list_head frame_list;
  int top;
  TValue stack[ROUTINE_STACK_SIZE];
} Routine;

typedef struct frame {
  OBJECT_HEAD
  struct list_head link;
  Routine *rt;
  Object *func;
  int pc;
  int size;
  TValue locals[0];
} Frame;

/* Exported APIs */
extern Klass Routine_Klass;
extern Klass Frame_Klass;
Object *Routine_New(Object *func, Object *obj, Object *args);
void Routine_Run(Object *rt, short prio);
int Routine_State(Object *rt);
void Frame_Free(Frame *frame);
TValue rt_stack_pop(void *ob);
void rt_stack_push(void *ob, TValue *v);
TValue rt_stack_get(void *ob, int index);
int rt_stack_size(void *ob);
void rt_stack_clear(void *ob);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_ROUTINE_H_ */
