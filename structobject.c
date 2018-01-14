
#include "structobject.h"

int StructObject_Get_Value(Object *ob, char *name, TValue *ret)
{
  Symbol *s = Klass_Get(OB_KLASS(ob), name);
  if (s == NULL) -1;
  if (s->kind != SYM_FIELD || s->kind != SYM_IMETHOD) return -1;
  int index = s->value.index;
  StructObject *so = (StructObject *)ob;
  ASSERT(index >= 0 && index < so->size);
  *ret = so->items[index];
  return 0;
}

Object *StructObject_Get_Method(Object *ob, char *name)
{
  return Klass_Get_Method(OB_KLASS(ob), name);
}

/*-------------------------------------------------------------------------*/

static void struct_mark(Object *ob)
{
  StructObject *sob = (StructObject *)ob;
  ASSERT(OB_KLASS(OB_KLASS(ob)) == &Klass_Klass);
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
  ASSERT(OB_KLASS(klazz) == &Klass_Klass);
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
  ASSERT(OB_KLASS(OB_KLASS(ob)) == &Klass_Klass);
  free(ob);
}

static uint32 struct_hash(TValue *v)
{
  ASSERT(VALUE_ISOBJECT(v));
  Object *ob = VALUE_OBJECT(v);
  ASSERT(OB_KLASS(OB_KLASS(ob)) == &Klass_Klass);
  return (uint32)ob;
}

static int struct_compare(TValue *v1, TValue *v2)
{
  ASSERT(VALUE_ISOBJECT(v1) && VALUE_ISOBJECT(v2));
  Object *ob1 = VALUE_OBJECT(v1);
  ASSERT(OB_KLASS(OB_KLASS(ob1)) == &Klass_Klass);
  Object *ob2 = VALUE_OBJECT(v2);
  ASSERT(OB_KLASS(OB_KLASS(ob2)) == &Klass_Klass);
  return ob1 != ob2;
}

static Object *struct_tostring(TValue *v)
{
  ASSERT(VALUE_ISOBJECT(v));
  Object *ob = VALUE_OBJECT(v);
  ASSERT(OB_KLASS(OB_KLASS(ob)) == &Klass_Klass);
  return NULL;
}

Klass *Struct_Klass_New(char *name)
{
  Klass *klazz = Klass_New(name, sizeof(StructObject), sizeof(TValue),
                           &Klass_Klass);
  klazz->ob_mark  = struct_mark;

  klazz->ob_alloc = struct_alloc;
  klazz->ob_free  = struct_free,

  klazz->ob_hash  = struct_hash;
  klazz->ob_cmp   = struct_compare;

  klazz->ob_tostr = struct_tostring;

  return klazz;
}
