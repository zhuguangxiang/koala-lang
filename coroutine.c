
#include "globalstate.h"
#include "methodobject.h"
#include "tupleobject.h"

Frame *frame_new(CoRoutine *rt)
{
  Object *ob = NULL;
  TValue_Parse(CoRoutine_StackPop(rt), 'O', &ob);
  assert(ob != NULL);
  OB_CHECK_KLASS(ob, Method_Klass);
  MethodObject *mo = (MethodObject *)ob;

  int nr_locals = 0;
  if (mo->type == METH_CFUNC) {
    nr_locals = 1 + mo->nr_paras;
  } else if (mo->type == METH_KFUNC) {
    nr_locals = 1 + mo->nr_paras + mo->nr_locals;
  } else {
    fprintf(stderr, "[ERROR]bug, unknown method type:%d\n", mo->type);
    assert(0);
  }

  Frame *frame = malloc(sizeof(*frame) + nr_locals * sizeof(TValue));
  frame->routine = rt;
  init_list_head(&frame->link);
  frame->func = ob;
  frame->pc = 0;
  frame->size = nr_locals;
  for (int i = 0; i < nr_locals; i++) {
    init_nil_tval(frame->locals[i]);
  }

  TValue v;
  int i = 0;
  while (!TVAL_ISANY(v = CoRoutine_StackPop(rt))) {
    frame->locals[i++] = v;
  }

  list_add(&frame->link, &rt->frame_list);

  return frame;
}

void frame_free(Frame *frame)
{
  free(frame);
}

void frame_loop(Frame *frame)
{
  MethodObject *mo = (MethodObject *)frame->func;
  if (mo->type == METH_CFUNC) {
    Object *ob = NULL;
    TValue_Parse(frame->locals[0], 'O', &ob);
    ob = mo->cf(ob, NULL);
    int size = 0;
    Tuple_Parse(ob, "i", &size);
    printf("size:%d\n", size);
  }
}

/*-------------------------------------------------------------------------*/

Object *CoRoutine_New(int prio, Object *func, Object *obj, Object *args)
{
  CoRoutine *rt = malloc(sizeof(*rt));
  init_object_head(rt, &CoRoutine_Klass);
  rt->state = STATE_INIT;
  rt->prio = prio;
  init_list_head(&rt->link);
  init_list_head(&rt->ready_link);
  init_list_head(&rt->frame_list);
  rt->top = -1;
  for (int i = 0; i < nr_elts(rt->stack); i++) {
    init_nil_tval(rt->stack[i]);
  }

  int size = Tuple_Size(args);
  assert(size < STACK_SIZE);

  for (int i = size - 1; i >= 0; i--) {
    CoRoutine_StackPush(rt, Tuple_Get(args, i));
  }

  CoRoutine_StackPush(rt, TValue_Build('O', obj));
  CoRoutine_StackPush(rt, TValue_Build('O', func));

  GState_Add_CoRoutine(rt);

  return (Object *)rt;
}

void CoRoutine_Run(CoRoutine *rt)
{
  Frame *f = NULL;

  if (list_empty(&rt->frame_list)) {
    f = frame_new(rt);
  } else {
    f = list_first_entry(&rt->frame_list, Frame, link);
  }

  frame_loop(f);
}

/*-------------------------------------------------------------------------*/

Klass CoRoutine_Klass = {
  OBJECT_HEAD_INIT(&Klass_Klass),
  .name  = "CoRoutine",
  .bsize = sizeof(CoRoutine),
};
