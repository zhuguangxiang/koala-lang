
#ifndef _KOALA_ROUTINE_H_
#define _KOALA_ROUTINE_H_

#include "object.h"
#include "thread.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STACK_SIZE  32

typedef struct frame Frame;

typedef struct routine {
  struct list_head link;
  Frame *frame;
  struct list_head frames;
  int top;
  TValue stack[STACK_SIZE];
} Routine;

struct frame {
  struct list_head link;
  Routine *rt;
  Object *code;
  int pc;
  int size;
  TValue locvars[0];
};

/* Exported APIs */
Routine *Routine_New(Object *code, Object *ob, Object *args);
void Routine_Run(Routine *rt);

/*-------------------------------------------------------------------------*/

static inline TValue rt_stack_top(Routine *rt)
{
  ASSERT(rt->top >= -1 && rt->top <= STACK_SIZE-1);
  if (rt->top >= 0) return rt->stack[rt->top];
  else return NilValue;
}

static inline TValue rt_stack_pop(Routine *rt)
{
  ASSERT(rt->top >= -1 && rt->top <= STACK_SIZE-1);
  if (rt->top >= 0) return rt->stack[rt->top--];
  else return NilValue;
}

static inline void rt_stack_push(Routine *rt, TValue *v)
{
  ASSERT(rt->top >= -1 && rt->top < STACK_SIZE-1);
  rt->stack[++rt->top] = *v;
}

static inline int rt_stack_size(Routine *rt)
{
  ASSERT(rt->top >= -1 && rt->top <= STACK_SIZE-1);
  return rt->top + 1;
}

static inline void rt_stack_init(Routine *rt)
{
  rt->top = -1;
  for (int i = 0; i <= STACK_SIZE-1; i++)
    initnilvalue(rt->stack + i);
}

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_ROUTINE_H_ */
