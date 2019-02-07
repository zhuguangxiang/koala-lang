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

#include "codegen.h"
#include "buffer.h"
#include "mem.h"
#include "log.h"

LOGGER(1)

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

Inst *Inst_New(uint8 op, Argument *val)
{
  Inst *i = Malloc(sizeof(Inst));
  init_list_head(&i->link);
  i->op = op;
  i->bytes = 1 + OpCode_ArgCount(op);
  if (val)
    i->arg = *val;
  return i;
}

void Inst_Free(Inst *i)
{
  Mfree(i);
}

Inst *Inst_Append(CodeBlock *b, uint8 op, Argument *val)
{
  Inst *i = Inst_New(op, val);
  list_add_tail(&i->link, &b->insts);
  b->bytes += i->bytes;
  i->upbytes = b->bytes;
  return i;
}

Inst *Inst_Append_NoArg(CodeBlock *b, uint8 op)
{
  return Inst_Append(b, op, NULL);
}

JmpInst *JmpInst_New(Inst *inst, int type)
{
  JmpInst *jmp = Malloc(sizeof(JmpInst));
  jmp->inst = inst;
  jmp->type = type;
  return jmp;
}

void JmpInst_Free(JmpInst *jmp)
{
  Mfree(jmp);
}

CodeBlock *CodeBlock_New(void)
{
  CodeBlock *b = Malloc(sizeof(CodeBlock));
  init_list_head(&b->insts);
  return b;
}

void CodeBlock_Free(CodeBlock *block)
{
  if (block == NULL)
    return;

  Inst *i;
  struct list_head *p, *n;
  list_for_each_safe(p, n, &block->insts) {
    i = container_of(p, Inst, link);
    list_del(&i->link);
    Inst_Free(i);
  }

  Mfree(block);
}

void CodeBlock_Merge(CodeBlock *from, CodeBlock *to)
{
  Inst *i;
  struct list_head *p, *n;
  list_for_each_safe(p, n, &from->insts) {
    i = container_of(p, Inst, link);
    list_del(&i->link);
    from->bytes -= i->bytes;
    list_add_tail(&i->link, &to->insts);
    to->bytes += i->bytes;
    i->upbytes = to->bytes;
  }
  assert(!from->bytes);

  CodeBlock *b = from->next;
  while (b) {
    list_for_each_safe(p, n, &b->insts) {
      i = container_of(p, Inst, link);
      list_del(&i->link);
      b->bytes -= i->bytes;
      list_add_tail(&i->link, &to->insts);
      to->bytes += i->bytes;
      i->upbytes = to->bytes;
    }
    assert(!b->bytes);
    //FIXME: codeblock_free(b);
    b = b->next;
  }
}

static void arg_print(char *buf, int sz, Argument *val)
{
  if (val == NULL) {
    buf[0] = '\0';
    return;
  }

  switch (val->kind) {
  case ARG_NIL: {
    snprintf(buf, sz, "(nil)");
    break;
  }
  case ARG_INT: {
    snprintf(buf, sz, "%lld", val->ival);
    break;
  }
  case ARG_FLOAT: {
    snprintf(buf, sz, "%.16lf", val->fval);
    break;
  }
  case ARG_BOOL: {
    snprintf(buf, sz, "%s", val->bval ? "true" : "false");
    break;
  }
  case ARG_STR: {
    snprintf(buf, sz, "%s", val->str);
    break;
  }
  case ARG_UCHAR: {
    snprintf(buf, sz, "%s", (char *)&val->uch);
    break;
  }
  default: {
    assert(0);
    break;
  }
  }
}

