/*
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "eval.h"
#include "tupleobject.h"
#include "opcode.h"
#include "log.h"
#include "mem.h"

static Logger logger;

static inline Object *stack_top(struct stack *stk)
{
  assert(stk->top >= -1 && stk->top <= STACK_SIZE - 1);
  return (stk->top >= 0) ? stk->stack[stk->top] : NULL;
}

static inline Object *stack_pop(struct stack *stk)
{
  assert(stk->top >= -1 && stk->top <= STACK_SIZE - 1);
  return (stk->top >= 0) ? stk->stack[stk->top--] : NULL;
}

static inline void stack_push(struct stack *stk, Object *v)
{
  assert(stk->top >= -1 && stk->top < STACK_SIZE - 1);
  stk->stack[++stk->top] = v;
}

static inline int stack_size(struct stack *stk)
{
  assert(stk->top >= -1 && stk->top <= STACK_SIZE - 1);
  return stk->top + 1;
}

static inline void init_stack(struct stack *stk)
{
  stk->top = -1;
}

static struct frame *frame_new(KoalaState *ks, CodeObject *code)
{
  int size = 0;
  if (CODE_IS_K(code))
    size = Vector_Size(&code->kf.locvec);

  int objsz = sizeof(struct frame) + size * sizeof(Object *);
  struct frame *frame = mm_alloc(objsz);
  frame->code = code;
  frame->ks = ks;
  frame->pc = -1;
  frame->size = size;
  frame->back = ks->frame;
  ks->frame = frame;
  return frame;
}

static void frame_free(struct frame *frame)
{
  for (int i = 0; i < frame->size; i++)
    OB_DECREF(frame->locvars[i]);
  mm_free(frame);
}

static void goto_up_frame(struct frame *frame)
{
  KoalaState *ks = frame->ks;
  ks->frame = frame->back;
  frame_free(frame);
}

#define TOP() stack_top(stack)
#define POP() stack_pop(stack)
#define PUSH(val) stack_push(stack, (val))
#define NEXT_CODE(frame, codes) codes[++(frame)->pc]

static inline uint8 fetch_byte(struct frame *frame, CodeObject *code)
{
  assert(frame->pc < code->kf.size);
  return NEXT_CODE(frame, code->kf.codes);
}

static uint16 fetch_2bytes(struct frame *frame, CodeObject *code)
{
  assert(frame->pc < code->kf.size);
  /* endian? */
  uint8 l = NEXT_CODE(frame, code->kf.codes);
  uint8 h = NEXT_CODE(frame, code->kf.codes);
  return (h << 8) + (l << 0);
}

static uint32 fetch_4bytes(struct frame *frame, CodeObject *code)
{
  assert(frame->pc < code->kf.size);
  /* endian? */
  uint8 l1 = NEXT_CODE(frame, code->kf.codes);
  uint8 l2 = NEXT_CODE(frame, code->kf.codes);
  uint8 h1 = NEXT_CODE(frame, code->kf.codes);
  uint8 h2 = NEXT_CODE(frame, code->kf.codes);
  return (h2 << 24) + (h1 << 16) + (l2 << 8) + (l1 << 0);
}

static inline uint8 fetch_opcode(struct frame *frame, CodeObject *code)
{
  return fetch_byte(frame, code);
}

static inline Object *index_const(int index, Object *consts)
{
  return Tuple_Get(consts, index);
}

static inline Object *load(struct frame *frame, int index)
{
  assert(index < frame->size);
  return frame->locvars[index];
}

static void store(struct frame *frame, int index, Object *val)
{
  assert(val);
  assert(index < frame->size);
  Object *v = frame->locvars[index];
  if (v)
    OB_DECREF(v);
  frame->locvars[index] = val;
}

static Object *find_code(Object *ob, char *name)
{
  MemberDef *m;
  if (OB_KLASS(ob) == &Package_Klass) {
    m = Package_Find_MemberDef((PackageObject *)ob, name);
    if (!m || m->kind != MEMBER_CODE) {
      Log_Error("function '%s' is not found", name);
      return NULL;
    }
    Log_Debug("found function '%s' in package '%s'",
              name, ((PackageObject *)ob)->name);
    return m->code;
  } else {
    Object *code = Object_Get_Method(ob, name, NULL);
    if (!code) {
      Log_Error("symbol '%s' is not found", name);
      return NULL;
    }
    Log_Debug("find function '%s' in class '%s'",
              name, OB_KLASS(ob)->name);
    return code;
  }
}

