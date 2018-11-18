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

#include "opcode.h"

static struct opcode {
  uint8 code;
  char *str;
  int argsize;
} opcodes[] = {
  {OP_HALT,     "halt",     0},
  {OP_LOADK,    "loadk",    4},
  {OP_LOADM,    "loadm",    4},
  {OP_GETM,     "getm",     0},
  {OP_LOAD,     "load",     2},
  {OP_LOAD0,    "load0",    0},
  {OP_STORE,    "store",    2},
  {OP_GETFIELD, "getfield", 4},
  {OP_SETFIELD, "setfield", 4},
  {OP_CALL,     "call",     6},
  {OP_CALL0,    "call0",    2},
  {OP_RET,      "return",   0},
  {OP_ADD,      "add",      0},
  {OP_SUB,      "sub",      0},
  {OP_MUL,      "mul",      0},
  {OP_DIV,      "div",      0},
  {OP_MOD,      "mod",      0},
  {OP_GT,       "gt",       0},
  {OP_GE,       "ge",       0},
  {OP_LT,       "lt",       0},
  {OP_LE,       "le",       0},
  {OP_EQ,       "eq",       0},
  {OP_NEQ,      "neq",      0},
  {OP_NEG,      "minus",    0},
  {OP_JUMP,     "jump",     4},
  {OP_JUMP_TRUE,   "jump_true",   4},
  {OP_JUMP_FALSE,  "jump_false",  4},
  {OP_NEW,      "new",      6},
  {OP_NEWARRAY, "array",    2},
  {OP_NEWMAP,   "map",      2},
  {OP_LOAD_SUBSCR, "load_subscr",   0},
  {OP_STORE_SUBSCR, "store_subscr", 0}
};

static struct opcode *get_opcode(uint8 op)
{
  for (int i = 0; i < nr_elts(opcodes); i++) {
    if (opcodes[i].code == op) {
      return opcodes + i;
    }
  }
  return NULL;
}

int OpCode_ArgCount(uint8 op)
{
  struct opcode *opcode = get_opcode(op);
  return opcode->argsize;
}

char *OpCode_String(uint8 op)
{
  struct opcode *opcode = get_opcode(op);
  return opcode ? opcode->str: "";
}

void Code_Show(uint8 *code, int32 size)
{
  int i = 0;
  struct opcode *op;
  int arg;
  printf("------code show--------\n");
  while (i < size) {
    op = get_opcode(code[i]);
    if (!op)
      break;
    i++;
    printf("op:%-12s", op->str);
    if (op->argsize == 4)
      arg = *(int32 *)(code + i);
    else if (op->argsize == 2)
      arg = *(int16 *)(code + i);
    else
      arg = -1;
    printf("arg:%d\n", arg);
    i += op->argsize;
  }
  printf("------code show end----\n");
}
