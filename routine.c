
#include "routine.h"
#include "debug.h"
#include "methodobject.h"
#include "tupleobject.h"

Frame *Frame_New(Object *func)
{
  MethodObject *meth = OB_TYPE_OF(func, MethodObject, Method_Klass);

  int locals = 0;
  if (meth->type == METH_KFUNC) locals = 1 + meth->locals;
  Frame *f = malloc(sizeof(Frame) + locals * sizeof(TValue));
  f->state = FRAME_READY;
  init_list_head(&f->link);
  f->func = func;
  f->pc = 0;
  f->size = locals;
  for (int i = 0; i < locals; i++)
    initnilvalue(&f->locals[i]);
  return f;
}

void Frame_Free(Frame *f)
{
  ASSERT(f->state == FRAME_EXIT);
  list_del(&f->link);
  free(f);
}

void run_cframe(Frame *f)
{
  Routine *rt = f->rt;
  MethodObject *meth = (MethodObject *)f->func;
  TValue val;
  int sz;
  int i;

  /* Prepare parameters */
  ASSERT(f->state == FRAME_READY);
  sz = ValueStack_Size(&rt->stack);
  //ASSERT(sz == (meth->nr_args + 1));
  val = ValueStack_Pop(&rt->stack);
  Object *obj = VALUE_OBJECT(&val);
  Object *args = NULL;
  if (sz > 1) args = Tuple_New(sz - 1);
  i = 0;
  val = ValueStack_Pop(&rt->stack);
  while (!VALUE_ISNIL(&val)) {
    Tuple_Set(args, i++, &val);
    val = ValueStack_Pop(&rt->stack);
  }

  /* Call c function */
  f->state = FRAME_RUNNING;
  Object *result = meth->cf(obj, args);
  f->state = FRAME_EXIT;

  /* Save the result */
  if (result != NULL) {
    sz = Tuple_Size(result);
    ASSERT(sz == meth->rets);
    for (i = sz - 1; i >= 0; i--) {
      val = Tuple_Get(result, i);
      ValueStack_Push(&rt->stack, &val);
    }
  }

  /* Free this Frame */
  Frame_Free(f);
}

void run_kframe(Frame *f)
{
  Routine *rt = f->rt;
  MethodObject *meth = (MethodObject *)f->func;
  TValue val;
  int sz;
  int i;

  if (f->state == FRAME_READY) {
    /* Prepare parameters */
    sz = ValueStack_Size(&rt->stack);
    ASSERT((sz == (meth->args + 1)) && (sz <= f->size));
    i = 0;
    val = ValueStack_Pop(&rt->stack);
    while (!VALUE_ISNIL(&val)) {
      f->locals[i++] = val;
      val = ValueStack_Pop(&rt->stack);
    }
    f->state = FRAME_RUNNING;
  }

  /* Call frame_loop() to execute instructions */
  ASSERT(f->state == FRAME_RUNNING);
  Frame_Loop(f);
}

/*-------------------------------------------------------------------------*/
/*
  Create a new routine
  Example:
    Run a function in current module: go func(123, "abc")
    or run one in external module: go extern_mod_name.func(123, "abc")
    run an object's method: go obj.method(123,"abc")
  Stack:
    Parameters are stored reversely in the stack,
    including a module or an object.
 */
Routine *Routine_Create(Object *func, Object *obj, Object *args)
{
  OB_ASSERT_KLASS(func, Method_Klass);
  Routine *rt = malloc(sizeof(*rt));
  init_object_head(rt, &Routine_Klass);
  init_list_head(&rt->link);
  init_list_head(&rt->frames);
  Value_Stack_Init(&rt->stack);

  TValue val;
  int size = Tuple_Size(args);
  for (int i = size; i > 0; i--) {
    val = Tuple_Get(args, i - 1);
    VALUE_ASSERT(&val);
    ValueStack_Push(&rt->stack, &val);
  }
  setobjvalue(&val, obj);
  ValueStack_Push(&rt->stack, &val);
  rt->func = func;
  return rt;
}

static void routine_task_func(struct task *tsk)
{
  Routine *rt = tsk->arg;
  struct list_head *node;
  MethodObject *meth;
  Frame *f = Frame_New(rt->func);
  Routine_Add_Frame(rt, f);

  node = list_first(&rt->frames);
  while (node != NULL) {
    f = container_of(node, Frame, link);
    meth = (MethodObject *)f->func;
    if (meth->type == METH_CFUNC) {
      run_cframe(f);
    } else if (meth->type == METH_KFUNC) {
      run_kframe(f);
    } else {
      ASSERT_MSG("invalid method type:%d\n", meth->type);
    }
    node = list_first(&rt->frames);
  }
}

void Routine_Run(Routine *rt, short prio)
{
  task_init(&rt->task, "routine", prio, routine_task_func, rt);
}

int Routine_State(Routine *rt)
{
  return rt->task.state;
}

void Routine_Add_Frame(Routine *rt, Frame *f)
{
  ASSERT(list_unlinked(&f->link));
  list_add(&f->link, &rt->frames);
  f->rt = rt;
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
