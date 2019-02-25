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

#include "koala.h"
#include "opcode.h"

LOGGER(0)

typedef struct frame {
  /* back frame */
  struct frame *back;
  /* KoalaState */
  KoalaState *ks;
  /* code */
  Object *code;
  /* object */
  Object *ob;
  /* arguments */
  Object *args;
  /* last instruction if it's called */
  int lasti;
  /* local variable's count */
  int size;
  /* local variable's memory */
  Object *locvars[0];
} CodeFrame;

#define TOP() Stack_Top(stack)
#define POP() Stack_Pop(stack)
#define PUSH(val) Stack_Push(stack, (val))
#define NEXT_CODE(frame, codes) codes[++(frame)->lasti]

static inline uint8 fetch_byte(CodeFrame *frame, CodeObject *code)
{
  assert(frame->lasti < code->kf.size);
  return NEXT_CODE(frame, code->kf.codes);
}

static inline uint16 fetch_2bytes(CodeFrame *frame, CodeObject *code)
{
  /* endian? */
  assert(frame->lasti < code->kf.size);
  uint8 l = NEXT_CODE(frame, code->kf.codes);
  uint8 h = NEXT_CODE(frame, code->kf.codes);
  return (h << 8) + (l << 0);
}

static inline uint32 fetch_4bytes(CodeFrame *frame, CodeObject *code)
{
  /* endian? */
  assert(frame->lasti < code->kf.size);
  uint8 l1 = NEXT_CODE(frame, code->kf.codes);
  uint8 l2 = NEXT_CODE(frame, code->kf.codes);
  uint8 h1 = NEXT_CODE(frame, code->kf.codes);
  uint8 h2 = NEXT_CODE(frame, code->kf.codes);
  return (h2 << 24) + (h1 << 16) + (l2 << 8) + (l1 << 0);
}

static inline uint8 fetch_opcode(CodeFrame *frame, CodeObject *code)
{
  return fetch_byte(frame, code);
}

static inline Object *index_const(int index, Object *consts)
{
  return Tuple_Get(consts, index);
}

static inline Object *load(CodeFrame *frame, int index)
{
  assert(index < frame->size);
  return frame->locvars[index];
}

static inline void store(CodeFrame *frame, int index, Object *val)
{
  assert(val != NULL);
  assert(index < frame->size);
  Object *v = frame->locvars[index];
  OB_DECREF(v);
  frame->locvars[index] = val;
}

static void push_frame(KoalaState *ks, Object *code, Object *ob, Object *args)
{
  if (ks->depth >= MAX_FRAME_DEPTH) {
    Log_Error("StackOverflow");
    exit(-1);
  }
  CodeObject *co = (CodeObject *)code;
  int size = IsKCode(code) ? Vector_Size(&co->kf.locvec) : 0;
  size = sizeof(CodeFrame) + size * sizeof(Object *);
  CodeFrame *frame = Malloc(size);
  frame->code = code;
  frame->ob = ob;
  frame->args = args;
  frame->ks = ks;
  frame->lasti = -1;
  frame->size = size;
  frame->back = ks->frame;
  ks->frame = frame;
  ks->depth++;
}

static void free_frame(CodeFrame *frame)
{
  for (int i = 0; i < frame->size; i++)
    OB_DECREF(frame->locvars[i]);
  Mfree(frame);
}

static void pop_frame(CodeFrame *frame)
{
  KoalaState *ks = frame->ks;
  ks->frame = frame->back;
  free_frame(frame);
}

static void loop_frame(CodeFrame *f)
{
  int loopflag = 1;
  KoalaState *ks = f->ks;
  Stack *stack = &ks->stack;
  CodeObject *code = (CodeObject *)f->code;
  Object *consts = code->kf.consts;

  uint8 inst;
  int32 index;
  int32 offset;
  Object *ob;

  while (loopflag) {
    inst = fetch_opcode(f, code);
    switch (inst) {
      case OP_HALT: {
        loopflag = 0;
        break;
      }
      case OP_LOADK: {
        index = fetch_2bytes(f, code);
        ob = index_const(index, consts);
        OB_INCREF(ob);
        PUSH(ob);
        break;
      }
      case OP_LOAD: {
        index = fetch_2bytes(f, code);
        ob = load(f, index);
        OB_INCREF(ob);
        PUSH(ob);
        break;
      }
      case OP_STORE: {
        index = fetch_2bytes(f, code);
        ob = POP();
        store(f, index, ob);
        break;
      }
      case OP_RETURN: {
        pop_frame(f);
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

static void check_arguments(CodeFrame *f)
{
  int size = Tuple_Size(f->args);
  int argc = Code_Get_Argc(f->code);
  Log_Debug("call argc: %d, proto argc: %d", size, argc);
  if (size < argc) {
    Log_Error("call argc %d < proto argc %d", size, argc);
    exit(-1);
  }
}

static void prepare_kargs(CodeFrame *f)
{
  OB_INCREF(f->ob);
  f->locvars[0] = f->ob;
  if (f->args) {
    if (OB_KLASS(f->args) == &Tuple_Klass) {
      Object *o;
      int size = Tuple_Size(f->args);
      for (int i = 0; i <= size - 1; i++) {
        o = Tuple_Get(f->args, i);
        OB_INCREF(o);
        f->locvars[i + 1] = o;
      }
    } else {
      OB_INCREF(f->args);
      f->locvars[1] = f->args;
    }
  }
}

static inline void run_kfunction(CodeFrame *f)
{
  if (f->lasti < 0) {
    check_arguments(f);
    prepare_kargs(f);
  }
  loop_frame(f);
}

static void run_cfunction(CodeFrame *f)
{
  KoalaState *ks = f->ks;
  Stack *stack = &ks->stack;
  check_arguments(f);

  Object *result = ((CodeObject *)f->code)->cf(f->ob, f->args);
  if (result) {
    /* save result */
    if (OB_KLASS(result) == &Tuple_Klass) {
      int sz = Tuple_Size(result);
      Object *ob;
      for (int i = sz - 1; i >= 0; i--) {
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
  pop_frame(f);
}

void Koala_RunCode(Object *code, Object *ob, Object *args)
{
  KoalaState *ks = Current_State();
  push_frame(ks, code, ob, args);

  CodeFrame *f;
  while ((f = ks->frame) != NULL) {
    if (IsKCode(f->code)) {
      run_kfunction(f);
    } else if (IsCCode(f->code)) {
      run_cfunction(f);
    } else {
      Log_Error("Invalid function type");
      exit(-1);
    }
  }
}
