
#include "globalstate.h"
#include "methodobject.h"
#include "tupleobject.h"
#include "moduleobject.h"
#include "opcode.h"

/*-------------------------------------------------------------------------*/
static void frame_free(Frame *frame);

#define NEXT_CODE(f, codes) codes[f->pc++]

static inline Instruction fetch_code(Frame *frame, Instruction *codes)
{
  return NEXT_CODE(frame, codes);
}

static inline int fetch_arg(Frame *frame, Instruction *codes, int bytes)
{
  int arg = 0;
  if (bytes == 1) {
    arg = NEXT_CODE(frame, codes);
  } else if (bytes == 2) { //endian?
    int low = NEXT_CODE(frame, codes);
    int hight = NEXT_CODE(frame, codes);
    arg = (hight << 8) + low;
  } else {
    assert(0);
  }
  return arg;
}

static void koala_frame_loop(Frame *frame)
{
  int loop = 1;
  CoRoutine *rt = frame->routine;
  MethodObject *m = (MethodObject *)frame->func;
  TValue *k = m->kf.k;
  Instruction *codes = m->kf.codes;
  TValue *locals = frame->locals;

  Instruction inst;
  int arg;

  while (loop == 1 && rt->state == STATE_RUNNING) {
    inst = fetch_code(frame, codes);
    switch (inst) {
      case OP_LOADK: {
        arg = fetch_arg(frame, codes, 2);
        CoRoutine_StackPush(rt, k[arg]);
        break;
      }
      case OP_LOAD: {
        arg = fetch_arg(frame, codes, 1);
        CoRoutine_StackPush(rt, locals[arg]);
        break;
      }
      case OP_STORE: {
        printf("store\n");
        arg = fetch_arg(frame, codes, 1);
        break;
      }
      case OP_CALL: {
        printf("call\n");
        arg = fetch_arg(frame, codes, 1);
        char *s = NULL;
        TValue_Parse(k[arg], 's', &s);
        printf("%s\n", s);
        CoRoutine_StackPush(rt, TValue_Build('i', 12345));
        loop = 0;
        break;
      }
      case OP_RET: {
        list_del(&frame->link);
        frame_free(frame);
        loop = 0;
        break;
      }
      case OP_GO: {
        printf("gogo\n");
        arg = fetch_arg(frame, codes, 1);
        char *s = NULL;
        TValue_Parse(k[arg], 's', &s);
        printf("%s\n", s);
        ModuleObject *mod = NULL;
        TValue_Parse(locals[0], 'O', &mod);
        Module_Display((Object *)mod);
        TValue mv = TVAL_INIT_NIL;
        MethodObject *method = NULL;
        if (OB_KLASS(mod) == &Module_Klass) {
          printf("get method...\n");
          Module_Get((Object *)mod, s, NULL, &mv);
          TValue_Parse(mv, 'O', &method);
        }
        printf("gogo...\n");
        Object *args = Tuple_New(method->nr_args);
        for (int i = 0; i < method->nr_args; i++) {
          printf("gogo111...\n");
          Tuple_Set(args, i, CoRoutine_StackPop(rt));
        }
        printf("gogo333...\n");
        Object *ob = CoRoutine_New(1, (Object *)method, (Object *)mod, args);
        sleep(3);
        assert(((CoRoutine *)ob)->state == STATE_DEAD);
        OB_CHECK_KLASS(ob, CoRoutine_Klass);
        CoRoutine *rt = (CoRoutine *)ob;
        int a = 0, b = 0, c = 0;
        TValue_Parse(CoRoutine_StackGet(rt, 0), 'i', &a);
        TValue_Parse(CoRoutine_StackGet(rt, 1), 'i', &b);
        TValue_Parse(CoRoutine_StackGet(rt, 2), 'i', &c);
        printf("a = %d, b = %d, a + b = %d\n", a, b, c);
        printf("%s\n", s);
        break;
      }
      case OP_ADD: {
        TValue v1 = CoRoutine_StackPop(rt);
        TValue v2 = CoRoutine_StackPop(rt);
        if (TVAL_ISINT(v1) && TVAL_ISINT(v2)) {
          uint64 v = INT_TVAL(v1) + INT_TVAL(v2);
          CoRoutine_StackPush(rt, TValue_Build('l', v));
        } else if (TVAL_ISOBJ(v1) && TVAL_ISOBJ(v2)) {

        } else {
          fprintf(stderr, "[ERROR]unsupported add arithmetic operation\n");
          assert(0);
        }
        break;
      }
      case OP_SUB: {
        break;
      }
      default: {
        fprintf(stderr, "[ERROR]unknown instruction:%d\n", inst);
        assert(0);
      }
    }
  }
}

/*-------------------------------------------------------------------------*/

static Frame *frame_new(CoRoutine *rt)
{
  Object *ob = NULL;
  TValue_Parse(CoRoutine_StackPop(rt), 'O', &ob);
  assert(ob != NULL);
  OB_CHECK_KLASS(ob, Method_Klass);
  MethodObject *mo = (MethodObject *)ob;

  int nr_locals = 0;
  if (mo->type == METH_CFUNC) {
    nr_locals = 1 + mo->nr_args;
  } else if (mo->type == METH_KFUNC) {
    nr_locals = 1 + mo->nr_args + mo->nr_locals;
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

static void frame_free(Frame *frame)
{
  free(frame);
}

static void frame_loop(Frame *frame)
{
  MethodObject *mo = (MethodObject *)frame->func;
  if (mo->type == METH_CFUNC) {
    // Get object and arguments
    Object *ob = NULL, *args = NULL;
    if (frame->size > 0) {
      TValue_Parse(frame->locals[0], 'O', &ob);
      args = Tuple_From_TValues(&frame->locals[1], frame->size - 1);
    }

    // Call C lanuage's function
    ob = mo->cf(ob, args);

    //Save result
    if (ob != NULL) {
      CoRoutine *rt = frame->routine;
      for (int i = 0; i < Tuple_Size(ob); i++) {
        CoRoutine_StackPush(rt, Tuple_Get(ob, i));
      }
    }

    // Free this Frame
    list_del(&frame->link);
    frame_free(frame);

  } else if (mo->type == METH_KFUNC) {
    koala_frame_loop(frame);
  } else {
    assert(0);
  }
}

/*-------------------------------------------------------------------------*/
// go func(123, "abc"), not allowed go obj.method(123,"abc")
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

  frame_new(rt);

  GState_Add_CoRoutine(rt);

  return (Object *)rt;
}

void CoRoutine_Run(CoRoutine *rt)
{
  Frame *f = NULL;

  assert(rt->state == STATE_RUNNING);

  while (!list_empty(&rt->frame_list)) {
    f = list_first_entry(&rt->frame_list, Frame, link);
    frame_loop(f);
  }

  printf("routine is dead\n");
  rt->state = STATE_DEAD;
}

/*-------------------------------------------------------------------------*/

Klass CoRoutine_Klass = {
  OBJECT_HEAD_INIT(&Klass_Klass),
  .name  = "CoRoutine",
  .bsize = sizeof(CoRoutine),
};
