
#include "koala.h"

void test_io(void)
{
  Package *pkg = Koala_Get_Package("io");
  assert(pkg);
  Object *func = Koala_Get_Function(pkg, "Println");
  assert(func && IsCFunc(func));
  CodeObject *co = (CodeObject *)func;
  Object *stro = String_New("hello, koala");
  Object *into = Integer_New(1230);
  co->cfunc(NULL, stro);
  co->cfunc(NULL, into);
  Object *tuple = Tuple_New(2);
  Tuple_Append(tuple, stro);
  Tuple_Append(tuple, into);
  co->cfunc(NULL, tuple);
  Object *args = Tuple_New(1);
  Tuple_Append(args, tuple);
  co->cfunc(NULL, args);
  OB_DECREF(stro);
  OB_DECREF(into);
  OB_DECREF(tuple);
  OB_DECREF(args);
}

int main(int argc, char *argv[])
{
  Koala_Initialize();
  test_io();
  Koala_Finalize();
  return 0;
}
