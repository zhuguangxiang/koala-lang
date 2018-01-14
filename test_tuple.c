
#include "koala.h"

/* gcc -g -std=c99 test_tuple.c -lkoala -L. */

void test_tuple(void)
{
  Object *tuple = Tuple_New(8);
  TValue val;

  set_int_value(&val, 100);
  Tuple_Set(tuple, 0, &val);

  set_float_value(&val, 100.123);
  Tuple_Set(tuple, 1, &val);

  set_bool_value(&val, 1);
  Tuple_Set(tuple, 2, &val);

  set_object_value(&val, String_New("hello,world"));
  Tuple_Set(tuple, 3, &val);

  ASSERT(Tuple_Size(tuple) == 8);

  Tuple_Get(tuple, 0, &val);
  ASSERT(VALUE_ISINT(&val));
  ASSERT(VALUE_INT(&val) == 100);

  Tuple_Get(tuple, 1, &val);
  ASSERT(VALUE_ISFLOAT(&val));
  //ASSERT(VALUE_FLOAT(&val) == 100.123);

  Tuple_Get(tuple, 2, &val);
  ASSERT(VALUE_ISBOOL(&val));
  ASSERT(VALUE_BOOL(&val) == 1);

  Tuple_Get(tuple, 3, &val);
  ASSERT(VALUE_ISOBJECT(&val));
  OB_ASSERT_KLASS(VALUE_OBJECT(&val), String_Klass);
  char *s = String_To_CString(VALUE_OBJECT(&val));
  ASSERT(strcmp(s, "hello,world") == 0);

  Tuple_Get(tuple, 4, &val);
  ASSERT(VALUE_ISNIL(&val));

  tuple = Tuple_Get_Slice(tuple, 2, 3);
  ASSERT(Tuple_Size(tuple) == 2);

  Tuple_Get(tuple, 0, &val);
  ASSERT(VALUE_ISBOOL(&val));
  ASSERT(VALUE_BOOL(&val) == 1);

  Tuple_Get(tuple, 1, &val);
  ASSERT(VALUE_ISOBJECT(&val));
  OB_ASSERT_KLASS(VALUE_OBJECT(&val), String_Klass);
  s = String_To_CString(VALUE_OBJECT(&val));
  ASSERT(strcmp(s, "hello,world") == 0);

  Object *ob = Tuple_Build("ifzsO", 200, 200.234, 1, "jimmy", tuple);
  OB_ASSERT_KLASS(ob, Tuple_Klass);
  ASSERT(Tuple_Size(ob) == 5);

  Tuple_Get(ob, 0, &val);
  ASSERT(VALUE_ISINT(&val));
  ASSERT(VALUE_INT(&val) == 200);

  Tuple_Get(ob, 1, &val);
  ASSERT(VALUE_ISFLOAT(&val));

  Tuple_Get(ob, 2, &val);
  ASSERT(VALUE_ISBOOL(&val));
  ASSERT(VALUE_BOOL(&val) == 1);

  Tuple_Get(ob, 3, &val);
  ASSERT(VALUE_ISOBJECT(&val));
  OB_ASSERT_KLASS(VALUE_OBJECT(&val), String_Klass);
  s = String_To_CString(VALUE_OBJECT(&val));
  ASSERT(strcmp(s, "jimmy") == 0);

  Tuple_Get(ob, 4, &val);
  ASSERT(VALUE_ISOBJECT(&val));
  OB_ASSERT_KLASS(VALUE_OBJECT(&val), Tuple_Klass);
  ASSERT(VALUE_OBJECT(&val) == tuple);

  int ival;
  float fval;
  int bval;
  char *sval;
  Object *obj;
  Tuple_Parse(ob, "ifzsO", &ival, &fval, &bval, &sval, &obj);
  ASSERT(ival == 200);
  ASSERT(fval != 0.0);
  ASSERT(bval == 1);
  ASSERT(strcmp(sval, "jimmy") == 0);
  ASSERT(obj == tuple);
}

int main(int argc, char *argv[]) {
  UNUSED_PARAMETER(argc);
  UNUSED_PARAMETER(argv);

  Koala_Init();
  test_tuple();
  Koala_Fini();

  return 0;
}
