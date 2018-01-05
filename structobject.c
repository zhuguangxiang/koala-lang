
#include "structobject.h"

static void struct_mark(Object *ob)
{
  StructObject *sob = (StructObject *)ob;
  assert(OB_KLASS(OB_KLASS(ob)) == &Klass_Klass);
  //ob_incref(ob);
  for (int i = 0; i < sob->size; i++) {
    if (VALUE_ISOBJECT(sob->items + i)) {
      Object *temp = VALUE_OBJECT(sob->items + i);
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
    init_nil_value(ob->items + i);
  }
  return (Object *)ob;
}

static void struct_free(Object *ob)
{
  assert(OB_KLASS(OB_KLASS(ob)) == &Klass_Klass);
  free(ob);
}

static uint32 struct_hash(TValue *v)
{
  assert(VALUE_ISOBJECT(v));
  Object *ob = VALUE_OBJECT(v);
  assert(OB_KLASS(OB_KLASS(ob)) == &Klass_Klass);
  return (uint32)ob;
}

static int struct_compare(TValue *v1, TValue *v2)
{
  assert(VALUE_ISOBJECT(v1) && VALUE_ISOBJECT(v2));
  Object *ob1 = VALUE_OBJECT(v1);
  assert(OB_KLASS(OB_KLASS(ob1)) == &Klass_Klass);
  Object *ob2 = VALUE_OBJECT(v2);
  assert(OB_KLASS(OB_KLASS(ob2)) == &Klass_Klass);
  return ob1 != ob2;
}

static Object *struct_tostring(TValue *v)
{
  assert(VALUE_ISOBJECT(v));
  Object *ob = VALUE_OBJECT(v);
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
