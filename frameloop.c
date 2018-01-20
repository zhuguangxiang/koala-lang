
#include "routine.h"
#include "opcode.h"
#include "debug.h"
#include "koala.h"

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

static TValue const_to_tvalue(ConstItem *k, ItemTable *itable)
{
  TValue ret = NilValue;

  switch (k->type) {
    case CONST_INT: {
      setivalue(&ret, k->ival);
      break;
    }
    case CONST_FLOAT: {
      setfltvalue(&ret, k->fval);
      break;
    }
    case CONST_BOOL: {
      setbvalue(&ret, k->bval);
      break;
    }
    case CONST_STRING: {
      StringItem *stritem;
      stritem = ItemTable_Get(itable, ITEM_STRING, k->string_index);
      setcstrvalue(&ret, stritem->data);
      break;
    }
    default: {
      ASSERT_MSG("unknown const type:%d\n", k->type);
      break;
    }
  }
  return ret;
}

static Object *__get_func(Object *ob, char *name)
{
  if (OB_CHECK_KLASS(ob, Module_Klass)) {
    return Module_Get_Function(ob, name);
  } else if (OB_CHECK_KLASS(OB_KLASS(ob), Klass_Klass)) {
    return Klass_Get_Method(OB_KLASS(ob), name);
  } else {
    ASSERT_MSG("invalid object klass in '__get_func'\n");
    return NULL;
  }
}

static void __set_value(Object *ob, char *name, TValue *val)
{
  if (OB_CHECK_KLASS(ob, Module_Klass)) {
    Module_Set_Value(ob, name, val);
  } else if (OB_CHECK_KLASS(OB_KLASS(ob), Klass_Klass)) {

  } else {
    ASSERT_MSG("invalid object klass in '__set_value'\n");
  }
}

static TValue __get_value(Object *ob, char *name)
{
  if (OB_CHECK_KLASS(ob, Module_Klass)) {
    return Module_Get_Value(ob, name);
  } else if (OB_CHECK_KLASS(OB_KLASS(ob), Klass_Klass)) {
    return NilValue;
  } else {
    ASSERT_MSG("invalid object klass in '__get_value'\n");
    return NilValue;
  }
}

#define TOP()   ValueStack_Top(&rt->stack)
#define POP()   ValueStack_Pop(&rt->stack)
#define PUSH(v) ValueStack_Push(&rt->stack, (v))

int tonumber(TValue *v)
{
  UNUSED_PARAMETER(v);
  return 0;
}

void Frame_Loop(Frame *frame)
{
  int loopflag = 1;
  Routine *rt = frame->rt;
  MethodObject *meth = (MethodObject *)frame->func;
  ConstItem *k = meth->kf.k;
  uint8 *codes = meth->kf.codes;
  TValue *locals = frame->locals;
  ItemTable *itable = meth->kf.itable;

  uint8 inst;
  uint32 index;
  TValue val;

  while (loopflag) {
    inst = fetch_code(frame, codes);
    switch (inst) {
      case OP_LOADK: {
        index = fetch_arg4(frame, codes);
        val = const_to_tvalue(k + index, itable);
        PUSH(&val);
        break;
      }
      case OP_LOADM: {
        index = fetch_arg4(frame, codes);
        val = const_to_tvalue(k + index, itable);
        char *path = VALUE_CSTR(&val);
        debug_info("load module '%s'\n", path);
        Object *ob = Koala_Load_Module(path);
        ASSERT_PTR(ob);
        setobjvalue(&val, ob);
        PUSH(&val);
        break;
      }
      case OP_LOAD: {
        index = fetch_arg4(frame, codes);
        PUSH(locals + index);
        break;
      }
      case OP_STORE: {
        index = fetch_arg4(frame, codes);
        val = POP();
        locals[index] = val;
        break;
      }
      case OP_SETFIELD: {
        index = fetch_arg4(frame, codes);
        val = const_to_tvalue(k + index, itable);
        char *name = VALUE_CSTR(&val);
        debug_info("setfield '%s'\n", name);
        val= POP();
        Object *ob = VALUE_OBJECT(&val);
        val = POP();
        VALUE_ASSERT(&val);
        __set_value(ob, name, &val);
        break;
      }
      case OP_GETFIELD: {
        index = fetch_arg4(frame, codes);
        val = const_to_tvalue(k + index, itable);
        char *name = VALUE_CSTR(&val);
        debug_info("getfield '%s'\n", name);
        val = POP();
        Object *ob = VALUE_OBJECT(&val);
        val = __get_value(ob, name);
        PUSH(&val);
        break;
      }
      case OP_CALL: {
        index = fetch_arg4(frame, codes);
        val = const_to_tvalue(k + index, itable);
        char *name = VALUE_CSTR(&val);
        debug_info("%s()\n", name);
        Object *ob = VALUE_OBJECT(TOP());
        Frame *f = Frame_New(__get_func(ob, name));
        Routine_Add_Frame(rt, f);
        loopflag = 0;
        break;
      }
      case OP_RET: {
        frame->state = FRAME_EXIT;
        Frame_Free(frame);
        loopflag = 0;
        break;
      }
      case OP_ADD: {
        TValue v1 = POP();
        TValue v2 = POP();
        val = NilValue;
        // v2 is left value
        if (VALUE_ISINT(&v2) && VALUE_ISINT(&v1)) {
          uint64 i = (uint64)VALUE_INT(&v2) + (uint64)VALUE_INT(&v1);
          setivalue(&val, i);
        } else if (tonumber(&v2) && tonumber(&v1)) {

        }
        PUSH(&val);
        break;
      }
      case OP_SUB: {
        TValue v1 = POP();
        TValue v2 = POP();
        val = NilValue;
        // v2 is left value
        if (VALUE_ISINT(&v2) && VALUE_ISINT(&v1)) {
          uint64 i = (uint64)VALUE_INT(&v2) - (uint64)VALUE_INT(&v1);
          setivalue(&val, i);
        } else if (tonumber(&v2) && tonumber(&v1)) {

        }
        PUSH(&val);
        break;
      }
      case OP_MUL: {
        TValue v1 = POP();
        TValue v2 = POP();
        val = NilValue;
        // v2 is left value
        if (VALUE_ISINT(&v2) && VALUE_ISINT(&v1)) {
          uint64 i = (uint64)VALUE_INT(&v2) * (uint64)VALUE_INT(&v1);
          setivalue(&val, i);
        } else if (tonumber(&v2) && tonumber(&v1)) {

        }
        PUSH(&val);
        break;
      }
      case OP_DIV: {
        /* float division (always with floats) */
        TValue v1 = POP();
        TValue v2 = POP();
        val = NilValue;
        // v2 is left value
        if (tonumber(&v2) && tonumber(&v1)) {

        }
        PUSH(&val);
        break;
      }
      default: {
        ASSERT_MSG("unknown instruction:%d\n", inst);
      }
    }
  }
}
