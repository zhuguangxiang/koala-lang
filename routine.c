
#include "routine.h"
#include "methodobject.h"
#include "tupleobject.h"

/*-------------------------------------------------------------------------*/
void koala_frame_loop(Frame *frame);

Frame *frame_new(Routine *rt, Object *func)
{
  OB_ASSERT_KLASS(func, Method_Klass);
  MethodObject *mo = (MethodObject *)func;

  int nr_locals = 0;
  if (mo->type == METH_CFUNC)
    nr_locals = 1 + mo->nr_args;
  else if (mo->type == METH_KFUNC)
    nr_locals = 1 + mo->nr_args + mo->nr_locals;
  else
    ASSERT(0);

  Frame *frame = malloc(sizeof(*frame) + nr_locals * sizeof(TValue));
  frame->rt = rt;
  init_list_head(&frame->link);
  frame->func = func;
  frame->pc = 0;
  frame->size = nr_locals;
  for (int i = 0; i < nr_locals; i++) {
    init_nil_value(&frame->locals[i]);
  }

  list_add(&frame->link, &rt->frame_list);

  return frame;
}

void Frame_Free(Frame *frame)
{
  free(frame);
}

void frame_run(Frame *frame)
{
  Routine *rt = frame->rt;
  MethodObject *mo = (MethodObject *)frame->func;
  if (mo->type == METH_CFUNC) {
    /* Get object and arguments */
    int stk_size = rt_stack_size(rt);
    ASSERT(stk_size > 0);
    TValue val = rt_stack_get(rt, 0);
    ASSERT(VALUE_ISOBJECT(&val));
    Object *obj = VALUE_OBJECT(&val);
    Object *args = Tuple_From_TValues(rt->stack + 1, stk_size - 1);
    rt_stack_clear(rt);

    /* Call C's function */
    Object *result = mo->cf(obj, args);

    /* Save result */
    if (result != NULL) {
      TValue val;
      int size = Tuple_Size(result);
      //ASSERT(size == mo->nr_rets);
      if (size > 0) {
        for (int i = 0; i < size; i++) {
          Tuple_Get(result, i, &val);
          rt_stack_push(rt, &val);
        }
      }
    }

    /* Free this Frame */
    list_del(&frame->link);
    Frame_Free(frame);
  } else if (mo->type == METH_KFUNC) {
    int stk_size = rt_stack_size(rt);
    int nr_paras = mo->nr_args;
    ASSERT(stk_size == nr_paras);
    set_object_value(&frame->locals[0], mo->owner);
    for (int i = 0; i < stk_size; i++)
      frame->locals[i+1] = rt_stack_get(rt, i);
    rt_stack_clear(rt);
    koala_frame_loop(frame);
  } else {
    ASSERT(0);
  }
}

/*-------------------------------------------------------------------------*/
// go func(123, "abc"), or go mod.func(), not allowed go obj.method(123,"abc")
Object *Routine_New(Object *func, Object *obj, Object *args)
{
  ASSERT_PTR(func);
  ASSERT_PTR(obj);
  OB_ASSERT_KLASS(func, Method_Klass);

  Routine *rt = malloc(sizeof(*rt));

  init_object_head(rt, &Routine_Klass);
  init_list_head(&rt->rt_link);
  init_list_head(&rt->frame_list);
  rt->top = -1;
  int i;
  for (i = 0; i < nr_elts(rt->stack); i++)
    init_nil_value(&rt->stack[i]);

  TValue val;
  set_object_value(&val, obj);
  rt_stack_push(rt, &val);

  if (args != NULL) {
    int size = Tuple_Size(args);
    for (i = 0; i < size; i++) {
      Tuple_Get(args, i, &val);
      ASSERT(!VALUE_ISNIL(&val));
      rt_stack_push(rt, &val);
    }
  }

  frame_new(rt, func);

  return (Object *)rt;
}

static void routine_task_func(struct task *tsk)
{
  Routine *rt = tsk->arg;
  struct list_head *node;
  while ((node = list_first(&rt->frame_list)) != NULL) {
    Frame *f = container_of(node, Frame, link);
    frame_run(f);
  }
}

void Routine_Run(Object *ob, short prio)
{
  OB_ASSERT_KLASS(ob, Routine_Klass);
  Routine *rt = (Routine *)ob;
  task_init(&rt->task, "routine", prio, routine_task_func, rt);
}

int Routine_State(Object *ob)
{
  OB_ASSERT_KLASS(ob, Routine_Klass);
  Routine *rt = (Routine *)ob;
  return rt->task.state;
}

/*-------------------------------------------------------------------------*/

TValue rt_stack_pop(void *ob)
{
  Routine *rt = ob;
  if (rt->top >= 0) return rt->stack[rt->top--];
  else return NilValue;
}

void rt_stack_push(void *ob, TValue *v)
{
  Routine *rt = ob;
  ASSERT(rt->top < ROUTINE_STACK_SIZE - 1);
  rt->stack[++rt->top] = *v;
}

TValue rt_stack_get(void *ob, int index)
{
  Routine *rt = ob;
  ASSERT(index >= 0 && index <= rt->top);
  return rt->stack[index];
}

int rt_stack_size(void *ob)
{
  Routine *rt = ob;
  return rt->top + 1;
}

void rt_stack_clear(void *ob)
{
  Routine *rt = ob;
  rt->top = -1;
}

/*-------------------------------------------------------------------------*/

Klass Routine_Klass = {
  OBJECT_HEAD_INIT(&Klass_Klass),
  .name  = "Routine",
  .bsize = sizeof(Routine),
};

Klass Frame_Klass = {
  OBJECT_HEAD_INIT(&Klass_Klass),
  .name  = "Frame",
  .bsize = sizeof(Frame),
};
