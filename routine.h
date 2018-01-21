
#ifndef _KOALA_ROUTINE_H_
#define _KOALA_ROUTINE_H_

#include "object.h"
#include "thread.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VALUE_STACK_SIZE  32
typedef struct valuestack {
  int top;
  TValue stack[VALUE_STACK_SIZE];
} ValueStack;

static inline TValue *ValueStack_Top(ValueStack *vs)
{
  ASSERT(vs->top >= -1 && vs->top < (nr_elts(vs->stack) - 1));
  if (vs->top >= 0) return vs->stack + vs->top;
  else return NULL;
}

static inline TValue ValueStack_Pop(ValueStack *vs)
{
  ASSERT(vs->top >= -1 && vs->top < (nr_elts(vs->stack) - 1));
  if (vs->top >= 0) return vs->stack[vs->top--];
  else return NilValue;
}

static inline void ValueStack_Push(ValueStack *vs, TValue *v)
{
  ASSERT(vs->top >= -1 && vs->top < (nr_elts(vs->stack) - 1));
  vs->stack[++vs->top] = *v;
}

static inline int ValueStack_Size(ValueStack *vs)
{
  ASSERT(vs->top >= -1 && vs->top < (nr_elts(vs->stack) - 1));
  return vs->top + 1;
}

static inline void Value_Stack_Init(ValueStack *vs)
{
  vs->top = -1;
  for (int i = 0; i < nr_elts(vs->stack); i++)
    initnilvalue(vs->stack + i);
}
/*-------------------------------------------------------------------------*/

typedef struct routine {
  OBJECT_HEAD
  struct task task;
  struct list_head link;
  struct list_head frames;
  Object *func;
  ValueStack stack;
} Routine;

#define FRAME_READY   1
#define FRAME_RUNNING 2
#define FRAME_EXIT    3

typedef struct frame {
  OBJECT_HEAD
  struct list_head link;
  int state;
  Routine *rt;
  Object *func;
  int pc;
  int size;
  TValue locals[0];
} Frame;

/* Exported APIs */
extern Klass Routine_Klass;
extern Klass Frame_Klass;
Routine *Routine_Create(Object *func, Object *obj, Object *args);
void Routine_Run(Routine *rt, short prio);
int Routine_State(Routine *rt);
void Frame_Loop(Frame *frame);
Frame *Frame_New(Object *func);
void Frame_Free(Frame *frame);
void Routine_Add_Frame(Routine *rt, Frame *f);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_ROUTINE_H_ */
