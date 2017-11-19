
#include "stringobject.h"
#include "tupleobject.h"
#include "hash.h"
#include "kstate.h"

Object *String_New(char *str)
{
  StringObject *s = malloc(sizeof(*s));
  init_object_head(s, &String_Klass);
  s->len = strlen(str);
  s->str = str;
  Object_Add_GCList((Object *)s);
  return (Object *)s;
}

void String_Free(Object *ob)
{
  assert(OB_KLASS(ob) == &String_Klass);
  free(ob);
}

char *String_To_CString(Object *ob)
{
  assert(OB_KLASS(ob) == &String_Klass);
  StringObject *s = (StringObject *)ob;
  return s->str;
}

/*-------------------------------------------------------------------------*/

static Object *string_length(Object *ob, Object *args)
{
  UNUSED_PARAMETER(args);
  assert(OB_KLASS(ob) == &String_Klass);
  StringObject *s = (StringObject *)ob;
  return Tuple_Build("i", s->len);
}

static Object *string_tostring(Object *ob, Object *args)
{
  UNUSED_PARAMETER(args);
  assert(OB_KLASS(ob) == &String_Klass);
  return Tuple_Build("O", ob);
}

static MethodStruct string_methods[] = {
  {"Length", "(V)(I)", ACCESS_PUBLIC, string_length},
  {"ToString", "(V)(Okoala/lang.String)", ACCESS_PUBLIC, string_tostring},
  {NULL, NULL, 0, NULL}
};

void Init_String_Klass(void)
{
  Klass_Add_Methods(&String_Klass, string_methods);
}

/*-------------------------------------------------------------------------*/

static int string_equal(TValue v1, TValue v2)
{
  Object *ob1 = TVAL_OBJECT(v1);
  Object *ob2 = TVAL_OBJECT(v2);
  assert(OB_KLASS(ob1) == &String_Klass);
  assert(OB_KLASS(ob2) == &String_Klass);
  StringObject *s1 = (StringObject *)ob1;
  StringObject *s2 = (StringObject *)ob2;
  return !strcmp(s1->str, s2->str);
}

static uint32 string_hash(TValue v)
{
  Object *ob = TVAL_OBJECT(v);
  assert(OB_KLASS(ob) == &String_Klass);
  StringObject *s = (StringObject *)ob;
  return hash_string(s->str);
}

static void string_free(Object *ob)
{
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
