
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
  Object *ob = Integer_New(100);
  Tuple_Set(tuple, 0, ob);
  OB_DECREF(ob);
  ob = String_New("hello");
  Tuple_Set(tuple, 1, ob);
  OB_DECREF(ob);
  Object *s = OB_KLASS(tuple)->ob_str(tuple);
  char *str = String_RawString(s);
  printf("%s\n", str);
  assert(!strcmp("[100, 'hello']", str));
  Object *method = Get_Method(tuple, NULL, "Size");
  assert(method);
  CodeObject *code = (CodeObject *)method;
  assert(code->kind == CFUNC_KIND);
  Object *res = code->cfunc(tuple, NULL);
  assert(8 == Integer_ToCInt(res));
  OB_DECREF(res);
  OB_DECREF(tuple);
}

int main(int argc, char *argv[])
{
  AtomString_Init();
  Init_TypeDesc();
  Init_String_Klass();
  Init_Integer_Klass();
  Init_Tuple_Klass();
  test_tupleobject();
  Fini_Tuple_Klass();
  Fini_Integer_Klass();
  Fini_String_Klass();
  Fini_TypeDesc();
  AtomString_Fini();
  return 0;
}
