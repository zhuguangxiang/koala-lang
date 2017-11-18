
#include "object.h"
#include "tableobject.h"

Klass *Klass_New(const char *name, int bsize, int isize)
{
  Klass *klass = malloc(sizeof(*klass));
  memset(klass, 0, sizeof(*klass));
  init_object_head(klass, &Klass_Klass);
  klass->name = name;
  klass->bsize = bsize;
  klass->isize = isize;
  return klass;
}

static int klass_add_member(Klass *klass, MemberStruct *m)
{
  if (klass->table == NULL) {
    klass->table = Table_New();
    assert(klass->table);
    klass->nr_vars = 0;
    klass->nr_meths = 0;
  }

  TValue name = {
    TYPE_NAME,
    Name_New(m->name, NT_VAR, m->signature, m->access)
  };

  TValue member = {
    TYPE_FUNC,
    m->offset
  };

  int res = Table_Put(klass->table, &name, &member);
  if (!res) {
    ++klass->nr_vars;
  }

  return res;
}

int Klass_Add_Members(Klass *klass, MemberStruct *members)
{
  MemberStruct *m = members;
  while (m->name != NULL) {
    klass_add_method(klass, m);
    ++m;
  }
  return 0;
}

static int klass_add_method(Klass *klass, MethodStruct *m)
{
  if (klass->table == NULL) {
    klass->table = Table_New();
    assert(klass->table);
    klass->nr_vars = 0;
    klass->nr_meths = 0;
  }

  TValue name = {
    TYPE_NAME,
    Name_New(m->name, NT_FUNC, m->signature, m->access)
  };

  TValue meth = {
    TYPE_FUNC,
    m->func
  };

  int res = Table_Put(klass->table, &name, &meth);
  if (!res) {
    ++klass->nr_meths;
  }

  return res;
}

int Klass_Add_Methods(Klass *klass, MethodStruct *meths)
{
  MethodStruct *meth = meths;
  while (meth->name != NULL) {
    klass_add_method(klass, meth);
    ++meth;
  }
  return 0;
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
    NULL
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

int Ineger_Compare(TValue *tv1, TValue *tv2)
{
  return TVAL_IVAL(tv1) - TVAL_IVAL(tv2);
}

Name *Name_New(char *name, uint8 type, char *signature, uint8 access)
{
  Name *n = malloc(sizeof(*n));
  n->name = name;
  n->type = type;
  n->signature = signature;
  n->access = access;
  return n;
}

void Name_Free(Name *name)
{
  free(name);
}

int Name_Compare(TValue *tv1, TValue *tv2)
{
  Name *n1 = tv1->name;
  Name *n2 = tv2->name;
  return !strcmp(n1->name, n2->name);
}

uint32 Name_Hash(TValue *tv)
{
  Name *n = tv->name;
  return hash_string(n->name);
}
