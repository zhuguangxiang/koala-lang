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

#include "common.h"
#include "opcode.h"

#define OPCODE(op, size) { op, #op, size }

static struct opcode {
  int op;
  char *opstr;
  uint8 argsize;
} opcodes [] = {
  OPCODE(POP_TOP,  0),
  OPCODE(POP_TOPN, 1),

  OPCODE(DUP, 0),

  OPCODE(CONST_BYTE,  1),
  OPCODE(CONST_SHORT, 2),

  OPCODE(LOAD_CONST,  2),
  OPCODE(LOAD_PKG,    2),
  OPCODE(LOAD,        1),
  OPCODE(LOAD_0,      0),
  OPCODE(LOAD_1,      0),
  OPCODE(LOAD_2,      0),
  OPCODE(LOAD_3,      0),
  OPCODE(STORE,       1),
  OPCODE(STORE_0,     0),
  OPCODE(STORE_1,     0),
  OPCODE(STORE_2,     0),
  OPCODE(STORE_3,     0),
  OPCODE(GET_ATTR,    2),
  OPCODE(SET_ATTR,    2),
  OPCODE(CALL,        3),
  OPCODE(RETURN,      0),

  OPCODE(ADD, 0),
  OPCODE(SUB, 0),
  OPCODE(MUL, 0),
  OPCODE(DIV, 0),
  OPCODE(MOD, 0),
  OPCODE(POW, 0),
  OPCODE(NEG, 0),

  OPCODE(GT,  0),
  OPCODE(GE,  0),
  OPCODE(LT,  0),
  OPCODE(LE,  0),
  OPCODE(EQ,  0),
  OPCODE(NEQ, 0),

  OPCODE(BAND,   0),
  OPCODE(BOR,    0),
  OPCODE(BXOR,   0),
  OPCODE(BNOT,   0),
  OPCODE(LSHIFT, 0),
  OPCODE(RSHIFT, 0),

  OPCODE(AND, 0),
  OPCODE(OR,  0),
  OPCODE(NOT, 0),

  OPCODE(JMP,        0),
  OPCODE(JMP_TRUE,   0),
  OPCODE(JMP_FALSE,  0),
  OPCODE(JMP_CMPEQ,  0),
  OPCODE(JMP_CMPNEQ, 0),
  OPCODE(JMP_CMPLT,  0),
  OPCODE(JMP_CMPGT,  0),
  OPCODE(JMP_CMPLE,  0),
  OPCODE(JMP_CMPGE,  0),
  OPCODE(JMP_NIL,    0),
  OPCODE(JMP_NOTNIL, 0),

  OPCODE(NEW,         0),
  OPCODE(NEW_ARRAY,   0),
  OPCODE(NEW_MAP,     0),
  OPCODE(NEW_SET,     0),
  OPCODE(NEW_CLOSURE, 0),
  OPCODE(MAP_LOAD,    0),
  OPCODE(MAP_STORE,   0),
};

#define OP_MAP(op, map) { op, #map }

struct operator {
  uint8 op;
  char *map;
} operators [] = {
  OP_MAP(ADD, __add__),
  OP_MAP(SUB, __sub__),
  OP_MAP(MUL, __mul__),
  OP_MAP(DIV, __div__),
  OP_MAP(MOD, __mod__),
  OP_MAP(POW, __pow__),
  OP_MAP(NEG, __neg__),

  OP_MAP(GT,  __gt__),
  OP_MAP(GE,  __ge__),
  OP_MAP(LT,  __lt__),
  OP_MAP(LE,  __le__),
  OP_MAP(EQ,  __eq__),
  OP_MAP(NEQ, __neq__),

  OP_MAP(BAND, __bitand__),
  OP_MAP(BOR,  __bitor__),
  OP_MAP(BXOR, __bitxor__),
  OP_MAP(BNOT, __bitnor__),
  OP_MAP(LSHIFT, __bitlshift__),
  OP_MAP(RSHIFT, __bitrshift__),

  OP_MAP(AND, __and__),
  OP_MAP(OR,  __or__),
  OP_MAP(NOT, __not__),

  OP_MAP(MAP_LOAD,  __getitem__),
  OP_MAP(MAP_STORE, __setitem__),
};

int OpCode_ArgCount(int op)
{
  struct opcode *opcode;
  for (int i = 0; i < nr_elts(opcodes); i++) {
    opcode = &opcodes[i];
    if (opcode->op == op)
      return opcode->argsize;
  }
  return -1;
}

char *OpCode_String(int op)
{
  struct opcode *opcode;
  for (int i = 0; i < nr_elts(opcodes); i++) {
    opcode = &opcodes[i];
    if (opcode->op == op)
      return opcode->opstr;
  }
  return NULL;
}

char *OpCode_Operator(int op)
{
  struct operator *operator;
  for (int i = 0; i < nr_elts(operators); i++) {
    operator = &operators[i];
    if (operator->op == op)
      return operator->map;
  }
  return NULL;
}
