/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "codegen.h"
#include "bytebuffer.h"
#include "memory.h"
#include "log.h"

static Inst *inst_new(uint8_t op, ConstValue *val)
{
  Inst *i = kmalloc(sizeof(Inst));
  INIT_LIST_HEAD(&i->link);
  i->op = op;
  i->bytes = 1 + opcode_argc(op);
  if (val)
    i->arg = *val;
  return i;
}

static void inst_free(Inst *i)
{
  kfree(i);
}

Inst *Inst_Append(CodeBlock *b, uint8_t op, ConstValue *val)
{
  Inst *i = inst_new(op, val);
  list_add_tail(&i->link, &b->insts);
  b->bytes += i->bytes;
  i->upbytes = b->bytes;
  return i;
}

Inst *Inst_Append_NoArg(CodeBlock *b, uint8_t op)
{
  return Inst_Append(b, op, NULL);
}

JmpInst *JmpInst_New(Inst *inst, int type)
{
  JmpInst *jmp = kmalloc(sizeof(JmpInst));
  jmp->inst = inst;
  jmp->type = type;
  return jmp;
}

void JmpInst_Free(JmpInst *jmp)
{
  kfree(jmp);
}

CodeBlock *CodeBlock_New(void)
{
  CodeBlock *b = kmalloc(sizeof(CodeBlock));
  INIT_LIST_HEAD(&b->insts);
  return b;
}

void CodeBlock_Free(CodeBlock *block)
{
  if (block == NULL)
    return;

  struct list_head *p, *n;
  list_for_each_safe(p, n, &block->insts) {
    list_del(p);
    inst_free((Inst *)p);
  }

  kfree(block);
}

void CodeBlock_Merge(CodeBlock *from, CodeBlock *to)
{
  Inst *i;
  struct list_head *p, *n;
  list_for_each_safe(p, n, &from->insts) {
    i = (Inst *)p;
    list_del(&i->link);
    from->bytes -= i->bytes;
    list_add_tail(&i->link, &to->insts);
    to->bytes += i->bytes;
    i->upbytes = to->bytes;
  }
  panic(from->bytes != 0, "block has %d bytes left", from->bytes);

  CodeBlock *b = from->next;
  while (b) {
    list_for_each_safe(p, n, &b->insts) {
      i = (Inst *)p;
      list_del(&i->link);
      b->bytes -= i->bytes;
      list_add_tail(&i->link, &to->insts);
      to->bytes += i->bytes;
      i->upbytes = to->bytes;
    }
    panic(b->bytes != 0, "block has %d bytes left", b->bytes);
    //FIXME: codeblock_free(b);
    b = b->next;
  }
}

void CodeBlock_Show(CodeBlock *block)
{
  if (block == NULL || block->bytes <= 0)
    return;

  puts("---------------------------------------------");
  puts("index\toffset\tbytes\topcode\t\targument");
  if (!list_empty(&block->insts)) {
    int cnt = 0;
    Inst *i;
    STRBUF(sbuf);
    char *opname;
    int offset = 0;
    struct list_head *p;
    list_for_each(p, &block->insts) {
      i = (Inst *)p;
      opname = opcode_str(i->op);
      printf("%d", cnt++);
      printf("\t%d", offset);
      printf("\t%d", i->bytes);
      offset += i->bytes;
      printf("\t%s", opname);
      Const_Show(&i->arg, &sbuf);
      if (strlen(opname) < 8)
        printf("\t\t%s\n", strbuf_tostr(&sbuf));
      else
        printf("\t%s\n", strbuf_tostr(&sbuf));
      strbuf_fini(&sbuf);
    }
  }
}

static void inst_gen(Image *image, ByteBuffer *buf, Inst *i)
{
  int index = -1;
  bytebuffer_write_byte(buf, i->op);
  switch (i->op) {
  case LOAD_CONST: {
    ConstValue *val = &i->arg;
    index = Image_Add_ConstValue(image, val);
    bytebuffer_write_2bytes(buf, index);
    break;
  }
  case LOAD_MODULE:
    index = Image_Add_String(image, i->arg.str);
    bytebuffer_write_2bytes(buf, index);
    break;
  case LOAD:
  case STORE:
    bytebuffer_write_byte(buf, i->arg.ival);
    break;
  case GET_FIELD:
    index = Image_Add_String(image, i->arg.str);
    bytebuffer_write_2bytes(buf, index);
    break;
  case SET_FIELD:
    index = Image_Add_String(image, i->arg.str);
    bytebuffer_write_2bytes(buf, index);
    break;
  case CALL:
  case NEW_EVAL:
    index = Image_Add_String(image, i->arg.str);
    bytebuffer_write_2bytes(buf, index);
    bytebuffer_write_byte(buf, i->argc);
    break;
  case NEW:
    index = Image_Add_String(image, i->arg.str);
    bytebuffer_write_2bytes(buf, index);
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
  case OP_RETURN:
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
    bytebuffer_write_2bytes(buf, i->arg.ival);
    break;
  case NEW_ARRAY:
    bytebuffer_write_4bytes(buf, i->arg.ival);
    break;
  default:
    panic(1, "invalid opcode %s", opcode_str(i->op));
    break;
  }
}

int CodeBlock_To_RawCode(Image *image, CodeBlock *block, uint8_t **code)
{
  if (block == NULL)
    return 0;

  ByteBuffer buf;
  bytebuffer_init(&buf, 32);
  Inst *i;
  struct list_head *p;
  list_for_each(p, &block->insts) {
    i = (Inst *)p;
    inst_gen(image, &buf, i);
  }
  int size = bytebuffer_toarr(&buf, (char **)code);
  bytebuffer_fini(&buf);
  return size;
}
