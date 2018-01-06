
#include "koala.h"

/* gcc -g -std=c99 test_object.c -lkoala -L. */

void test_object(void)
{
  TValue val = NIL_VALUE_INIT();
  TValue_Build(&val, 'i', 200);
  assert(VALUE_ISINT(&val) && VALUE_INT(&val) == 200);

  int ival;
  TValue_Parse(&val, 'i', &ival);
  assert(ival == 200);

  TValue_Build(&val, 'f', 200);
  assert(VALUE_ISFLOAT(&val));

  TValue_Build(&val, 'z', 1);
  assert(VALUE_ISBOOL(&val) && VALUE_BOOL(&val) == 1);

  TValue_Build(&val, 's', "jimmy");
  assert(VALUE_ISOBJECT(&val));
  OB_ASSERT_KLASS(VALUE_OBJECT(&val), String_Klass);
  char *s = String_To_CString(VALUE_OBJECT(&val));
  assert(strcmp(s, "jimmy") == 0);

  TValue_Parse(&val, 's', &s);
  assert(strcmp(s, "jimmy") == 0);

  TValue_Build(&val, 'O', String_New("hello"));
  assert(VALUE_ISOBJECT(&val));
  OB_ASSERT_KLASS(VALUE_OBJECT(&val), String_Klass);
  s = String_To_CString(VALUE_OBJECT(&val));
  assert(strcmp(s, "hello") == 0);

  Object *ob;
  TValue_Parse(&val, 'O', &ob);
  assert(ob);
  s = String_To_CString(ob);
  assert(strcmp(s, "hello") == 0);

  ob = GState_Find_Module(&gs, "koala/lang");
  Object *klazz;
  Module_Get_Class(ob, "Tuple", &klazz);
  KLASS_ASSERT(klazz, Tuple_Klass);
  ob = Klass_Get_Method((Klass *)klazz, "Get");
  assert(ob);
  OB_ASSERT_KLASS(ob, Method_Klass);
}

int main(int argc, char *argv[]) {
  UNUSED_PARAMETER(argc);
  UNUSED_PARAMETER(argv);

  Koala_Initialize();
  test_object();
  Koala_Finalize();

  return 0;
}
