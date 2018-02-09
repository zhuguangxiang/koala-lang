
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
  Object *module = Module_New("test", "/");
  TypeDesc desc;
  INIT_PRIMITIVE_DESC(&desc, 0, PRIMITIVE_INT);
  Module_Add_Var(module, "result", &desc, 0);

  static uint8 codes[] = {
    OP_LOAD, 2, 0, 0, 0,
    OP_LOAD, 1, 0, 0, 0,
    OP_ADD,
    OP_RET
  };

  ProtoInfo proto;
  CodeInfo codeinfo;
  FuncInfo funcinfo;
  DECL_FUNCTYPE_INIT(functype, 1, "i", 2, "ii");
  Init_ProtoInfo(&functype, &proto);
  Init_CodeInfo(codes, sizeof(codes), NULL, 0, &codeinfo);
  Init_FuncInfo(&proto, &codeinfo, 0, &funcinfo);
  Object *meth = Method_New(&funcinfo, Module_AtomTable(module));
  Module_Add_Func(module, "add", &proto, meth);

  static uint8 __init__[] = {
    /* var result = add(-123, 456) */
    OP_LOADK, 0, 0, 0, 0,
    OP_LOADK, 1, 0, 0, 0,
    OP_LOAD, 0, 0, 0, 0,
    OP_CALL, 2, 0, 0, 0,
    OP_LOAD, 0, 0, 0, 0,
    OP_SETFIELD, 0, 0, 0, 0,

    /* io.Print("-123 + 456 = ", result) */
    OP_LOAD, 0, 0, 0, 0,
    OP_GETFIELD, 0, 0, 0, 0,
    OP_LOADK, 6, 0, 0, 0,
    OP_LOADM, 4, 0, 0, 0,
    OP_CALL, 5, 0, 0, 0,

    /* return */
    OP_RET,
  };

  AtomTable *itable = Module_AtomTable(module);
  int index1 = StringItem_Set(itable, "add");
  int index2 = StringItem_Set(itable, "result");
  int index3 = StringItem_Set(itable, "koala/io");
  int index4 = StringItem_Set(itable, "Print");
  int index5 = StringItem_Set(itable, "-123 + 368 = ");

  static ConstItem k[10] = {
    CONST_IVAL_INIT(368),
    CONST_IVAL_INIT(-123),
  };

  const_setstrvalue(&k[2], index1);
  const_setstrvalue(&k[3], index2);
  const_setstrvalue(&k[4], index3);
  const_setstrvalue(&k[5], index4);
  const_setstrvalue(&k[6], index5);

  INIT_FUNCTYPE(&functype, 0, NULL, 0, NULL);
  Init_ProtoInfo(&functype, &proto);
  Init_CodeInfo(__init__, sizeof(__init__), k, 10, &codeinfo);
  Init_FuncInfo(&proto, &codeinfo, 0, &funcinfo);
  meth = Method_New(&funcinfo, Module_AtomTable(module));
  Module_Add_Func(module, "__init__", &proto, meth);
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
