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
  char *opstr;
  uint8 argsize;
} opcodes [] = {
  {"halt",        0},
  {NULL, 0},
  {NULL, 0},
  {NULL, 0},
  {NULL, 0},
  {NULL, 0},
  {NULL, 0},
  {NULL, 0},
  {NULL, 0},
  {"pop",         0},
  {"dup",         0},
  {"bpush",       1},
  {"spush",       2},
  {NULL, 0},
  {NULL, 0},
  {NULL, 0},
  {"loadk",       2},
  {"loadp",       2},
  {"load",        1},
  {"load0",       0},
  {"load1",       0},
  {"load2",       0},
  {"load3",       0},
  {"store",       1},
  {"store0",      0},
  {"store1",      0},
  {"store2",      0},
  {"store3",      0},
  {"loadfield",   2},
  {"storefield",  2},
  {"call",        1},
  {"return",      0},
  {"add",         0},
  {"sub",         0},
  {"mul",         0},
  {"div",         0},
  {"mod",         0},
  {"power",       0},
  {"neg",         0},
  {"gt",          0},
  {"ge",          0},
  {"lt",          0},
  {"le",          0},
  {"eq",          0},
  {"neq",         0},
  {"and",         0},
  {"or",          0},
  {"not",         0},
  {"bitand",      0},
  {"bitor",       0},
  {"bitxor",      0},
  {"bitnot",      0},
  {"bitlsh",      0},
  {"bitrsh",      0},
  {NULL, 0},
  {NULL, 0},
  {NULL, 0},
  {NULL, 0},
  {NULL, 0},
  {NULL, 0},
  {NULL, 0},
  {NULL, 0},
  {NULL, 0},
  {NULL, 0},
  {"jmp",         2},
  {"jtrue",       2},
  {"jfalse",      2},
  {"jcmpeq",      2},
  {"jcmpneq",     2},
  {"jcmplt",      2},
  {"jcmpgt",      2},
  {"jcmple",      2},
  {"jcmpge",      2},
  {"jnil",        2},
  {"jnotnil",     2},
  {NULL, 0},
  {NULL, 0},
  {NULL, 0},
  {NULL, 0},
  {NULL, 0},
  {"new",         0},
  {"newarray",    0},
  {"newdict",     0},
  {"newset",      0},
  {"newclosure",  0},
  {"arrayload",   0},
  {"arraystore",  0},
  {"dictload",    0},
  {"dictstore",   0},
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
