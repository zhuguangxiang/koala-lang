
#include "codegen.h"
#include "opcode.h"
#include "log.h"

Inst *Inst_New(uint8 op, Argument *val)
{
  Inst *i = calloc(1, sizeof(Inst));
  init_list_head(&i->link);
  i->op = op;
  i->bytes = 1 + opcode_argsize(op);
  if (val) i->arg = *val;
  return i;
}

void Inst_Free(Inst *i)
{
  free(i);
}

Inst *Inst_Append(CodeBlock *b, uint8 op, Argument *val)
{
  debug("inst:'%s'", opcode_string(op));
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

void Inst_Gen(AtomTable *atbl, Buffer *buf, Inst *i)
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
        index = ConstItem_Set_Int(atbl, val->ival);
      } else if (val->kind == ARG_FLOAT) {
        index = ConstItem_Set_Float(atbl, val->fval);
      } else if (val->kind == ARG_BOOL) {
        index = ConstItem_Set_Bool(atbl, val->bval);
      } else if (val->kind == ARG_STR) {
        index = ConstItem_Set_String(atbl, val->str);
      } else {
        assert(0);
      }
      Buffer_Write_4Bytes(buf, index);
      break;
    }
    case OP_LOADM: {
      index = ConstItem_Set_String(atbl, i->arg.str);
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
      index = ConstItem_Set_String(atbl, i->arg.str);
      Buffer_Write_4Bytes(buf, index);
      break;
    }
    case OP_SETFIELD: {
      index = ConstItem_Set_String(atbl, i->arg.str);
      Buffer_Write_4Bytes(buf, index);
      break;
    }
    case OP_CALL0: {
      Buffer_Write_2Bytes(buf, i->argc);
      break;
    }
    case OP_CALL:
    case OP_NEW: {
      index = ConstItem_Set_String(atbl, i->arg.str);
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
    default: {
      assert(0);
      break;
    }
  }
}

static void add_locvar(KImage *image, int index, Symbol *sym, int flags,
  int incls)
{
  if (Vector_Size(&sym->locvec) <= 0) {
    if (incls)
      debug("     no vars");
    else
      debug("   no vars");
    return;
  }

  Symbol *item;
  Vector_ForEach(item, &sym->locvec) {
    if (incls)
      debug("     var '%s'", item->name);
    else
      debug("   var '%s'", item->name);
    KImage_Add_LocVar(image, item->name, item->desc, item->index,
      flags, index);
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
        debug("   var '%s'", sym->name);
        KImage_Add_Field(tmp->image, tmp->clazz, sym->name, sym->desc);
      } else {
        debug("%s %s:", sym->access & ACCESS_CONST ? "const" : "var",
          sym->name);
        if (sym->access & ACCESS_CONST)
          KImage_Add_Const(tmp->image, sym->name, sym->desc);
        else
          KImage_Add_Var(tmp->image, sym->name, sym->desc);
      }
      break;
    }
    case SYM_PROTO: {
      if (sym->inherited) {
        assert(tmp->bcls);
        break;
      }
      if (tmp->bcls) {
        debug("   func %s:", sym->name);
      } else {
        debug("func %s:", sym->name);
      }
      CodeBlock *b = sym->ptr;
      int locvars = sym->locvars;
      AtomTable *atbl = tmp->image->table;

      Buffer buf;
      Buffer_Init(&buf, 32);
      Inst *i;
      list_for_each_entry(i, &b->insts, link) {
        Inst_Gen(atbl, &buf, i);
      }

      uint8 *data = Buffer_RawData(&buf);
      int size = Buffer_Size(&buf);
      //code_show(data, size);
      Buffer_Fini(&buf);

      int index;
      if (tmp->bcls) {
        index = KImage_Add_Method(tmp->image, tmp->clazz, sym->name,
          sym->desc, locvars, data, size);
        add_locvar(tmp->image, index, sym, METHLOCVAR, 1);
      } else {
        index = KImage_Add_Func(tmp->image, sym->name, sym->desc, locvars,
          data, size);
        add_locvar(tmp->image, index, sym, FUNCLOCVAR, 0);
      }
      break;
    }
    case SYM_CLASS: {
      debug("----------------------");
      debug("class %s:", sym->name);
      char *path = NULL;
      char *type = NULL;
      if (sym->super) {
        path = sym->super->desc->usrdef.path;
        type = sym->super->desc->usrdef.type;
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
      debug("   abstract func %s;", sym->name);
      KImage_Add_IMeth(tmp->image, tmp->clazz, sym->name, sym->desc);
      break;
    }
    case SYM_TRAIT: {
      debug("----------------------");
      debug("trait %s:", sym->name);
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
      kassert(0, "unknown symbol kind:%d", sym->kind);
    }
  }
}

void codegen_klc(ParserState *ps, char *out)
{
  printf("----------codegen------------\n");
  KImage *image = KImage_New(ps->package);
  struct gencode_struct tmp = {0, image, NULL};
  STable_Traverse(ps->sym->ptr, __gen_code_fn, &tmp);
  debug("----------------------");
  KImage_Finish(image);
#if 1
  KImage_Show(image);
#endif
  KImage_Write_File(image, out);
  printf("----------codegen end--------\n");
}

void codegen_binary(ParserState *ps, int op)
{
  switch (op) {
    case BINARY_ADD: {
      debug("add 'OP_ADD'");
      Inst_Append(ps->u->block, OP_ADD, NULL);
      break;
    }
    case BINARY_SUB: {
      debug("add 'OP_SUB'");
      Inst_Append(ps->u->block, OP_SUB, NULL);
      break;
    }
  case BINARY_MULT: {
    debug("add 'OP_MUL'");
    Inst_Append(ps->u->block, OP_MUL, NULL);
    break;
  }
  case BINARY_DIV: {
    debug("add 'OP_DIV'");
    Inst_Append(ps->u->block, OP_DIV, NULL);
    break;
  }
  case BINARY_MOD: {
    debug("add 'OP_MOD'");
    Inst_Append(ps->u->block, OP_MOD, NULL);
    break;
  }
    case BINARY_GT: {
      debug("add 'OP_GT'");
      Inst_Append(ps->u->block, OP_GT, NULL);
      break;
    }
    case BINARY_LT: {
      debug("add 'OP_LT'");
      Inst_Append(ps->u->block, OP_LT, NULL);
      break;
    }
    case BINARY_EQ: {
      debug("add 'OP_EQ'");
      Inst_Append(ps->u->block, OP_EQ, NULL);
      break;
    }
    default: {
      assert(0);
      break;
    }
  }
}

void codegen_unary(ParserState *ps, int op)
{
  switch (op) {
    case UNARY_PLUS: {
      debug("omit 'OP_PLUS'");
      break;
    }
    case UNARY_MINUS: {
      debug("add 'OP_NEG'");
      Inst_Append(ps->u->block, OP_NEG, NULL);
      break;
    }
    case UNARY_BIT_NOT: {
      debug("add 'OP_BNOT'");
      Inst_Append(ps->u->block, OP_BNOT, NULL);
      break;
    }
    case UNARY_LNOT: {
      debug("add 'OP_LNOT'");
      Inst_Append(ps->u->block, OP_LNOT, NULL);
      break;
    }
    default: {
      assert(0);
      break;
    }
  }
}
