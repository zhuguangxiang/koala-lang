
#include "routine.h"
#include "koala.h"
#include "opcode.h"

#define TOP()   rt_stack_top(rt)
#define POP()   rt_stack_pop(rt)
#define PUSH(v) rt_stack_push(rt, (v))

static void frame_loop(Frame *frame);

/*-------------------------------------------------------------------------*/

static void frame_new(Routine *rt, Object *code)
{
  CodeObject *cob = OB_TYPE_OF(code, CodeObject, Code_Klass);

  int size = 0;
  if (CODE_ISKFUNC(code)) size = 1 + cob->kf.locvars;
  Frame *f = malloc(sizeof(Frame) + size * sizeof(TValue));
  init_list_head(&f->link);
  f->rt = rt;
  f->code = code;
  f->pc = 0;
  f->size = size;
  for (int i = 0; i < size; i++)
    initnilvalue(&f->locvars[i]);
  if (rt->frame != NULL)
    list_add(&rt->frame->link, &rt->frames);
  rt->frame = f;
}

static void frame_free(Frame *f)
{
  ASSERT(list_unlinked(&f->link));
  free(f);
}

static void restore_previous_frame(Frame *f)
{
  Routine *rt = f->rt;
  if (!list_empty(&rt->frames)) {
    Frame *nf = list_first_entry(&rt->frames, Frame, link);
    list_del(&nf->link);
    rt->frame = nf;
  } else {
    rt->frame = NULL;
  }

  frame_free(f);
}

static void start_cframe(Frame *f)
{
  Routine *rt = f->rt;
  CodeObject *code = (CodeObject *)f->code;
  TValue val;
  Object *obj;
  Object *args = NULL;

  /* Prepare parameters */
  int sz = rt_stack_size(rt);
  //FIXME: check arguments
  val = rt_stack_pop(rt);
  obj = VALUE_OBJECT(&val);
  if (sz > 1) args = Tuple_New(sz - 1);

  int i = 0;
  val = rt_stack_pop(rt);
  while (!VALUE_ISNIL(&val)) {
    Tuple_Set(args, i++, &val);
    val = rt_stack_pop(rt);
  }

  /* Call c function */
  Object *result = code->cf(obj, args);

  /* Save the result */
  sz = Tuple_Size(result);
  //FIXME: check results
  for (i = sz - 1; i >= 0; i--) {
    val = Tuple_Get(result, i);
    PUSH(&val);
  }

  /* Get previous frame and free old frame */
  restore_previous_frame(f);
}

static void start_kframe(Frame *f)
{
  Routine *rt = f->rt;
  //CodeObject *code = (CodeObject *)f->code;
  TValue val;

  /* Prepare parameters */
  int sz = rt_stack_size(rt);
  //ASSERT(sz == (KFunc_Argc(f->code) + 1) && (sz <= f->size));
  int count = min(sz, KFunc_Argc(f->code) + 1);
  int i = 0;
  while (count-- > 0) {
    val = rt_stack_pop(rt);
    ASSERT(!VALUE_ISNIL(&val));
    f->locvars[i++] = val;
  }

  /* Call frame_loop() to execute instructions */
  frame_loop(f);
}

/*-------------------------------------------------------------------------*/
/*
  Create a new routine
  Example:
    Run a function in current module: go func(123, "abc")
    or run one in external module: go extern_mod_name.func(123, "abc")
  Stack:
    Parameters are stored reversely in the stack, including a module.
 */
Routine *Routine_New(Object *code, Object *ob, Object *args)
{
  Routine *rt = malloc(sizeof(Routine));
  init_list_head(&rt->link);
  init_list_head(&rt->frames);
  rt_stack_init(rt);

  /* prepare parameters */
  TValue val;
  int size = Tuple_Size(args);
  for (int i = size - 1; i >= 0; i--) {
    val = Tuple_Get(args, i);
    VALUE_ASSERT(&val);
    PUSH(&val);
  }
  setobjvalue(&val, ob);
  PUSH(&val);

  /* new frame */
  frame_new(rt, code);

  return rt;
}

/*
static void routine_task_func(struct task *tsk)
{
  Routine *rt = tsk->arg;
  Frame *f = rt->frame;

  while (f != NULL) {
    if (CODE_ISCFUNC(f->code)) {
      start_cframe(f);
    } else if (CODE_ISKFUNC(f->code)) {
      if (f->pc == 0)
        start_kframe(f);
      else
        frame_loop(f);
    } else {
      ASSERT(0);
    }
    f = rt->frame;
  }
}
*/

