
#include "codegen.h"

typedef void (*code_gen_t)(FuncData *, void *);

void int_code_gen(FuncData *func, struct expr *exp)
{
  int index = ConstItem_Set_Int(func->itable, exp->ival);
  Buffer_Write_Byte(&func->buf, OP_LOADK);
  Buffer_Write_4Bytes(&func->buf, index);
}

void float_code_gen(FuncData *func, struct expr *exp)
{
  int index = ConstItem_Set_Float(func->itable, exp->fval);
  Buffer_Write_Byte(&func->buf, OP_LOADK);
  Buffer_Write_4Bytes(&func->buf, index);
}

void bool_code_gen(FuncData *func, struct expr *exp)
{
  int index = ConstItem_Set_Bool(func->itable, exp->bval);
  Buffer_Write_Byte(&func->buf, OP_LOADK);
  Buffer_Write_4Bytes(&func->buf, index);
}

void string_code_gen(FuncData *func, struct expr *exp)
{
  ItemTable *itable = Object_ItemTable(func->owner);
  int index = StringItem_Set(itable, exp->str);
  index = ConstItem_Set_String(func->itable, index);
  Buffer_Write_Byte(&func->buf, OP_LOADK);
  Buffer_Write_4Bytes(&func->buf, index);
}

void expr_generate_code(FuncData *func, struct expr *exp)
{
  static code_gen_t gens[] = {
    NULL, /* invalid */
    NULL,
    (code_gen_t)int_code_gen,
    (code_gen_t)float_code_gen,
    (code_gen_t)bool_code_gen,
    (code_gen_t)string_code_gen,
  };
  ASSERT_MSG(exp->kind > 0 && exp->kind < nr_elts(gens),
             "kind %d", exp->kind);
  code_gen_t gen = gens[exp->kind];
  ASSERT_PTR(gen);
  gen(func, exp);
}
