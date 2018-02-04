
#include "koala.h"

void test_routine(void)
{
  Object *lang = Koala_Get_Module("koala/lang");
  Klass *klazz = Module_Get_Class(lang, "Tuple");
  ASSERT_PTR(klazz);
  Object *method = Klass_Get_Method(klazz, "Size");
  Object *tuple = Tuple_New(10);
  Routine *rt = Routine_New(method, tuple, NULL);
  Routine_Run(rt, PRIO_NORMAL);
  while (Routine_State(rt) != STATE_DEAD);
  TValue val = ValueStack_Pop(&rt->stack);
  ASSERT(VALUE_ISINT(&val));
  printf("size:%lld\n", VALUE_INT(&val));
  assert(VALUE_INT(&val) == 10);
  assert(ValueStack_Size(&rt->stack) == 0);

  method = Klass_Get_Method(klazz, "Get");
  setivalue(&val, 100);
  Tuple_Set(tuple, 0, &val);
  setbvalue(&val, 1);
  Tuple_Set(tuple, 1, &val);

  Object *args = Tuple_New(1);
  setivalue(&val, 0);
  Tuple_Set(args, 0, &val);

  rt = Routine_New(method, tuple, args);
  Routine_Run(rt, PRIO_NORMAL);
  while (Routine_State(rt) != STATE_DEAD);
  val = ValueStack_Pop(&rt->stack);
  ASSERT(VALUE_ISINT(&val));
  printf("int value:%lld\n", VALUE_INT(&val));
  assert(VALUE_INT(&val) == 100);
  assert(ValueStack_Size(&rt->stack) == 0);

  setivalue(&val, 1);
  Tuple_Set(args, 0, &val);

  rt = Routine_New(method, tuple, args);
  Routine_Run(rt, PRIO_NORMAL);
  while (Routine_State(rt) != STATE_DEAD);
  val = ValueStack_Pop(&rt->stack);
  ASSERT(VALUE_ISBOOL(&val));
  printf("bool value:%d\n", VALUE_BOOL(&val));
  assert(VALUE_BOOL(&val) == 1);
  assert(ValueStack_Size(&rt->stack) == 0);
}

int main(int argc, char *argv[]) {
  UNUSED_PARAMETER(argc);
  UNUSED_PARAMETER(argv);

  Koala_Init();
  test_routine();
  Koala_Fini();

  //thread_forever();
  return 0;
}
