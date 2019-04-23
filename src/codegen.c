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

LOGGER(0)

static Inst *inst_new(uint8 op, ConstValue *val)
{
  Inst *i = Malloc(sizeof(Inst));
  init_list_head(&i->link);
  i->op = op;
  i->bytes = 1 + OpCode_ArgCount(op);
  if (val)
    i->arg = *val;
  return i;
}

static void inst_free(Inst *i)
{
  Mfree(i);
}

Inst *Inst_Append(CodeBlock *b, uint8 op, ConstValue *val)
{
  Inst *i = inst_new(op, val);
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
    inst_free(i);
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

void CodeBlock_Show(CodeBlock *block)
{
  if (block == NULL || block->bytes <= 0)
    return;

  Log_Puts("---------------------------------------------");
  Log_Puts("index\toffset\tbytes\topcode\t\targument");
  if (!list_empty(&block->insts)) {
    int cnt = 0;
    Inst *i;
    char buf[64];
    char *opname;
    int offset = 0;
    list_for_each_entry(i, &block->insts, link) {
      opname = OpCode_String(i->op);
      Log_Printf("%d", cnt++);
      Log_Printf("\t%d", offset);
      Log_Printf("\t%d", i->bytes);
      offset += i->bytes;
      Log_Printf("\t%s", opname);
      buf[0] = '\0';
      Const_Show(&i->arg, buf);
      if (strlen(opname) < 8)
        Log_Printf("\t\t%s\n", buf);
      else
        Log_Printf("\t%s\n", buf);
    }
  }
}

static void inst_gen(KImage *image, Buffer *buf, Inst *i)
{
  int index = -1;
  Buffer_Write_Byte(buf, i->op);
  switch (i->op) {
  case LOAD_CONST: {
    ConstValue *val = &i->arg;
    index = KImage_Add_Const(image, val);
    Buffer_Write_2Bytes(buf, index);
    break;
  }
  case LOAD_PKG:
    index = KImage_Add_String(image, i->arg.str);
    Buffer_Write_2Bytes(buf, index);
    break;
  case LOAD:
    Buffer_Write_Byte(buf, i->arg.ival);
    break;
  case STORE:
    Buffer_Write_Byte(buf, i->arg.ival);
    break;
  case GET_ATTR:
    index = KImage_Add_String(image, i->arg.str);
    Buffer_Write_2Bytes(buf, index);
    break;
  case SET_ATTR:
    index = KImage_Add_String(image, i->arg.str);
    Buffer_Write_2Bytes(buf, index);
    break;
  case CALL:
    index = KImage_Add_String(image, i->arg.str);
    Buffer_Write_2Bytes(buf, index);
    Buffer_Write_Byte(buf, i->argc);
    break;
  case NEW:
    index = KImage_Add_String(image, i->arg.str);
    Buffer_Write_4Bytes(buf, index);
    Buffer_Write_2Bytes(buf, i->argc);
    break;
  case NEW_ENUM:
    index = KImage_Add_String(image, i->arg.str);
    Buffer_Write_2Bytes(buf, index);
    Buffer_Write_Byte(buf, i->argc);
    break;
  case DUP:
  case LOAD_0:
  case LOAD_1:
  case LOAD_2:
  case LOAD_3:
  case STORE_0:
  case STORE_1:
  case STORE_2:
  case STORE_3:
  case RETURN:
  case ADD:
  case SUB:
  case MUL:
  case DIV:
  case MOD:
  case GT:
  case GE:
  case LT:
  case LE:
  case EQ:
  case NEQ:
  case NEG:
    break;
  case JMP:
  case JMP_TRUE:
  case JMP_FALSE:
    Buffer_Write_2Bytes(buf, i->arg.ival);
    break;
  case NEW_ARRAY:
    Buffer_Write_4Bytes(buf, i->arg.ival);
    break;
  default:
    assert(0);
    break;
  }
}

int CodeBlock_To_RawCode(KImage *image, CodeBlock *block, uint8 **code)
{
  if (block == NULL)
    return 0;

  Buffer buf;
  Buffer_Init(&buf, 32);
  Inst *i;
  list_for_each_entry(i, &block->insts, link) {
    inst_gen(image, &buf, i);
  }
  uint8 *data = Buffer_RawData(&buf);
  int size = Buffer_Size(&buf);
  Buffer_Fini(&buf);
  *code = data;
  return size;
}
