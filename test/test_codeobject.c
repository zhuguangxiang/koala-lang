
#include "koala.h"
#include "codeobject.h"

void test_codeobject(void)
{
  Object *s = String_New("hello world");
  Object *method = Object_Get_Method(s, NULL, "Length");
  assert(method);
  CodeObject *code = (CodeObject *)method;
  assert(code->kind == CFUNC_KIND);
  Object *res = code->cfunc(s, NULL);
  assert(11 == Integer_Raw(res));
  OB_DECREF(res);
  Object *dis = Object_Get_Method(method, NULL, "Disassemble");
  assert(dis);
}

int main(int argc, char *argv[])
{
  Koala_Initialize();
  test_codeobject();
  Koala_Finalize();
  return 0;
}
