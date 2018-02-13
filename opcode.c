
#include "opcode.h"

static struct {
  uint8 code;
  char *str;
} opcodes[] = {
  {OP_HALT,     "halt"},
  {OP_LOADK,    "loadk"},
  {OP_LOADM,    "loadm"},
  {OP_LOAD,     "load"},
  {OP_STORE,    "store"},
  {OP_GETFIELD, "getfield"},
  {OP_SETFIELD, "setfield"},
  {OP_CALL,     "call"},
  {OP_RET,      "return"},
  {OP_ADD,      "add"},
  {OP_SUB,      "sub"},
  {OP_MUL,      "mul"},
  {OP_DIV,      "div"},
};

char *OPCode_ToString(uint8 op)
{
  for (int i = 0; i < nr_elts(opcodes); i++) {
    if (opcodes[i].code == op) {
      return opcodes[i].str;
    }
  }
  return "";
}