void Routine_Run(Routine *rt)
{
  //task_init(&rt->task, "routine", prio, routine_task_func, rt);
  Frame *f = rt->frame;

  while (f != NULL) {
    if (CODE_ISCFUNC(f->code)) {
      start_cframe(f);
    } else if (CODE_ISKFUNC(f->code)) {
      if (f->pc == 0)
        start_kframe(f);
      else
        frame_loop(f);
    } else {
      ASSERT(0);
    }
    f = rt->frame;
  }
}

/*-------------------------------------------------------------------------*/

#define NEXT_CODE(f, codes) codes[f->pc++]

static inline uint8 fetch_byte(Frame *frame, CodeObject *code)
{
  ASSERT(frame->pc < code->kf.size);
  return NEXT_CODE(frame, code->kf.codes);
}

static inline uint16 fetch_2bytes(Frame *frame, CodeObject *code)
{
  ASSERT(frame->pc < code->kf.size);
  //endian?
  uint8 l = NEXT_CODE(frame, code->kf.codes);
  uint8 h = NEXT_CODE(frame, code->kf.codes);
  return (h << 8) + (l << 0);
}

static inline uint32 fetch_4bytes(Frame *frame, CodeObject *code)
{
  ASSERT(frame->pc < code->kf.size);
  //endian?
  uint8 l1 = NEXT_CODE(frame, code->kf.codes);
  uint8 l2 = NEXT_CODE(frame, code->kf.codes);
  uint8 h1 = NEXT_CODE(frame, code->kf.codes);
  uint8 h2 = NEXT_CODE(frame, code->kf.codes);
  return (h2 << 24) + (h1 << 16) + (l2 << 8) + (l1 << 0);
}

static inline uint8 fetch_code(Frame *frame, CodeObject *code)
{
  return fetch_byte(frame, code);
}

static TValue index_const(int index, STable *stbl)
{
  TValue res = NilValue;
  ConstItem *k = AtomTable_Get(stbl->atbl, ITEM_CONST, index);

  switch (k->type) {
    case CONST_INT: {
      setivalue(&res, k->ival);
      break;
    }
    case CONST_FLOAT: {
      setfltvalue(&res, k->fval);
      break;
    }
    case CONST_BOOL: {
      setbvalue(&res, k->bval);
      break;
    }
    case CONST_STRING: {
      StringItem *item;
      item = AtomTable_Get(stbl->atbl, ITEM_STRING, k->index);
      setcstrvalue(&res, item->data);
      break;
    }
    default: {
      ASSERT_MSG(0, "unknown const type:%d\n", k->type);
      break;
    }
  }
  return res;
}

static inline TValue load(Frame *f, int index)
{
  ASSERT(index < f->size);
  return f->locvars[index];
}

static inline void store(Frame *f, int index, TValue *val)
{
  ASSERT(index < f->size);
  f->locvars[index] = *val;
}

static Object *getcode(Object *ob, char *name)
{
  if (OB_CHECK_KLASS(ob, Module_Klass)) {
    return Module_Get_Function(ob, name);
  } else if (OB_CHECK_KLASS(OB_KLASS(ob), Klass_Klass)) {
    return Klass_Get_Method(OB_KLASS(ob), name);
  } else {
    ASSERT(0);
    return NULL;
  }
}

static void setfield(Object *ob, char *field, TValue *val)
{
  if (OB_CHECK_KLASS(ob, Module_Klass)) {
    Module_Set_Value(ob, field, val);
  } else if (OB_CHECK_KLASS(OB_KLASS(ob), Klass_Klass)) {
    ASSERT(0);
  } else {
    ASSERT(0);
  }
}

static TValue getfield(Object *ob, char *field)
{
  if (OB_CHECK_KLASS(ob, Module_Klass)) {
    return Module_Get_Value(ob, field);
  } else if (OB_CHECK_KLASS(OB_KLASS(ob), Klass_Klass)) {
    ASSERT(0);
    return NilValue;
  } else {
    ASSERT(0);
    return NilValue;
  }
}

int tonumber(TValue *v)
{
  UNUSED_PARAMETER(v);
  return 1;
}

