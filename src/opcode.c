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

#define OPCODE(op, size) { op, #op, size }

static struct opcode {
  int op;
  char *opstr;
  uint8 argsize;
} opcodes [] = {
  OPCODE(POP_TOP,   0),
  OPCODE(POP_TOPN,  1),

  OPCODE(DUP,   0),

  OPCODE(PUSH_BYTE,  1),
  OPCODE(PUSH_SHORT, 2),

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
  OPCODE(LOAD_FIELD,  2),
  OPCODE(STORE_FIELD, 2),
  OPCODE(CALL,        3),
  OPCODE(RETURN,      0),

  OPCODE(ADD,   0),
  OPCODE(SUB,   0),
  OPCODE(MUL,   0),
  OPCODE(DIV,   0),
  OPCODE(MOD,   0),
  OPCODE(POWER, 0),
  OPCODE(NEG,   0),

  OPCODE(GT,  0),
  OPCODE(GE,  0),
  OPCODE(LT,  0),
  OPCODE(LE,  0),
  OPCODE(EQ,  0),
  OPCODE(NEQ, 0),

  OPCODE(AND, 0),
  OPCODE(OR,  0),
  OPCODE(NOT, 0),

  OPCODE(BAND,   0),
  OPCODE(BOR,    0),
  OPCODE(BXOR,   0),
  OPCODE(BNOT,   0),
  OPCODE(LSHIFT, 0),
  OPCODE(RSHIFT, 0),

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
  OPCODE(ARRAY_LOAD,  0),
  OPCODE(ARRAY_STORE, 0),
  OPCODE(MAP_LOAD,    0),
  OPCODE(MAP_STORE,   0),
};

int OpCode_ArgCount(uint8 op)
{
  struct opcode *opcode = &opcodes[op];
  return opcode->argsize;
}

char *OpCode_String(uint8 op)
{
  struct opcode *opcode = &opcodes[op];
  return opcode ? opcode->opstr: "";
}
