
#include "object.h"
#include "nameobject.h"
#include "tupleobject.h"
#include "tableobject.h"
#include "methodobject.h"
#include "stringobject.h"
#include "kstate.h"

Klass *Klass_New(const char *name, int bsize, int isize)
{
  Klass *klazz = malloc(sizeof(*klazz));
  memset(klazz, 0, sizeof(*klazz));
  init_object_head(klazz, &Klass_Klass);
  klazz->name = name;
  klazz->bsize = bsize;
  klazz->isize = isize;
  Object_Add_GCList((Object *)klazz);
  return klazz;
}

static int klass_add_member(Klass *klazz, MemberStruct *m)
{
  if (klazz->table == NULL) {
    klazz->table = Table_New();
    assert(klazz->table);
    klazz->nr_vars = 0;
    klazz->nr_meths = 0;
  }

  Object *o = Name_New(m->name, NT_VAR, m->signature, m->access);
  TValue name = TValue_Build('O', o);
  TValue member = TValue_Build('i', m->offset);

  int res = Table_Put(klazz->table, name, member);
  if (!res) {
    ++klazz->nr_vars;
  }

  return res;
}

int Klass_Add_Members(Klass *klazz, MemberStruct *members)
{
  MemberStruct *m = members;
  while (m->name != NULL) {
    klass_add_member(klazz, m);
    ++m;
  }
  return 0;
}

static int klass_add_method(Klass *klazz, MethodStruct *m)
{
  if (klazz->table == NULL) {
    klazz->table = Table_New();
    assert(klazz->table);
    klazz->nr_vars = 0;
    klazz->nr_meths = 0;
  }

  Object *o = Name_New(m->name, NT_VAR, m->signature, m->access);
  TValue name = TValue_Build('O', o);
  TValue meth = TValue_Build('O', CMethod_New(m->func));

  int res = Table_Put(klazz->table, name, meth);
  if (!res) {
    ++klazz->nr_meths;
  }

  return res;
}

int Klass_Add_Methods(Klass *klazz, MethodStruct *meths)
{
  MethodStruct *meth = meths;
  while (meth->name != NULL) {
    klass_add_method(klazz, meth);
    ++meth;
  }
  return 0;
}

TValue Klass_Get(Klass *klazz, char *name)
{
  if (klazz->table == NULL) return TVAL_NIL;
  Object *n = Name_New(name, 0, NULL, 0);
  return Table_Get(klazz->table, TValue_Build('O', n));
}

/*-------------------------------------------------------------------------*/

static Object *klass_get_method(Object *klazz, Object *args)
{
  assert(OB_KLASS(klazz) == &Klass_Klass);
  char *str = NULL;
  Tuple_Parse(args, "s", &str);
  return Tuple_From_TValues(1, Klass_Get((Klass *)klazz, str));
}

static MethodStruct klass_methods[] = {
  {
    "GetField",
    "(Okoala/lang.String;)(Okoala/reflect.Field;)",
    ACCESS_PUBLIC,
    NULL
  },
  {
    "GetMethod",
    "(Okoala/lang.String;)(Okoala/reflect.Method;)",
    ACCESS_PUBLIC,
    klass_get_method
  },
  {
    "NewInstance",
    "(V)(V)",
    ACCESS_PUBLIC,
    NULL
  },
  {NULL, NULL, 0, NULL}
};

void Init_Klass_Klass(void)
{
  Klass_Add_Methods(&Klass_Klass, klass_methods);
}

/*-------------------------------------------------------------------------*/

