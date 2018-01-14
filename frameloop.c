
#include "routine.h"
#include "opcode.h"
#include "debug.h"
#include "methodobject.h"
#include "moduleobject.h"
#include "stringobject.h"

#define NEXT_CODE(f, codes) codes[f->pc++]

static inline uint8 fetch_code(Frame *frame, uint8 *codes)
{
  return NEXT_CODE(frame, codes);
}

static inline uint8 fetch_arg1(Frame *frame, uint8 *codes)
{
  return NEXT_CODE(frame, codes);
}

static inline uint32 fetch_arg4(Frame *frame, uint8 *codes)
{
  //endian?
  uint8 l1 = NEXT_CODE(frame, codes);
  uint8 l2 = NEXT_CODE(frame, codes);
  uint8 h1 = NEXT_CODE(frame, codes);
  uint8 h2 = NEXT_CODE(frame, codes);
  return (h2 << 24) + (h1 << 16) + (l2 << 8) + (l1 << 0);
}

static int ConstItemToTValue(ConstItem *k, ItemTable *itable, TValue *ret)
{
  switch (k->type) {
    case CONST_INT: {
      set_int_value(ret, k->value.ival);
      break;
    }
    case CONST_FLOAT: {
      set_float_value(ret, k->value.fval);
      break;
    }
    case CONST_BOOL: {
      set_bool_value(ret, k->value.bval);
      break;
    }
    case CONST_STRING: {
      StringItem *stritem;
      stritem = ItemTable_Get(itable, ITEM_STRING, k->value.string_index);
      set_object_value(ret, String_New(stritem->data));
      break;
    }
    default: {
      debug_error("unknown const type:%d\n", k->type);
      *ret = NilValue;
      assert(0);
      break;
    }
  }
  return 0;
}

void koala_frame_loop(Frame *frame)
{
  int loopflag = 1;
  Routine *rt = frame->rt;
  MethodObject *meth = (MethodObject *)frame->func;
  ConstItem *k = meth->kf.k;
  uint8 *codes = meth->kf.codes;
  TValue *locals = frame->locals;
  ItemTable *itable;

  if (OB_CHECK_KLASS(meth->owner, Module_Klass))
    itable = ((ModuleObject *)meth->owner)->itable;
  else if (OB_CHECK_KLASS(meth->owner, Klass_Klass))
    itable = ((Klass *)meth->owner)->itable;
  else {
    debug_error("which is method owner?\n");
    assert(0);
  }

  uint8 inst;
  uint32 index;
  TValue val;

  while (loopflag) {
    inst = fetch_code(frame, codes);
    switch (inst) {
      case OP_LOADK: {
        index = fetch_arg4(frame, codes);
        ConstItemToTValue(k + index, itable, &val);
        rt_stack_push(rt, &val);
        break;
      }
      case OP_LOAD: {
        index = fetch_arg4(frame, codes);
        rt_stack_push(rt, locals + index);
        break;
      }
      case OP_STORE: {
        index = fetch_arg4(frame, codes);
        val = rt_stack_pop(rt);
        locals[index] = val;
        break;
      }
      case OP_CALL: {
        //method name is full path
        index = fetch_arg4(frame, codes);
        ConstItemToTValue(k + index, itable, &val);
        assert(VALUE_ISOBJECT(&val));
        //frame_new(rt, KState_Find_Method());
        loopflag = 0;
        break;
      }
      case OP_RET: {
        list_del(&frame->link);
        Frame_Free(frame);
        loopflag = 0;
        break;
      }
      case OP_GO: {

        break;
      }
      case OP_ADD: {
        break;
      }
      case OP_SUB: {
        break;
      }
      default: {
        debug_error("unknown instruction:%d\n", inst);
        assert(0);
      }
    }
  }
}
