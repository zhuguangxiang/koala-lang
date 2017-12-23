
#include "structobject.h"

static void struct_mark(Object *ob)
{
  StructObject *sob = (StructObject *)ob;
  assert(OB_KLASS(OB_KLASS(ob)) == &Klass_Klass);
  ob_incref(ob);
  for (int i = 0; i < sob->size; i++) {
    if (TVAL_ISOBJ(sob->items[i])) {
      Object *temp = OBJECT_TVAL(sob->items[i]);
      OB_KLASS(temp)->ob_mark(temp);
    }
  }
}

static Object *struct_alloc(Klass *klazz, int num)
{
  assert(OB_KLASS(klazz) == &Klass_Klass);
  size_t size = klazz->bsize + klazz->isize * num;
  StructObject *ob = malloc(size);
  init_object_head(ob, klazz);
  ob->size = num;
  for (int i = 0; i < num; i++) {
    init_nil_tval(ob->items[i]);
  }
  return (Object *)ob;
}

static void struct_free(Object *ob)
{
  assert(OB_KLASS(OB_KLASS(ob)) == &Klass_Klass);
  free(ob);
}

static uint32 struct_hash(TValue v)
{
  assert(TVAL_ISOBJ(v));
  Object *ob = OBJECT_TVAL(v);
  assert(OB_KLASS(OB_KLASS(ob)) == &Klass_Klass);
  return (uint32)ob;
}

static int struct_compare(TValue v1, TValue v2)
{
  assert(TVAL_ISOBJ(v1) && TVAL_ISOBJ(v2));
  Object *ob1 = OBJECT_TVAL(v1);
  assert(OB_KLASS(OB_KLASS(ob1)) == &Klass_Klass);
  Object *ob2 = OBJECT_TVAL(v2);
  assert(OB_KLASS(OB_KLASS(ob2)) == &Klass_Klass);
  return ob1 != ob2;
}

static Object *struct_tostring(TValue v)
{
  assert(TVAL_ISOBJ(v));
  Object *ob = OBJECT_TVAL(v);
  assert(OB_KLASS(OB_KLASS(ob)) == &Klass_Klass);
  return NULL;
}

Klass *Struct_Klass_New(char *name)
{
  Klass *klazz = Klass_New(name, sizeof(StructObject), sizeof(TValue));

  klazz->ob_mark  = struct_mark;

  klazz->ob_alloc = struct_alloc;
  klazz->ob_free  = struct_free,

  klazz->ob_hash  = struct_hash;
  klazz->ob_cmp   = struct_compare;

  klazz->ob_tostr = struct_tostring;

  return klazz;
}
