
#include "routine.h"
#include "moduleobject.h"
#include "codeobject.h"
#include "tupleobject.h"
#include "stringobject.h"
#include "koalastate.h"
#include "listobject.h"
#include "klc.h"
#include "opcode.h"
#include "log.h"

#define TOP()   rt_stack_top(rt)
#define POP()   rt_stack_pop(rt)
#define PUSH(v) rt_stack_push(rt, (v))

static void frame_loop(Frame *frame);

/*-------------------------------------------------------------------------*/

static void frame_new(Routine *rt, Object *ob, Object *cob, int argc)
{
  CodeObject *code = OB_TYPE_OF(cob, CodeObject, Code_Klass);

  int size = 0;
  if (CODE_ISKFUNC(code)) size = code->kf.locvars;

  Frame *f = malloc(sizeof(Frame) + size * sizeof(TValue));
  init_list_head(&f->link);
  f->rt = rt;
  f->argc = argc;
  f->code = (Object *)code;
  f->pc = 0;
  f->size = size;
  for (int i = 0; i < size; i++)
    initnilvalue(&f->locvars[i]);

  debug("loc vars : %d", size);
  if (size > 0 && Vector_Size(&code->kf.locvec) > 0) {
    MemberDef *item;
    char buf[64];
    debug("------Set LocVar's Type-------");
    Vector_ForEach(item, &code->kf.locvec) {
      Type_ToString(item->desc, buf);
      debug("set [%d] as '%s' type", item->offset, buf);
      assert(item->offset >= 0 && item->offset < size);
      TValue_Set_TypeDesc(f->locvars + item->offset, item->desc, ob);
    }
    debug("------Set LocVar's Type End---");
  }

  if (rt->frame)
    list_add(&rt->frame->link, &rt->frames);
  rt->frame = f;
}

