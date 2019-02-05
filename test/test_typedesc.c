
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "typedesc.h"

TypeDesc *String_To_TypeDesc(char **string, int _dims, int _varg);

void test_typedesc(void)
{
  char buf[64];

  TypeDesc *type;

  type = TypeDesc_Get_Basic(BASIC_INT);
  TypeDesc_ToString(type, buf);
  assert(!strcmp("int", buf));
  //TYPE_INCREF(type);
  //TypeDesc_Free(type);

  type = TypeDesc_Get_Klass("lang", "Tuple");
  TypeDesc_ToString(type, buf);
  assert(!strcmp("lang.Tuple", buf));

  type = TypeDesc_Get_Array(2, type);
  TypeDesc_ToString(type, buf);
  assert(!strcmp("[][]lang.Tuple", buf));

  type = TypeDesc_Get_Basic(BASIC_INT);
  //TYPE_INCREF(type);
  Vector *arg = Vector_New();
  type = TypeDesc_Get_Array(1, type);
  TYPE_INCREF(type);
  Vector_Append(arg, type);
  type = TypeDesc_Get_Basic(BASIC_STRING);
  TYPE_INCREF(type);
  Vector_Append(arg, type);

  Vector *ret = Vector_New();
  type = TypeDesc_Get_Basic(BASIC_INT);
  TYPE_INCREF(type);
  Vector_Append(ret, type);
  type = TypeDesc_Get_Basic(BASIC_STRING);
  TYPE_INCREF(type);
  Vector_Append(ret, type);

  type = TypeDesc_Get_Proto(arg, ret);
  TypeDesc_ToString(type, buf);
  assert(!strcmp("func([]int, string) (int, string)", buf));
}

void test_string_to_typedesc(void)
{
  char buf[64];
  TypeDesc *type;
  char *s = "i[sz";

  type = String_To_TypeDesc(&s, 0, 0);
  TypeDesc_ToString(type, buf);
  assert(!strcmp("int", buf));

  type = String_To_TypeDesc(&s, 0, 0);
  TypeDesc_ToString(type, buf);
  assert(!strcmp("[]string", buf));

  type = String_To_TypeDesc(&s, 0, 0);
  TypeDesc_ToString(type, buf);
  assert(!strcmp("bool", buf));
  assert(!*s);

  s = "[[Mi[s"; //[][]map[int][]string
  type = String_To_TypeDesc(&s, 0, 0);
  TypeDesc_ToString(type, buf);
  assert(!strcmp("[][]map[int][]string", buf));

  s = "...Mss"; // ...map[string]string
  type = String_To_TypeDesc(&s, 0, 0);
  TypeDesc_ToString(type, buf);
  assert(!strcmp("...map[string]string", buf));

  s = "...A"; //...Any
  type = String_To_TypeDesc(&s, 0, 0);
  TypeDesc_ToString(type, buf);
  assert(!strcmp("...any", buf));

  s = "S[[[i"; //set[[][][]int]
  type = String_To_TypeDesc(&s, 0, 0);
  TypeDesc_ToString(type, buf);
  assert(!strcmp("set[[][][]int]", buf));

  s = "Llang.Tuple;"; //lang.Tuple
  type = String_To_TypeDesc(&s, 0, 0);
  TypeDesc_ToString(type, buf);
  puts(buf);
  assert(!strcmp("lang.Tuple", buf));

  s = "PPs:i;s:e;"; //func(func(string) int, string) error
  type = String_To_TypeDesc(&s, 0, 0);
  TypeDesc_ToString(type, buf);
  assert(!strcmp("func(func(string) int, string) error", buf));

  //func(map[string]int, string, ...lang.Tuple) (int, []error, set[byte])
  s = "PMsis...Llang.Tuple;:i[eSb;";
  type = String_To_TypeDesc(&s, 0, 0);
  TypeDesc_ToString(type, buf);
  puts(buf);
  s = "func(map[string]int, string, ...lang.Tuple) (int, []error, set[byte])";
  assert(!strcmp(s, buf));
}

int main(int argc, char *argv[])
{
  AtomString_Init();
  Init_TypeDesc();
  test_typedesc();
  test_string_to_typedesc();
  Fini_TypeDesc();
  AtomString_Fini();
  return 0;
}