void CodeBlock_Show(CodeBlock *block)
{
  if (!block) return;

  char buf[64];

  Log_Puts("---------CodeBlock-------");
  Log_Printf("insts:%d\n", block->bytes);
  if (!list_empty(&block->insts)) {
    int cnt = 0;
    Inst *i;
    struct list_head *pos;
    list_for_each(pos, &block->insts) {
      i = container_of(pos, Inst, link);
      Log_Printf("[%d]:\n", cnt++);
      Log_Printf("  opcode:%s\n", OpCode_String(i->op));
      buf[0] = '\0';
      arg_print(buf, sizeof(buf), &i->arg);
      Log_Printf("  arg:%s\n", buf);
      Log_Printf("  bytes:%d\n", i->bytes);
      Log_Puts("-----------------\n");
    }
  }
  Log_Puts("--------CodeBlock End----");
}

void Inst_Gen(KImage *image, Buffer *buf, Inst *i)
{
  int index = -1;
  Buffer_Write_Byte(buf, i->op);
  switch (i->op) {
    case OP_HALT: {
      break;
    }
    case OP_LOADK: {
      Argument *val = &i->arg;
      if (val->kind == ARG_INT) {
        index = KImage_Add_Integer(image, val->ival);
      } else if (val->kind == ARG_FLOAT) {
        index = KImage_Add_Float(image, val->fval);
      } else if (val->kind == ARG_BOOL) {
        index = KImage_Add_Bool(image, val->bval);
      } else if (val->kind == ARG_STR) {
        index = KImage_Add_String(image, val->str);
      } else if (val->kind == ARG_UCHAR) {
        index = KImage_Add_UChar(image, val->uch);
      }else {
        assert(0);
      }
      Buffer_Write_4Bytes(buf, index);
      break;
    }
    case OP_LOADM: {
      index = KImage_Add_String(image, i->arg.str);
      Buffer_Write_4Bytes(buf, index);
      break;
    }
    case OP_GETM:
    case OP_LOAD0: {
      break;
    }
    case OP_LOAD: {
      Buffer_Write_2Bytes(buf, i->arg.ival);
      break;
    }
    case OP_STORE: {
      Buffer_Write_2Bytes(buf, i->arg.ival);
      break;
    }
    case OP_GETFIELD: {
      index = KImage_Add_String(image, i->arg.str);
      Buffer_Write_4Bytes(buf, index);
      break;
    }
    case OP_SETFIELD: {
      index = KImage_Add_String(image, i->arg.str);
      Buffer_Write_4Bytes(buf, index);
      break;
    }
    case OP_CALL0: {
      Buffer_Write_2Bytes(buf, i->argc);
      break;
    }
    case OP_CALL:
    case OP_NEW: {
      index = KImage_Add_String(image, i->arg.str);
      Buffer_Write_4Bytes(buf, index);
      Buffer_Write_2Bytes(buf, i->argc);
      break;
    }
    case OP_RET: {
      break;
    }
    case OP_ADD:
    case OP_SUB:
    case OP_MUL:
    case OP_DIV:
    case OP_MOD:
    case OP_GT:
    case OP_GE:
    case OP_LT:
    case OP_LE:
    case OP_EQ:
    case OP_NEQ:
    case OP_NEG: {
      break;
    }
    case OP_JUMP:
    case OP_JUMP_TRUE:
    case OP_JUMP_FALSE: {
      Buffer_Write_4Bytes(buf, i->arg.ival);
      break;
    }
    case OP_NEWARRAY: {
      Buffer_Write_4Bytes(buf, i->arg.ival);
      break;
    }
    case OP_LOAD_SUBSCR:
    case OP_STORE_SUBSCR: {
      break;
    }
    default: {
      assert(0);
      break;
    }
  }
}

int CodeBlock_To_RawCode(KImage *image, CodeBlock *block, uint8 **code)
{
  if (block == NULL) {
    return 0;
  }

  Buffer buf;
  Buffer_Init(&buf, 32);
  Inst *i;
  struct list_head *pos;
  list_for_each(pos, &block->insts) {
    i = container_of(pos, Inst, link);
    Inst_Gen(image, &buf, i);
  }
  uint8 *data = Buffer_RawData(&buf);
  int size = Buffer_Size(&buf);
  Buffer_Fini(&buf);
  *code = data;
  return size;
}
