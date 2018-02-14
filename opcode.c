
#include "opcode.h"

static struct opcode {
  uint8 code;
  char *str;
  int argsize;
} opcodes[] = {
  {OP_HALT,     "halt",     0},
  {OP_LOADK,    "loadk",    4},
  {OP_LOADM,    "loadm",    4},
  {OP_LOAD,     "load",     2},
  {OP_STORE,    "store",    2},
  {OP_GETFIELD, "getfield", 4},
  {OP_SETFIELD, "setfield", 4},
  {OP_CALL,     "call",     4},
  {OP_RET,      "return",   0},
  {OP_ADD,      "add",      0},
  {OP_SUB,      "sub",      0},
  {OP_MUL,      "mul",      0},
  {OP_DIV,      "div",      0},
};

struct opcode *get_opcode(uint8 op)
{
  for (int i = 0; i < nr_elts(opcodes); i++) {
    if (opcodes[i].code == op) {
      return opcodes + i;
    }
  }
  return NULL;
}

char *OPCode_ToString(uint8 op)
{
  struct opcode *opcode = get_opcode(op);
  return opcode != NULL ? opcode->str: "";
}

void Code_Show(uint8 *code, int32 size)
{
  int i = 0;
  struct opcode *op;
  int arg;
  printf("-------------------------------------\n");
  while (i < size) {
    op = get_opcode(code[i]);
    if (op == NULL) break;
    i++;
    printf("op:%-12s", op->str);
    if (op->argsize == 4) arg = *(int *)(code + i);
    else if (op->argsize == 2) arg = *(short *)(code + i);
    else arg = -1;
    printf("arg:%x\n", arg);
    i += op->argsize;
  }
  printf("-------------------------------------\n");
}
