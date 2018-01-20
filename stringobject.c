
#include "stringobject.h"
#include "tupleobject.h"
#include "moduleobject.h"
#include "symbol.h"
#include "hash.h"

Object *String_New(char *str)
{
  StringObject *sobj = malloc(sizeof(*sobj));
  init_object_head(sobj, &String_Klass);
  sobj->len = strlen(str);
  sobj->str = str;
  //Object_Add_GCList(s);
  return (Object *)sobj;
}

void String_Free(Object *ob)
{
  OB_ASSERT_KLASS(ob, String_Klass);
  free(ob);
}

char *String_RawString(Object *ob)
{
  OB_ASSERT_KLASS(ob, String_Klass);
  StringObject *sobj = (StringObject *)ob;
  return sobj->str;
}

static int isneg(char **s)
{
  if (**s == '-') {
    (*s)++;
    return 1;
  } else if (**s == '+') {
    (*s)++;
  }
  return 0;
}

static int hexavalue(int c)
{
  if (isdigit(c)) return c - '0';
  else return (tolower(c) - 'a') + 10;
}

TValue String_ToInteger(Object *ob)
{
  OB_ASSERT_KLASS(ob, String_Klass);
  StringObject *sobj = (StringObject *)ob;
  char *s = sobj->str;
  while (isspace(*s)) s++;  /* skip prefix spaces */
  int neg = isneg(&s);
  uint64 a = 0;
  if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {  /* hex? */
    s += 2;  /* skip '0x' */
    while (isxdigit(*s)) {
      a = a * 16 + hexavalue(*s);
      s++;
    }
  } else {  /* decimal */
    while (isdigit(*s)) {
      int d = *s - '0';
      a = a * 10 + d;
      s++;
    }
  }
  while (isspace(*s)) s++;  /* skip trailing spaces */
  if (*s != '\0') return NilValue;
  else { TValue val; setivalue(&val, neg ? 0ul - a : a); return val;}
}

/*-------------------------------------------------------------------------*/

static Object *__string_length(Object *ob, Object *args)
{
  ASSERT(args == NULL);
  OB_ASSERT_KLASS(ob, String_Klass);
  StringObject *s = (StringObject *)ob;
  return Tuple_Build("i", s->len);
}

static Object *__string_tostring(Object *ob, Object *args)
{
  ASSERT(args == NULL);
  OB_ASSERT_KLASS(ob, String_Klass);
  return Tuple_Build("O", ob);
}

static FunctionStruct string_functions[] = {
  {"Length", "i", "v", ACCESS_PUBLIC, __string_length},
  {"ToString", "s", "v", ACCESS_PUBLIC, __string_tostring},
  {NULL}
};

void Init_String_Klass(Object *ob)
{
  ModuleObject *mo = (ModuleObject *)ob;
  String_Klass.itable = mo->itable;
  Klass_Add_CFunctions(&String_Klass, string_functions);
  Module_Add_Class(ob, &String_Klass, ACCESS_PUBLIC);
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