static void frame_loop(Frame *frame)
{
  int loopflag = 1;
  Routine *rt = frame->rt;
  CodeObject *code = (CodeObject *)frame->code;
  STable *stbl = code->kf.stbl;

  uint8 inst;
  int32 index;
  TValue val;
  Object *ob;

  while (loopflag) {
    inst = fetch_code(frame, code);
    switch (inst) {
      case OP_HALT: {
        exit(0);
        break;
      }
      case OP_LOADK: {
        index = fetch_4bytes(frame, code);
        val = index_const(index, stbl);
        PUSH(&val);
        break;
      }
      case OP_LOADM: {
        index = fetch_4bytes(frame, code);
        val = index_const(index, stbl);
        char *path = VALUE_CSTR(&val);
        debug("load module '%s'", path);
        ob = Koala_Load_Module(path);
        ASSERT_PTR(ob);
        setobjvalue(&val, ob);
        PUSH(&val);
        break;
      }
      case OP_LOAD: {
        index = fetch_2bytes(frame, code);
        val = load(frame, index);
        PUSH(&val);
        break;
      }
      case OP_STORE: {
        index = fetch_2bytes(frame, code);
        val = POP();
        store(frame, index, &val);
        break;
      }
      case OP_GETFIELD: {
        index = fetch_4bytes(frame, code);
        val = index_const(index, stbl);
        char *field = VALUE_CSTR(&val);
        debug("getfield '%s'", field);
        val = POP();
        ob = VALUE_OBJECT(&val);
        val = getfield(ob, field);
        PUSH(&val);
        break;
      }
      case OP_SETFIELD: {
        index = fetch_4bytes(frame, code);
        val = index_const(index, stbl);
        char *field = VALUE_CSTR(&val);
        debug("setfield '%s'", field);
        val = POP();
        ob = VALUE_OBJECT(&val);
        val = POP();
        VALUE_ASSERT(&val);
        setfield(ob, field, &val);
        break;
      }
      case OP_CALL: {
        index = fetch_4bytes(frame, code);
        val = index_const(index, stbl);
        char *name = VALUE_CSTR(&val);
        debug("call %s()", name);
        val = TOP();
        Object *ob = VALUE_OBJECT(&val);
        frame_new(rt, getcode(ob, name));
        loopflag = 0;
        break;
      }
      case OP_RET: {
        restore_previous_frame(frame);
        loopflag = 0;
        break;
      }
      case OP_ADD: {
        TValue v2 = POP();
        TValue v1 = POP();
        val = NilValue;
        // v1 is left value
        if (VALUE_ISINT(&v1) && VALUE_ISINT(&v2)) {
          uint64 i = (uint64)VALUE_INT(&v1) + (uint64)VALUE_INT(&v2);
          setivalue(&val, i);
        } else if (tonumber(&v2) && tonumber(&v1)) {

        }
        PUSH(&val);
        break;
      }
      // case OP_SUB: {
      //   TValue v1 = POP();
      //   TValue v2 = POP();
      //   val = NilValue;
      //   // v2 is left value
      //   if (VALUE_ISINT(&v2) && VALUE_ISINT(&v1)) {
      //     uint64 i = (uint64)VALUE_INT(&v2) - (uint64)VALUE_INT(&v1);
      //     setivalue(&val, i);
      //   } else if (tonumber(&v2) && tonumber(&v1)) {

      //   }
      //   PUSH(&val);
      //   break;
      // }
      // case OP_MUL: {
      //   TValue v1 = POP();
      //   TValue v2 = POP();
      //   val = NilValue;
      //   // v2 is left value
      //   if (VALUE_ISINT(&v2) && VALUE_ISINT(&v1)) {
      //     uint64 i = (uint64)VALUE_INT(&v2) * (uint64)VALUE_INT(&v1);
      //     setivalue(&val, i);
      //   } else if (tonumber(&v2) && tonumber(&v1)) {

      //   }
      //   PUSH(&val);
      //   break;
      // }
      // case OP_DIV: {
      //   /* float division (always with floats) */
      //   TValue v1 = POP();
      //   TValue v2 = POP();
      //   val = NilValue;
      //   // v2 is left value
      //   if (tonumber(&v2) && tonumber(&v1)) {

      //   }
      //   PUSH(&val);
      //   break;
      // }
      default: {
        ASSERT_MSG(0, "unknown instruction:%d\n", inst);
      }
    }
  }
}
