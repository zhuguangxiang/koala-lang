
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "typedesc.h"

void test_typedesc(void)
{
  String s;

  TypeDesc *type;

  type = TypeDesc_Get_Base(BASE_INT);
  s = TypeDesc_ToString(type);
  assert(!strcmp("int", s.str));
  //TYPE_INCREF(type);
  //TypeDesc_Free(type);

  type = TypeDesc_New_Klass("lang", "Tuple", NULL);
  s = TypeDesc_ToString(type);
  assert(!strcmp("lang.Tuple", s.str));

  type = TypeDesc_New_Array(type);
  s = TypeDesc_ToString(type);
  assert(!strcmp("[][]lang.Tuple", s.str));

  type = TypeDesc_Get_Base(BASE_INT);
  //TYPE_INCREF(type);
  Vector *arg = Vector_New();
  type = TypeDesc_New_Array(type);
  TYPE_INCREF(type);
  Vector_Append(arg, type);
  type = TypeDesc_Get_Base(BASE_STRING);
  TYPE_INCREF(type);
  Vector_Append(arg, type);

  type = TypeDesc_Get_Base(BASE_STRING);
  type = TypeDesc_New_Proto(arg, type);
  s = TypeDesc_ToString(type);
  assert(!strcmp("func([]int, string) string", s.str));
}

void test_string_to_typedesc(void)
{
  String str;
  TypeDesc *type;
  char *s = "i[sz";

  type = String_To_TypeDesc(s);
  str = TypeDesc_ToString(type);
  assert(!strcmp("int", str.str));

  type = String_To_TypeDesc(s);
  str = TypeDesc_ToString(type);
  assert(!strcmp("[]string", str.str));

  type = String_To_TypeDesc(s);
  str = TypeDesc_ToString(type);
  assert(!strcmp("bool", str.str));
  assert(!*s);

  s = "[[Mi[s"; //[][]map[int][]string
  type = String_To_TypeDesc(s);
  str = TypeDesc_ToString(type);
  assert(!strcmp("[][]map[int][]string", str.str));

  s = "...Mss"; // ...map[string]string
  type = String_To_TypeDesc(s);
  str = TypeDesc_ToString(type);
  assert(!strcmp("...map[string]string", str.str));

  s = "...A"; //...Any
  type = String_To_TypeDesc(s);
  str = TypeDesc_ToString(type);
  assert(!strcmp("...any", str.str));

  s = "S[[[i"; //set[[][][]int]
  type = String_To_TypeDesc(s);
  str = TypeDesc_ToString(type);
  assert(!strcmp("set[[][][]int]", str.str));

  s = "Llang.Tuple;"; //lang.Tuple
  type = String_To_TypeDesc(s);
  str = TypeDesc_ToString(type);
  puts(str.str);
  assert(!strcmp("lang.Tuple", str.str));

  s = "PPs:i;s:z;"; //func(func(string) int, string) bool
  type = String_To_TypeDesc(s);
  str = TypeDesc_ToString(type);
  assert(!strcmp("func(func(string) int, string) bool", str.str));

  //func(map[string]int, string, ...lang.Tuple) set[byte]
  s = "PMsis...Llang.Tuple;:Sb;";
  type = String_To_TypeDesc(s);
  str = TypeDesc_ToString(type);
  puts(str.str);
  s = "func(map[string]int, string, ...lang.Tuple) set[byte]";
  assert(!strcmp(s, str.str));
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
