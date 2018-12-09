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
#include "image.h"
#include "buffer.h"
#include "mem.h"
#include "log.h"

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
  Inst *i = mm_alloc(sizeof(Inst));
  init_list_head(&i->link);
  i->op = op;
  i->bytes = 1 + OpCode_ArgCount(op);
  if (val)
    i->arg = *val;
  return i;
}

void Inst_Free(Inst *i)
{
  free(i);
}

Inst *Inst_Append(CodeBlock *b, uint8 op, Argument *val)
{
  Log_Debug("OpCode:'%s'", OpCode_String(op));
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
  JmpInst *jmp = malloc(sizeof(JmpInst));
  jmp->inst = inst;
  jmp->type = type;
  return jmp;
}

void JmpInst_Free(JmpInst *jmp)
{
  free(jmp);
}

CodeBlock *CodeBlock_New(void)
{
  CodeBlock *b = calloc(1, sizeof(CodeBlock));
  //init_list_head(&b->link);
  //STable_Init(&b->stbl, atbl);
  init_list_head(&b->insts);
  return b;
}

void CodeBlock_Free(CodeBlock *b)
{
  if (!b) return;
  //assert(list_unlinked(&b->link));
  //STable_Fini(&b->stbl);

  Inst *i;
  struct list_head *p, *n;
  list_for_each_safe(p, n, &b->insts) {
    i = container_of(p, Inst, link);
    list_del(&i->link);
    Inst_Free(i);
  }

  free(b);
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

#if 1

static void arg_print(char *buf, int sz, Argument *val)
{
  if (!val) {
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

  Log_Debug("---------CodeBlock-------");
  Log_Debug("insts:%d", block->bytes);
  if (!list_empty(&block->insts)) {
    int cnt = 0;
    Inst *i;
    struct list_head *pos;
    list_for_each(pos, &block->insts) {
      i = container_of(pos, Inst, link);
      printf("[%d]:\n", cnt++);
      printf("  opcode:%s\n", OpCode_String(i->op));
      arg_print(buf, sizeof(buf), &i->arg);
      printf("  arg:%s\n", buf);
      printf("  bytes:%d\n", i->bytes);
      printf("-----------------\n");
    }
  }
  Log_Debug("--------CodeBlock End----");
}
#endif

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
      } else {
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

static void add_locvar(KImage *image, int index, Symbol *sym, int incls)
{
  if (Vector_Size(&sym->locvec) <= 0) {
    if (incls)
      Log_Debug("     no vars");
    else
      Log_Debug("   no vars");
    return;
  }

  Symbol *item;
  Vector_ForEach(item, &sym->locvec) {
    if (incls)
      Log_Debug("     var '%s'", item->name);
    else
      Log_Debug("   var '%s'", item->name);
    KImage_Add_LocVar(image, item->name, item->desc, item->index, index);
  }
}

struct gencode_struct {
  int bcls;
  KImage *image;
  char *clazz;
};

static void __gen_code_fn(Symbol *sym, void *arg)
{
  struct gencode_struct *tmp = arg;
  switch (sym->kind) {
    case SYM_VAR: {
      if (sym->inherited) {
        assert(tmp->bcls);
        break;
      }

      if (tmp->bcls) {
        Log_Debug("   var '%s'", sym->name);
        KImage_Add_Field(tmp->image, tmp->clazz, sym->name, sym->desc);
      } else {
        Log_Debug("%s %s:", sym->konst ? "const" : "var",
          sym->name);
        if (sym->konst)
          KImage_Add_Var(tmp->image, sym->name, sym->desc, 1);
        else
          KImage_Add_Var(tmp->image, sym->name, sym->desc, 0);
      }
      break;
    }
    case SYM_PROTO: {
      if (sym->inherited) {
        assert(tmp->bcls);
        break;
      }
      if (tmp->bcls) {
        Log_Debug("   func %s:", sym->name);
      } else {
        Log_Debug("func %s:", sym->name);
      }
      CodeBlock *b = sym->ptr;
      int locvars = sym->locvars;
      KImage *image = tmp->image;

      Buffer buf;
      Buffer_Init(&buf, 32);
      Inst *i;
      struct list_head *pos;
      list_for_each(pos, &b->insts) {
        i = container_of(pos, Inst, link);
        Inst_Gen(image, &buf, i);
      }

      uint8 *data = Buffer_RawData(&buf);
      int size = Buffer_Size(&buf);
      //code_show(data, size);
      Buffer_Fini(&buf);

      int index;
      if (tmp->bcls) {
        index = KImage_Add_Method(tmp->image, tmp->clazz, sym->name,
                                  sym->desc, data, size);
        add_locvar(tmp->image, index, sym, 1);
      } else {
        index = KImage_Add_Func(tmp->image, sym->name, sym->desc, data, size);
        add_locvar(tmp->image, index, sym, 0);
      }
      break;
    }
    case SYM_CLASS: {
      Log_Debug("----------------------");
      Log_Debug("class %s:", sym->name);
      char *path = NULL;
      char *type = NULL;
      if (sym->super) {
        path = sym->super->desc->klass.path.str;
        type = sym->super->desc->klass.type.str;
      }
      int size = Vector_Size(&sym->traits);
      Vector v;
      Vector_Init(&v);
      Symbol *trait;
      for (int i = 0; i < size; i++) {
        trait = Vector_Get(&sym->traits, i);
        Vector_Append(&v, trait->desc);
      }
      KImage_Add_Class(tmp->image, sym->name, path, type, &v);
      Vector_Fini(&v, NULL, NULL);
      struct gencode_struct tmp2 = {1, tmp->image, sym->name};
      STable_Traverse(sym->ptr, __gen_code_fn, &tmp2);
      break;
    }
    case SYM_IPROTO: {
      Log_Debug("   abstract func %s;", sym->name);
      KImage_Add_IMeth(tmp->image, tmp->clazz, sym->name, sym->desc);
      break;
    }
    case SYM_TRAIT: {
      Log_Debug("----------------------");
      Log_Debug("trait %s:", sym->name);
      int size = Vector_Size(&sym->traits);
      Vector v;
      Vector_Init(&v);
      Symbol *trait;
      for (int i = 0; i < size; i++) {
        trait = Vector_Get(&sym->traits, i);
        Vector_Append(&v, trait->desc);
      }
      KImage_Add_Trait(tmp->image, sym->name, &v);
      Vector_Fini(&v, NULL, NULL);
      struct gencode_struct tmp2 = {1, tmp->image, sym->name};
      STable_Traverse(sym->ptr, __gen_code_fn, &tmp2);
      break;
    }
    case SYM_TYPEALIAS: {
      break;
    }
    default: {
      assert(0);
    }
  }
}

void Generate_KLC_File(STable *stbl, char *pkgname, char *pkgfile)
{
  printf("----------codegen------------\n");
  KImage *image = KImage_New(pkgname);
  struct gencode_struct tmp = {0, image, NULL};
  STable_Traverse(stbl, __gen_code_fn, &tmp);
  Log_Debug("----------------------");
  KImage_Finish(image);
#if 1
  //KImage_Show(image);
#endif
  KImage_Write_File(image, pkgfile);
  printf("----------codegen end--------\n");
}
