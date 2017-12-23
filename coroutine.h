
#ifndef _KOALA_COROUTINE_H_
#define _KOALA_COROUTINE_H_

#include "list.h"
#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct globalstate GlobalState;

#define STATE_INIT    1
#define STATE_DEAD    2
#define STATE_READY   3
#define STATE_RUNNING 4
#define STATE_SLEEP   5

#define STACK_SIZE    32

typedef struct coroutine {
  OBJECT_HEAD
  uint16 state;
  uint16 prio;
  GlobalState *gstate;
  struct list_head link;
  struct list_head ready_link;
  struct list_head frame_list;
  int top;
  TValue stack[STACK_SIZE];
} CoRoutine;

typedef struct frame {
  CoRoutine *routine;
  struct list_head link;
  Object *func;
  int pc;
  int size;
  TValue locals[0];
} Frame;

/* Exported symbols */
extern Klass CoRoutine_Klass;
Object *CoRoutine_New(int prio, Object *func, Object *obj, Object *args);
void CoRoutine_Run(CoRoutine *rt);

static inline TValue CoRoutine_StackPop(CoRoutine *rt)
{
  if (rt->top >= 0) return rt->stack[rt->top--];
  else return NIL_TVAL;
}

static inline void CoRoutine_StackPush(CoRoutine *rt, TValue v)
{
  assert(rt->top < nr_elts(rt->stack) - 1);
  rt->stack[++rt->top] = v;
}

static inline TValue CoRoutine_StackGet(CoRoutine *rt, int index)
{
  assert(index >= 0 && index <= rt->top);
  return rt->stack[index];
}

static inline int CoRoutine_StackSize(CoRoutine *rt)
{
  return rt->top + 1;
}

static inline void CoRoutine_StackClear(CoRoutine *rt)
{
  rt->top = -1;
}

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_COROUTINE_H_ */
