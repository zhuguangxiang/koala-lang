
#include "koala.h"

void test_tupleobject(void)
{
  Object *tuple = Tuple_New(8);
  Object *ob = Integer_New(100);
  Tuple_Set(tuple, 0, ob);
  OB_DECREF(ob);
  ob = String_New("hello");
  Tuple_Set(tuple, 1, ob);
  OB_DECREF(ob);
  Object *s = To_String(tuple);
  char *str = String_Raw(s);
  printf("%s\n", str);
  assert(!strcmp("[100, 'hello']", str));
  Object *method = Get_Method(tuple, NULL, "Size");
  assert(method);
  CodeObject *code = (CodeObject *)method;
  assert(code->kind == CFUNC_KIND);
  Object *res = code->cfunc(tuple, NULL);
  assert(8 == Integer_Raw(res));
  OB_DECREF(res);
  OB_DECREF(tuple);
}

int main(int argc, char *argv[])
{
  Koala_Initialize();
  test_tupleobject();
  Koala_Finalize();
  return 0;
}
