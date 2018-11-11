
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "tupleobject.h"
#include "intobject.h"
#include "stringobject.h"
#include "codeobject.h"

void test_tupleobject(void)
{
  Object *tuple = Tuple_New(8);
  Tuple_Set(tuple, 0, Integer_New(100));
  Tuple_Set(tuple, 1, String_New("hello"));
  Object *s = OB_KLASS(tuple)->ob_str(tuple);
  char *str = String_RawString(s);
  printf("%s\n", str);
  assert(!strcmp("[100, 'hello']", str));
  OB_DECREF(s);
  Object *method = Object_Get_Method(tuple, "Size", OB_KLASS(tuple));
  assert(method);
  CodeObject *code = (CodeObject *)method;
  assert(code->kind == CODE_CLANG);
  Object *res = code->cl(tuple, NULL);
  assert(8 == Integer_ToCInt(res));
  OB_DECREF(res);
  res = Tuple_Get(tuple, 0);
  OB_DECREF(res);
  res = Tuple_Get(tuple, 1);
  OB_DECREF(res);
  Tuple_Free(tuple);
}

int main(int argc, char *argv[])
{
  AtomString_Init();
  Init_String_Klass();
  Init_Integer_Klass();
  Init_Tuple_Klass();
  test_tupleobject();
  Fini_String_Klass();
  Fini_Integer_Klass();
  Fini_Tuple_Klass();
  AtomString_Fini();
  return 0;
}
