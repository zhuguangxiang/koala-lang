
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "codeobject.h"
#include "stringobject.h"
#include "intobject.h"

void test_codeobject(void)
{
  Object *s = String_New("hello world");
  Object *method = Object_Get_Method(s, "Length", OB_KLASS(s));
  assert(method);
  CodeObject *code = (CodeObject *)method;
  assert(code->kind == CODE_CLANG);
  Object *res = code->cf(s, NULL);
  assert(11 == Integer_ToCInt(res));
  Object *dis = Object_Get_Method(method, "DisAssemble", OB_KLASS(method));
  assert(dis);
}

int main(int argc, char *argv[])
{
  AtomString_Init();
  Init_TypeDesc();
  Init_String_Klass();
  Init_Code_Klass();
  test_codeobject();
  Fini_Code_Klass();
  Fini_String_Klass();
  AtomString_Fini();
  return 0;
}
