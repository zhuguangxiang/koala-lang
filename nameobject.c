
#include "hash.h"
#include "nameobject.h"
#include "kstate.h"

Object *Name_New(char *name, uint8 type, char *signature, uint8 access)
{
  NameObject *n = malloc(sizeof(*n));
  init_object_head(n, &Name_Klass);
  n->name = name;
  n->type = type;
  n->signature = signature;
  n->access = access;
  Object_Add_GCList((Object *)n);
  return (Object *)n;
}

void Name_Free(Object *ob)
{
  assert(OB_KLASS(ob) == &Name_Klass);
  free(ob);
}

/*-------------------------------------------------------------------------*/

static MethodStruct name_methods[] = {
  {NULL, NULL, 0, NULL}
};

void Init_Name_Klass(void)
{
  Klass_Add_Methods(&Name_Klass, name_methods);
}

/*-------------------------------------------------------------------------*/

static int name_equal(TValue v1, TValue v2)
{
  Object *ob1 = TVAL_OBJECT(v1);
  Object *ob2 = TVAL_OBJECT(v2);
  assert((OB_KLASS(ob1) == &Name_Klass) && (OB_KLASS(ob2) == &Name_Klass));
  NameObject *n1 = (NameObject *)ob1;
  NameObject *n2 = (NameObject *)ob2;
  return !strcmp(n1->name, n2->name);
}

static uint32 name_hash(TValue v)
{
  Object *ob = TVAL_OBJECT(v);
  assert(OB_KLASS(ob) == &Name_Klass);
  NameObject *n = (NameObject *)ob;
  return hash_string(n->name);
}

static void name_free(Object *ob)
{
  Name_Free(ob);
}

Klass Name_Klass = {
  OBJECT_HEAD_INIT(&Klass_Klass),
  .name  = "Name",
  .bsize = sizeof(NameObject),

  .ob_free = name_free,

  .ob_hash = name_hash,
  .ob_cmp  = name_equal
};
