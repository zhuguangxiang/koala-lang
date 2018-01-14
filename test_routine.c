
#include "koala.h"

void test_routine(void)
{
  Object *lang = KState_Get_Module(&ks, "koala/lang");
  Object *klazz = Module_Get_Class(lang, "Tuple");
  ASSERT_PTR(klazz);
  Object *method = Klass_Get_Method((Klass *)klazz, "Size");
  Object *tuple = Tuple_New(10);
  Object *rt = Routine_New(method, tuple, NULL);
  Routine_Run(rt, PRIO_NORMAL);
  while (Routine_State(rt) != STATE_DEAD);
  TValue val = rt_stack_pop(rt);
  ASSERT(VALUE_ISINT(&val));
  printf("size:%lld\n", VALUE_INT(&val));
  assert(VALUE_INT(&val) == 10);
  assert(rt_stack_size(rt) == 0);

  method = Klass_Get_Method((Klass *)klazz, "Get");
  set_int_value(&val, 100);
  Tuple_Set(tuple, 0, &val);
  set_bool_value(&val, 1);
  Tuple_Set(tuple, 1, &val);

  Object *args = Tuple_New(1);
  set_int_value(&val, 0);
  Tuple_Set(args, 0, &val);

  rt = Routine_New(method, tuple, args);
  Routine_Run(rt, PRIO_NORMAL);
  while (Routine_State(rt) != STATE_DEAD);
  val = rt_stack_pop(rt);
  ASSERT(VALUE_ISINT(&val));
  printf("int value:%lld\n", VALUE_INT(&val));
  assert(VALUE_INT(&val) == 100);
  assert(rt_stack_size(rt) == 0);

  set_int_value(&val, 1);
  Tuple_Set(args, 0, &val);

  rt = Routine_New(method, tuple, args);
  Routine_Run(rt, PRIO_NORMAL);
  while (Routine_State(rt) != STATE_DEAD);
  val = rt_stack_pop(rt);
  ASSERT(VALUE_ISBOOL(&val));
  printf("bool value:%d\n", VALUE_BOOL(&val));
  assert(VALUE_BOOL(&val) == 1);
  assert(rt_stack_size(rt) == 0);
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