static void set_field(Object *ob, char *field, Object *val)
{
  if (OB_KLASS(ob) == &Package_Klass) {
    Koala_Set_Value((PackageObject *)ob, field, val);
  } else {
    Object_Set_Value(ob, field, NULL, val);
  }
}

static Object *get_field(Object *ob, char *field)
{
  if (OB_KLASS(ob) == &Package_Klass) {
    return Koala_Get_Value((PackageObject *)ob, field);
  } else {
    return Object_Get_Value(ob, field, NULL);
  }
}

static void frame_loop(struct frame *frame)
{
  int loopflag = 1;
  KoalaState *ks = frame->ks;
  struct stack *stack = &ks->stack;
  CodeObject *code = frame->code;
  Object *consts = code->kf.consts;

  uint8 inst;
  int32 index;
  int32 offset;
  Object *ob;

  while (loopflag) {
    inst = fetch_opcode(frame, code);
    switch (inst) {
      case OP_HALT: {
        exit(0);
        break;
      }
      case OP_LOADK: {
        index = fetch_4bytes(frame, code);
        ob = index_const(index, consts);
        OB_INCREF(ob);
        PUSH(ob);
        break;
      }
      case OP_LOAD: {
        index = fetch_2bytes(frame, code);
        ob = load(frame, index);
        OB_INCREF(ob);
        PUSH(ob);
        break;
      }
      case OP_STORE: {
        index = fetch_2bytes(frame, code);
        ob = POP();
        store(frame, index, ob);
        break;
      }
      case OP_RET: {
        goto_up_frame(frame);
        loopflag = 0;
        break;
      }
      default: {
        assert(0);
        break;
      }
    }
  }
}

static void prepare_kframe(struct frame *frame)
{
  KoalaState *ks = frame->ks;
  struct stack *stack = &ks->stack;
  int sz = stack_size(stack);
  int argc = Code_Get_Argc((Object *)frame->code);
  assert(sz >= argc && argc <= frame->size);
  Log_Debug("stack size: %d, code argc: %d", sz, argc);

  /* prepare arguments */
  Object *ob;
  int i = 0;
  while (argc-- > 0) {
    ob = POP();
    frame->locvars[i++] = ob;
  }
}

static void run_cframe(struct frame *frame)
{
  KoalaState *ks = frame->ks;
  struct stack *stack = &ks->stack;
  int sz = stack_size(stack);
  int argc = Code_Get_Argc((Object *)frame->code);
  assert(sz >= argc);
  Log_Debug("stack size: %d, code argc: %d", sz, argc);

  /* prepare arguments */
  Object *ob = POP();
  Object *arg;
  Object *args = Tuple_New(argc);
  int i = 0;
  while (argc-- > 0) {
    arg = POP();
    Tuple_Set(args, i++, arg);
  }

  /* call c function */
  Object *result = frame->code->cf(ob, args);
  Tuple_Free(args);

  /* Save the result */
  if (result) {
    if (OB_KLASS(result) == &Tuple_Klass) {
      sz = Tuple_Size(result);
      for (i = sz - 1; i >= 0; i--) {
        ob = Tuple_Get(result, i);
        OB_INCREF(ob);
        PUSH(ob);
      }
    } else {
      OB_INCREF(result);
      PUSH(result);
    }
    Tuple_Free(result);
  }

  /* goto up frame */
  goto_up_frame(frame);
}

int Koala_Run_Code(KoalaState *ks, Object *code, Object *ob, Object *args)
{
  struct stack *stack = &ks->stack;
  if (args) {
    if (OB_KLASS(args) == &Tuple_Klass) {
      Object *o;
      int size = Tuple_Size(args);
      for (int i = 0; i <= size - 1; i++) {
        o = Tuple_Get(args, i);
        OB_INCREF(o);
        PUSH(o);
      }
    } else {
      OB_INCREF(args);
      PUSH(args);
    }
  }
  OB_INCREF(ob);
  PUSH(ob);

  frame_new(ks, (CodeObject *)code);

  struct frame *frame;
  while (1) {
    frame = ks->frame;
    if (!frame)
      return 0;
    if (CODE_IS_K(frame->code)) {
      if (frame->pc == -1) {
        prepare_kframe(frame);
      }
      /* run codes */
      frame_loop(frame);
    } else if (CODE_IS_C(frame->code)) {
      run_cframe(frame);
    } else {
      assert(0);
    }
  }
}

KoalaState *KoalaState_New(void)
{
  KoalaState *ks = mm_alloc(sizeof(KoalaState));
  init_list_head(&ks->ksnode);
  ks->gs = &gState;
  list_add_tail(&ks->ksnode, &ks->gs->kslist);
  init_stack(&ks->stack);
  return ks;
}
