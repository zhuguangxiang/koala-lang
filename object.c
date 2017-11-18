
#include "object.h"
#include "tableobject.h"
#include "methodobject.h"

Klass *Klass_New(const char *name, int bsize, int isize)
{
  Klass *klazz = malloc(sizeof(*klazz));
  memset(klazz, 0, sizeof(*klazz));
  init_object_head(klazz, &Klass_Klass);
  klazz->name = name;
  klazz->bsize = bsize;
  klazz->isize = isize;
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

  TValue name = {
    .type = TYPE_NAME,
    .name = Name_New(m->name, NT_VAR, m->signature, m->access)
  };

  TValue member = {
    .type = TYPE_INT,
    .ival = m->offset
  };

  int res = Table_Put(klazz->table, &name, &member);
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

  TValue name = {
    .type = TYPE_NAME,
    .name = Name_New(m->name, NT_FUNC, m->signature, m->access)
  };

  TValue meth = {
    .type = TYPE_OBJECT,
    .ob   = CMethod_New(m->func)
  };

  int res = Table_Put(klazz->table, &name, &meth);
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

TValue *Klass_Get(Klass *klazz, char *name)
{
  if (klazz->table == NULL) return NULL;
  Name n = {name};
  TValue key = {.type = TYPE_NAME, .name = &n};
  return Table_Get(klazz->table, &key);
}

/*-------------------------------------------------------------------------*/

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
  return strcmp(n1->name, n2->name);
}

uint32 Name_Hash(TValue *tv)
{
  Name *n = tv->name;
  return hash_string(n->name);
}
