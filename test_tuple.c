
#include "koala.h"

/* gcc -g -std=c99 test_tuple.c -lkoala -L. */

void test_tuple(void)
{
  Object *tuple = Tuple_New(8);
  TValue val;

  setivalue(&val, 100);
  Tuple_Set(tuple, 0, &val);

  setfltvalue(&val, 100.123);
  Tuple_Set(tuple, 1, &val);

  setbvalue(&val, 1);
  Tuple_Set(tuple, 2, &val);

  setobjvalue(&val, String_New("hello,world"));
  Tuple_Set(tuple, 3, &val);

  assert(Tuple_Size(tuple) == 8);

  val = Tuple_Get(tuple, 0);
  assert(VALUE_ISINT(&val));
  assert(VALUE_INT(&val) == 100);

  val = Tuple_Get(tuple, 1);
  assert(VALUE_ISFLOAT(&val));
  //assert(VALUE_FLOAT(&val) == 100.123);

  val = Tuple_Get(tuple, 2);
  assert(VALUE_ISBOOL(&val));
  assert(VALUE_BOOL(&val) == 1);

  val = Tuple_Get(tuple, 3);
  assert(VALUE_ISOBJECT(&val));
  OB_ASSERT_KLASS(VALUE_OBJECT(&val), String_Klass);
  char *s = String_RawString(VALUE_OBJECT(&val));
  assert(strcmp(s, "hello,world") == 0);

  val = Tuple_Get(tuple, 4);
  assert(VALUE_ISNIL(&val));

  tuple = Tuple_Get_Slice(tuple, 2, 3);
  assert(Tuple_Size(tuple) == 2);

  val = Tuple_Get(tuple, 0);
  assert(VALUE_ISBOOL(&val));
  assert(VALUE_BOOL(&val) == 1);

  val = Tuple_Get(tuple, 1);
  assert(VALUE_ISOBJECT(&val));
  OB_ASSERT_KLASS(VALUE_OBJECT(&val), String_Klass);
  s = String_RawString(VALUE_OBJECT(&val));
  assert(strcmp(s, "hello,world") == 0);

  Object *ob = Tuple_Build("ifzsO", 200, 200.234, 1, "jimmy", tuple);
  OB_ASSERT_KLASS(ob, Tuple_Klass);
  assert(Tuple_Size(ob) == 5);

  val = Tuple_Get(ob, 0);
  assert(VALUE_ISINT(&val));
  assert(VALUE_INT(&val) == 200);

  val = Tuple_Get(ob, 1);
  assert(VALUE_ISFLOAT(&val));

  val = Tuple_Get(ob, 2);
  assert(VALUE_ISBOOL(&val));
  assert(VALUE_BOOL(&val) == 1);

  val = Tuple_Get(ob, 3);
  assert(VALUE_ISOBJECT(&val));
  OB_ASSERT_KLASS(VALUE_OBJECT(&val), String_Klass);
  s = String_RawString(VALUE_OBJECT(&val));
  assert(strcmp(s, "jimmy") == 0);

  val = Tuple_Get(ob, 4);
  assert(VALUE_ISOBJECT(&val));
  OB_ASSERT_KLASS(VALUE_OBJECT(&val), Tuple_Klass);
  assert(VALUE_OBJECT(&val) == tuple);

  int ival;
  float fval;
  int bval;
  char *sval;
  Object *obj;
  Tuple_Parse(ob, "ifzsO", &ival, &fval, &bval, &sval, &obj);
  assert(ival == 200);
  assert(fval != 0.0);
  assert(bval == 1);
  assert(strcmp(sval, "jimmy") == 0);
  assert(obj == tuple);
}

int main(int argc, char *argv[]) {
  UNUSED_PARAMETER(argc);
  UNUSED_PARAMETER(argv);

  Koala_Initialize();
  test_tuple();
  Koala_Finalize();

  return 0;
}
