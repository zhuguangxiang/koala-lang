
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
Object *Routine_New(Object *func, Object *args);
void Routine_Run(Routine *rt, short prio);
void Frame_Free(Frame *frame);

static inline TValue rt_stack_pop(Routine *rt)
{
  if (rt->top >= 0) return rt->stack[rt->top--];
  else return NilValue;
}

static inline void rt_stack_push(Routine *rt, TValue *v)
{
  ASSERT(rt->top < ROUTINE_STACK_SIZE - 1);
  rt->stack[++rt->top] = *v;
}

static inline TValue rt_stack_get(Routine *rt, int index)
{
  ASSERT(index >= 0 && index <= rt->top);
  return rt->stack[index];
}

static inline int rt_stack_size(Routine *rt)
{
  return rt->top + 1;
}

static inline void rt_stack_clear(Routine *rt)
{
  rt->top = -1;
}

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_ROUTINE_H_ */
