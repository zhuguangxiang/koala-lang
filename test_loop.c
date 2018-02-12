
#include "koala.h"
#include "opcode.h"

Object *init_test_module(void)
{
  /*
   * test.klc
   * func add(a int, b int) int  {
   *  return a + b;
   * }
   *
   * var result = add(-123, 456);
   * io.Print("-123 + 456 = ", result);
   */
  Object *module = Koala_New_Module("test", "/");
  TypeDesc desc;
  Init_Primitive_Desc(&desc, 0, PRIMITIVE_INT);
  Module_Add_Var(module, "result", &desc, 0);

  AtomTable *atbl = Module_AtomTable(module);

  static uint8 codes[] = {
    OP_LOAD, 1, 0,
    OP_LOAD, 2, 0,
    OP_ADD,
    OP_RET
  };

  Proto *proto = Proto_New(1, "i", 2, "ii");
  Object *code = KFunc_New(2, codes, sizeof(codes));
  Module_Add_Func(module, "add", proto, code);

  int v1 = ConstItem_Set_Int(atbl, -123);
  int v2 = ConstItem_Set_Int(atbl, 456);
  int func = ConstItem_Set_String(atbl, "add");
  int field = ConstItem_Set_String(atbl, "result");
  int str = ConstItem_Set_String(atbl, "result = ");
  int path = ConstItem_Set_String(atbl, "koala/io");
  int print = ConstItem_Set_String(atbl, "Print");

  Buffer buf;
  Buffer_Init(&buf, 64);

  /* var result = add(-123, 456) */
  Buffer_Write_Byte(&buf, OP_LOADK);
  Buffer_Write_4Bytes(&buf, v2);

  Buffer_Write_Byte(&buf, OP_LOADK);
  Buffer_Write_4Bytes(&buf, v1);

  Buffer_Write_Byte(&buf, OP_LOAD);
  Buffer_Write_2Bytes(&buf, 0);

  Buffer_Write_Byte(&buf, OP_CALL);
  Buffer_Write_4Bytes(&buf, func);

  Buffer_Write_Byte(&buf, OP_LOAD);
  Buffer_Write_2Bytes(&buf, 0);

  Buffer_Write_Byte(&buf, OP_SETFIELD);
  Buffer_Write_4Bytes(&buf, field);

  /* io.Print("-123 + 456 = ", result) */
  Buffer_Write_Byte(&buf, OP_LOAD);
  Buffer_Write_2Bytes(&buf, 0);

  Buffer_Write_Byte(&buf, OP_GETFIELD);
  Buffer_Write_4Bytes(&buf, field);

  Buffer_Write_Byte(&buf, OP_LOADK);
  Buffer_Write_4Bytes(&buf, str);

  Buffer_Write_Byte(&buf, OP_LOADM);
  Buffer_Write_4Bytes(&buf, path);

  Buffer_Write_Byte(&buf, OP_CALL);
  Buffer_Write_4Bytes(&buf, print);

  Buffer_Write_Byte(&buf, OP_RET);

  uint8 *__init__ = Buffer_RawData(&buf);

  proto = Proto_New(0, NULL, 0, NULL);
  code = KFunc_New(0, __init__, Buffer_Size(&buf));
  Module_Add_Func(module, "__init__", proto, code);
  return module;
}

void test_frameloop(void)
{
  Object *module = init_test_module();
  Object *method = Module_Get_Function(module, "__init__");
  Routine *rt = Routine_New(method, module, NULL);
  Routine_Run(rt, PRIO_NORMAL);
  while (Routine_State(rt) != STATE_DEAD);
}

int main(int argc, char *argv[])
{
  UNUSED_PARAMETER(argc);
  UNUSED_PARAMETER(argv);

  Koala_Init();
  test_frameloop();
  Koala_Fini();

  return 0;
}