TValue Va_Build_Value(char ch, va_list *ap)
{
  TValue val = NIL_TVAL_INIT;

  switch (ch) {
    case 'i': {
      uint32 i = va_arg(*ap, uint32);
      val.type = TYPE_INT;
      val.ival = (int64)i;
      break;
    }

    case 'l': {
      uint64 i = va_arg(*ap, uint64);
      val.type = TYPE_INT;
      val.ival = (int64)i;
      break;
    }

    case 'f':
    case 'd': {
      float64 f = va_arg(*ap, float64);
      val.type = TYPE_FLOAT;
      val.fval = f;
      break;
    }

    case 'b': {
      uint32 i = va_arg(*ap, uint32);
      val.type = TYPE_BYTE;
      val.ival = (int64)i;
      break;
    }

    case 'z': {
      int i = va_arg(*ap, int);
      val.type = TYPE_BOOL;
      val.bval = i;
      break;
    }

    case 's': {
      char *str = va_arg(*ap, char *);
      val.type = TYPE_OBJECT;
      val.ob = String_New(str);
      break;
    }

    case 'O': {
      Object *o = va_arg(*ap, Object *);
      val.type = TYPE_OBJECT;
      val.ob = o;
      break;
    }

    default: {
      fprintf(stderr, "[ERROR] unsupported type: %d\n", ch);
      assert(0);
      break;
    }
  }

  return val;
}

TValue TValue_Build(char ch, ...)
{
  TValue val;
  va_list vp;
  va_start(vp, ch);
  val = Va_Build_Value(ch, &vp);
  va_end(vp);
  return val;
}

int Va_Parse_Value(TValue val, char ch, va_list *ap)
{
  switch (ch) {
    case 'i': {
      int32 *i = va_arg(*ap, int32 *);
      *i = (int32)TVAL_INT(val);
      assert(TVAL_TYPE(val) == TYPE_INT);
      break;
    }

    case 'l': {
      int64 *i = va_arg(*ap, int64 *);
      *i = TVAL_INT(val);
      assert(TVAL_TYPE(val) == TYPE_INT);
      break;
    }

    case 'f':
    case 'd': {
      float64 *f = va_arg(*ap, float64 *);
      *f = TVAL_FLOAT(val);
      assert(TVAL_TYPE(val) == TYPE_FLOAT);
      break;
    }

    case 'b': {
      uint8 *ch = va_arg(*ap, uint8 *);
      *ch = TVAL_BYTE(val);
      assert(TVAL_TYPE(val) == TYPE_BYTE);
      break;
    }

    case 'z': {
      int *i = va_arg(*ap, int *);
      *i = TVAL_BOOL(val);
      assert(TVAL_TYPE(val) == TYPE_BOOL);
      break;
    }

    case 's': {
      char **str = va_arg(*ap, char **);
      assert(TVAL_TYPE(val) == TYPE_OBJECT);
      assert(OB_KLASS(TVAL_OBJECT(val)) == &String_Klass);
      *str = String_To_CString(TVAL_OBJECT(val));
      break;
    }

    case 'O': {
      Object **o = va_arg(*ap, Object **);
      *o = TVAL_OBJECT(val);
      assert(TVAL_TYPE(val) == TYPE_OBJECT);
      break;
    }

    default: {
      fprintf(stderr, "[ERROR] unsupported type: %d\n", ch);
      assert(0);
      break;
    }
  }

  return 0;
}

int TValue_Parse(TValue val, char ch, ...)
{
  int res;
  va_list vp;
  va_start(vp, ch);
  res = Va_Parse_Value(val, ch, &vp);
  va_end(vp);
  return res;
}

/*-------------------------------------------------------------------------*/

static void klass_mark(Object *ob)
{
  assert(OB_KLASS(ob) == &Klass_Klass);
  ob_incref(ob);
  //FIXME
}

static void klass_free(Object *ob)
{
  // Klass_Klass cannot be freed.
  assert(ob != (Object *)&Klass_Klass);
  assert(OB_KLASS(ob) == &Klass_Klass);
  free(ob);
}

Klass Klass_Klass = {
  OBJECT_HEAD_INIT(&Klass_Klass),
  .name  = "Klass",
  .bsize = sizeof(Klass),

  .ob_mark = klass_mark,

  .ob_free = klass_free,
};

/*-------------------------------------------------------------------------*/

uint32 Integer_Hash(TValue v)
{
  return hash_uint32((uint32)TVAL_INT(v), 0);
}

int Ineger_Compare(TValue v1, TValue v2)
{
  return TVAL_INT(v1) - TVAL_INT(v2);
}