static void frame_free(Frame *f)
{
  assert(list_unlinked(&f->link));
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
  assert(f->argc <= sz);

  val = rt_stack_pop(rt);
  obj = val.ob;
  if (f->argc > 0) args = Tuple_New(f->argc);

  int count = f->argc;
  int i = 0;
  while (count-- > 0) {
    val = rt_stack_pop(rt);
    VALUE_ASSERT(&val);
    Tuple_Set(args, i++, &val);
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
  assert(f->argc <= sz);
  //assert(sz == (KFunc_Argc(f->code) + 1) && (sz <= f->size));
  int count = min(f->argc, Func_Argc(f->code)) + 1;
  int i = 0;
  while (count-- > 0) {
    val = rt_stack_pop(rt);
    VALUE_ASSERT(&val);
    TValue_Set_Value(f->locvars + i, &val);
    i++;
  }

  /* Call frame_loop() to execute instructions */
  frame_loop(f);
}

/*-------------------------------------------------------------------------*/
int Routine_Init(Routine *rt)
{
  TValue *stack = malloc(STACK_SIZE * sizeof(TValue));
  if (!stack) {
    assert(0);
    return -1;
  }

  rt->stack = stack;
  init_list_head(&rt->link);
  rt->frame = NULL;
  init_list_head(&rt->frames);
  rt_stack_init(rt);
  list_add_tail(&rt->link, &gs.routines);
  return 0;
}

void Routine_Fini(Routine *rt)
{
  list_del(&rt->link);
  assert(list_empty(&rt->frames));
  free(rt->stack);
}

/*
  Create a new routine
  Example:
    Run a function in current module: go func(123, "abc")
    or run one in external module: go extern_mod_name.func(123, "abc")
  Stack:
    Parameters are stored reversely in the stack, including a module.
 */
#if 0
Routine *Routine_New(Object *code, Object *ob, Object *args)
{
  Routine *rt = calloc(1, sizeof(Routine));
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
  frame_new(rt, ob, code, size);

  list_add_tail(&rt->link, &gs.routines);

  return rt;
}
#endif

/*
static void routine_task_func(struct task *tsk)
{
  Routine *rt = tsk->arg;
  Frame *f = rt->frame;

  while (f) {
    if (CODE_ISCFUNC(f->code)) {
      start_cframe(f);
    } else if (CODE_ISKFUNC(f->code)) {
      if (f->pc == 0)
        start_kframe(f);
      else
        frame_loop(f);
    } else {
      assert(0);
    }
    f = rt->frame;
  }
}
*/

void Routine_Run(Routine *rt, Object *code, Object *ob, Object *args)
{
  /* prepare arguments */
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
  frame_new(rt, ob, code, size);

  Frame *f = rt->frame;

  while (f) {
    if (CODE_ISCFUNC(f->code)) {
      start_cframe(f);
    } else if (CODE_ISKFUNC(f->code)) {
      if (f->pc == 0)
        start_kframe(f);
      else
        frame_loop(f);
    } else {
      assert(0);
    }
    f = rt->frame;
  }
}

/*-------------------------------------------------------------------------*/

#define NEXT_CODE(f, codes) codes[f->pc++]

static inline uint8 fetch_byte(Frame *frame, CodeObject *code)
{
  assert(frame->pc < code->kf.size);
  return NEXT_CODE(frame, code->kf.codes);
}

static inline uint16 fetch_2bytes(Frame *frame, CodeObject *code)
{
  assert(frame->pc < code->kf.size);
  //endian?
  uint8 l = NEXT_CODE(frame, code->kf.codes);
  uint8 h = NEXT_CODE(frame, code->kf.codes);
  return (h << 8) + (l << 0);
}

static inline uint32 fetch_4bytes(Frame *frame, CodeObject *code)
{
  assert(frame->pc < code->kf.size);
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

static inline TValue index_const(int index, Object *consts)
{
  return Tuple_Get(consts, index);
}

static inline TValue load(Frame *f, int index)
{
  assert(index < f->size);
  return f->locvars[index];
}

static inline void store(Frame *f, int index, TValue *val)
{
  VALUE_ASSERT(val);
  assert(index < f->size);
  assert(!TValue_Check(f->locvars + index, val));
  TValue *v = &f->locvars[index];
  if (v->klazz == &Int_Klass) {
    v->ival = val->ival;
  } else if (v->klazz == &Float_Klass) {
    v->fval = val->fval;
  } else if (v->klazz == &Bool_Klass) {
    v->bval = val->bval;
  } else {
    v->ob = val->ob;
  }
}

static Object *getcode(Object *ob, char *name, Object **rob)
{
  Object *code = NULL;
  if (OB_CHECK_KLASS(ob, Module_Klass)) {
    debug("getcode '%s' in module", name);
    code = Module_Get_Function(ob, name);
    *rob = ob;
    if (!code) {
      debug("'%s' is not found", name);
      assert(!strcmp(name, "__init__"));
    }
  } else {
    Check_Klass(OB_KLASS(ob));
    debug("getcode '%s' in class %s", name, OB_KLASS(ob)->name);
    code = Object_Get_Method(ob, name, rob);
    if (!code) {
      debug("'%s' is not found", name);
      assert(!strcmp(name, "__init__"));
    }
  }
  return code;
}

static void setfield(Object *ob, char *field, TValue *val)
{
  if (OB_CHECK_KLASS(ob, Module_Klass)) {
    Module_Set_Value(ob, field, val);
  } else {
    Check_Klass(OB_KLASS(ob));
    int res = Object_Set_Value(ob, field, val);
    assert(!res);
  }
}

static TValue getfield(Object *ob, char *field)
{
  if (OB_CHECK_KLASS(ob, Module_Klass)) {
    return Module_Get_Value(ob, field);
  } else {
    Check_Klass(OB_KLASS(ob));
    TValue val = Object_Get_Value(ob, field);
    VALUE_ASSERT(&val);
    return val;
  }
}

// static int check_virtual_call(TValue *val, char *name)
// {
// 	VALUE_ASSERT_OBJECT(val);
// 	Klass *klazz = val->klazz;
// 	assert(klazz);
// 	//module not check
// 	if (klazz == &Module_Klass) return 0;

// 	Check_Klass(klazz);

// 	if (strchr(name, '.')) return 0;
// 	Symbol *sym = Klass_Get_Symbol(klazz, name);
// 	if (!sym) {
// 		error("cannot find '%s' in '%s' class", name, klazz->name);
// 		return -1;
// 	}
// 	if (sym->kind != SYM_PROTO && sym->kind != SYM_IPROTO) {
// 		error("symbol '%s' is not method", name);
// 		return -1;
// 	}
// 	return 0;
// }

static void check_args(Routine *rt, int argc, TypeDesc *proto, char *name)
{
  assert(proto->kind == TYPE_PROTO);
  int size = Vector_Size(proto->proto.arg);
  if (argc != size) {
    error("%s argc: expected %d, but %d", name, size, argc);
    exit(-1);
  }

  TValue *val;
  TypeDesc *desc;
  int pos = rt->top - 1;
  for (int i = 0; i < size; i++) {
    val = rt_stack_get(rt, pos--);
    desc = Vector_Get(proto->proto.arg, i);
    if (TValue_Check_TypeDesc(val, desc)) {
      error("'%s' args type check failed", name);
      //exit(-1);
    }
  }
}

int tonumber(TValue *v)
{
  UNUSED_PARAMETER(v);
  return 1;
}

// void build_traits_init_frames(Routine *rt, Object *ob)
// {
// 	Object *base = ob;
// 	Object *code;
// 	Object *rob;
// 	while (OB_HasBase(base)) {
// 		base = OB_Base(base);
// 		if (OB_KLASS(base)->ob_klass == &Klass_Klass) break;
// 		code = getcode(base, "__init__", &rob);
// 		if (code) frame_new(rt, rob, code, 0);
// 	}
// }

void do_new_array(Routine *rt, int count)
{
  assert(rt->top + 1 >= count);
  Object *ob = List_New(NULL);
  TValue val;
  int i = 0;
  while (i < count) {
    val = POP();
    List_Set(ob, i, &val);
    ++i;
  }
  TValue v = {.klazz = &List_Klass, .ob = ob};
  PUSH(&v);
}

TValue do_load_subscr(Routine *rt)
{
  TValue w = POP();
  TValue v = POP();
  // v[w]
  MapOperations *ops = v.klazz->mapops;
  if (ops && ops->get) {
    return ops->get(&v, &w);
  } else {
    //FIXME: lookup __getitem__
    return NilValue;
  }
}

void do_store_subscr(Routine *rt)
{
  TValue w = POP();
  TValue v = POP();
  TValue u = POP();
  // v[w] = u
  MapOperations *ops = v.klazz->mapops;
  if (ops && ops->set) {
    ops->set(&v, &w, &u);
  } else {
    //FIXME: lookup __setitem__
  }
}

#define case_two_args_op(_case_, _op_)  \
  case _case_: {                        \
    TValue v1 = POP();                  \
    TValue v2 = POP();                  \
    TValue res = NilValue;              \
    NumberOperations *ops;              \
    if (v1.klazz && v1.klazz->numops) { \
      ops = v1.klazz->numops;           \
      if (ops->_op_) {                  \
        res = ops->_op_(&v1, &v2);      \
      } else {                          \
        exit(-1);                       \
      }                                 \
    }                                   \
    PUSH(&res);                         \
    break;                              \
  }

#define case_one_arg_op(_case_, _op_)   \
  case _case_: {                        \
    TValue v = POP();                   \
    TValue res = NilValue;              \
    NumberOperations *ops;              \
    if (v.klazz && v.klazz->numops) {   \
      ops = v.klazz->numops;            \
      if (ops->_op_) {                  \
        res = ops->_op_(&v);            \
      } else {                          \
        exit(-1);                       \
      }                                 \
    }                                   \
    PUSH(&res);                         \
    break;                              \
  }

#define NUMBER_OPERATION_CASES          \
  /* arithmetic */                      \
  case_two_args_op(OP_ADD, add)         \
  case_two_args_op(OP_SUB, sub)         \
  case_two_args_op(OP_MUL, mul)         \
  case_two_args_op(OP_DIV, div)         \
  case_two_args_op(OP_MOD, mod)         \
  case_one_arg_op(OP_NEG, neg)          \
  /* comparison */                      \
  case_two_args_op(OP_GT, gt)           \
  case_two_args_op(OP_GE, ge)           \
  case_two_args_op(OP_LT, lt)           \
  case_two_args_op(OP_LE, le)           \
  case_two_args_op(OP_EQ, eq)           \
  case_two_args_op(OP_NEQ, neq)         \
  /* bit */                             \
  case_two_args_op(OP_BAND, band)       \
  case_two_args_op(OP_BOR, bor)         \
  case_two_args_op(OP_BXOR, bxor)       \
  case_one_arg_op(OP_BNOT, bnot)        \
  case_two_args_op(OP_LSHIFT, lshift)   \
  case_two_args_op(OP_RSHIFT, rshift)   \
  /* logic */                           \
  case_two_args_op(OP_LAND, land)       \
  case_two_args_op(OP_LOR, lor)         \
  case_one_arg_op(OP_LNOT, lnot)

static void frame_loop(Frame *frame)
{
  int loopflag = 1;
  Routine *rt = frame->rt;
  CodeObject *code = (CodeObject *)frame->code;
  Object *consts = code->kf.consts;

  uint8 inst;
  int32 index;
  int32 offset;
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
        val = index_const(index, consts);
        PUSH(&val);
        break;
      }
      case OP_LOADM: {
        index = fetch_4bytes(frame, code);
        val = index_const(index, consts);
        char *path = String_RawString(val.ob);
        debug("load module '%s'", path);
        ob = Koala_Load_Module(path);
        assert(ob);
        setobjvalue(&val, ob);
        PUSH(&val);
        break;
      }
      case OP_GETM: {
        val = TOP();
        ob = val.ob;
        if (!OB_CHECK_KLASS(ob, Module_Klass)) {
          val = POP();
          Klass *klazz = OB_KLASS(ob);
          OB_ASSERT_KLASS(klazz, Klass_Klass);
          assert(klazz->module);
          setobjvalue(&val, klazz->module);
          PUSH(&val);
        }
        break;
      }
      case OP_LOAD: {
        index = fetch_2bytes(frame, code);
        val = load(frame, index);
        PUSH(&val);
        break;
      }
      case OP_LOAD0: {
        val = load(frame, 0);
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
        val = index_const(index, consts);
        char *field = String_RawString(val.ob);
        debug("getfield '%s'", field);
        val = POP();
        ob = val.ob;
        val = getfield(ob, field);
        if (!val.klazz) {
          Object *rob = NULL;
          ob = getcode(ob, field, &rob);
          val.klazz = &Code_Klass;
          val.ob = ob;
        }
        PUSH(&val);
        //Klass *k = (Klass *)(((CodeObject *)(frame->code))->owner);
        //Object_Get_Value2(ob, k, field);
        break;
      }
      case OP_SETFIELD: {
        index = fetch_4bytes(frame, code);
        val = index_const(index, consts);
        char *field = String_RawString(val.ob);
        debug("setfield '%s'", field);
        val = POP();
        ob = val.ob;
        val = POP();
        VALUE_ASSERT(&val);
        setfield(ob, field, &val);
        break;
      }
      case OP_CALL0: {
        int argc = fetch_2bytes(frame, code);
        debug("OP_CALL0, argc:%d", argc);
        val = POP();
        Object *meth = val.ob;
        assert(OB_KLASS(meth) == &Code_Klass);
        val = TOP();
        ob = val.ob;
        frame_new(rt, ob, meth, argc);
        loopflag = 0;
        break;
      }
      case OP_CALL: {
        index = fetch_4bytes(frame, code);
        val = index_const(index, consts);
        char *name = String_RawString(val.ob);
        int argc = fetch_2bytes(frame, code);
        debug("OP_CALL, %s, argc:%d", name, argc);
        val = TOP();
        ob = val.ob;
        //assert(!check_virtual_call(&val, name));
        Object *rob = NULL;
        Object *meth = getcode(ob, name, &rob);
        assert(rob);
        if (rob != ob) {
          debug(">>>>>update object<<<<<");
          POP();
          setobjvalue(&val, rob);
          PUSH(&val);
        }
        assert(meth);
        if (CODE_ISKFUNC(meth)) {
          //FIXME: for c function
          CodeObject *code = OB_TYPE_OF(meth, CodeObject, Code_Klass);
          check_args(rt, argc, code->proto, name);
        }
        frame_new(rt, rob, meth, argc);
        if (!strcmp(name, "__init__")) {
          //build_traits_init_frames(rt, ob);
        }
        loopflag = 0;
        break;
      }
      case OP_RET: {
        restore_previous_frame(frame);
        loopflag = 0;
        break;
      }
      case OP_NEWARRAY: {
        int count = fetch_4bytes(frame, code);
        do_new_array(rt, count);
        break;
      }
      case OP_LOAD_SUBSCR: {
        val = do_load_subscr(rt);
        PUSH(&val);
        break;
      }
      case OP_STORE_SUBSCR: {
        do_store_subscr(rt);
        break;
      }
      case OP_JUMP: {
        offset = fetch_4bytes(frame, code);
        frame->pc += offset;
        break;
      }
      case OP_JUMP_TRUE: {
        val = POP();
        VALUE_ASSERT_BOOL(&val);
        offset = fetch_4bytes(frame, code);
        if (val.bval) {
          frame->pc += offset;
        }
        break;
      }
      case OP_JUMP_FALSE: {
        val = POP();
        VALUE_ASSERT_BOOL(&val);
        offset = fetch_4bytes(frame, code);
        if (!val.bval) {
          frame->pc += offset;
        }
        break;
      }
      case OP_NEW: {
        index = fetch_4bytes(frame, code);
        val = index_const(index, consts);
        char *name = String_RawString(val.ob);
        val = POP();
        int argc = fetch_2bytes(frame, code);
        debug("OP_NEW, %s, argc:%d", name, argc);
        ob = val.ob;
        Klass *klazz = Module_Get_Class(ob, name);
        assert(klazz);
        assert(klazz != &Klass_Klass);
        assert(OB_Base(klazz) != (Object *)&Klass_Klass);
        ob = klazz->ob_alloc(klazz);
        setobjvalue(&val, ob);
        PUSH(&val);
#if 0
        Object *rob = NULL;
        Object *__init__ = getcode(ob, "__init__", &rob);
        assert(rob);
        assert(rob == ob);
        if (__init__)	{
          CodeObject *code = OB_TYPE_OF(__init__, CodeObject, Code_Klass);
          debug("__init__'s argc: %d", code->kf.proto->psz);
          check_args(rt, argc, code->kf.proto, name);
          if (code->kf.proto->rsz) {
            error("__init__ must be no any returns");
            exit(-1);
          }
          frame_new(rt, rob, __init__, argc);
          loopflag = 0;
        } else {
          debug("no __init__");
          if (argc) {
            error("no __init__, but %d", argc);
            exit(-1);
          }
        }
#endif
        break;
      }
      NUMBER_OPERATION_CASES
      default: {
        kassert(0, "unknown instruction:%d\n", inst);
      }
    }
  }
}
