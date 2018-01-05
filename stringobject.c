
#include "stringobject.h"
#include "tupleobject.h"
#include "symbol.h"
#include "hash.h"

Object *String_New(char *str)
{
  StringObject *s = malloc(sizeof(*s));
  init_object_head(s, &String_Klass);
  s->len = strlen(str);
  s->str = str;
  //Object_Add_GCList(s);
  return (Object *)s;
}

void String_Free(Object *ob)
{
  OB_ASSERT_KLASS(ob, String_Klass);
  free(ob);
}

char *String_To_CString(Object *ob)
{
  OB_ASSERT_KLASS(ob, String_Klass);
  StringObject *s = (StringObject *)ob;
  return s->str;
}

/*-------------------------------------------------------------------------*/

static Object *__string_length(Object *ob, Object *args)
{
  UNUSED_PARAMETER(args);
  OB_ASSERT_KLASS(ob, String_Klass);
  StringObject *s = (StringObject *)ob;
  return Tuple_Build("i", s->len);
}

static Object *__string_tostring(Object *ob, Object *args)
{
  UNUSED_PARAMETER(args);
  OB_ASSERT_KLASS(ob, String_Klass);
  return Tuple_Build("O", ob);
}

// static CMethodStruct string_cmethods[] = {
//   {"Length", "i", "v", ACCESS_PUBLIC, __string_length},
//   {"ToString", "s", "v", ACCESS_PUBLIC, __string_tostring},
//   {NULL, NULL, NULL, 0, NULL}
// };

void Init_String_Klass(void)
{
  //Klass_Add_CMethods(&String_Klass, string_cmethods);
}

/*-------------------------------------------------------------------------*/

static int string_equal(TValue *v1, TValue *v2)
{
  Object *ob1 = VALUE_OBJECT(v1);
  Object *ob2 = VALUE_OBJECT(v2);
  OB_ASSERT_KLASS(ob1, String_Klass);
  OB_ASSERT_KLASS(ob2, String_Klass);
  StringObject *s1 = (StringObject *)ob1;
  StringObject *s2 = (StringObject *)ob2;
  return !strcmp(s1->str, s2->str);
}

static uint32 string_hash(TValue *v)
{
  Object *ob = VALUE_OBJECT(v);
  OB_ASSERT_KLASS(ob, String_Klass);
  StringObject *s = (StringObject *)ob;
  return hash_string(s->str);
}

static void string_free(Object *ob)
{
  OB_ASSERT_KLASS(ob, String_Klass);
  String_Free(ob);
}

Klass String_Klass = {
  OBJECT_HEAD_INIT(&Klass_Klass),
  .name  = "String",
  .bsize = sizeof(StringObject),

  .ob_free = string_free,

  .ob_hash = string_hash,
  .ob_cmp  = string_equal
};
