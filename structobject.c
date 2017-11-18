
#include "structobject.h"

static void struct_mark(Object *ob)
{
  StructObject *sob = (StructObject *)ob;
  assert(OB_KLASS(OB_KLASS(ob)) == &Klass_Klass);
  ob_incref(ob);
  for (int i = 0; i < sob->size; i++) {
    if (tval_isobject(sob->items + i)) {
      Object *temp = TVAL_OVAL(sob->items + i);
      OB_KLASS(temp)->ob_mark(temp);
    }
  }
}

static Object *struct_alloc(Klass *klass, int num)
{
  assert(OB_KLASS(klass) == &Klass_Klass);
  size_t size = klass->bsize + klass->isize * num;
  StructObject *ob = malloc(size);
  init_object_head(ob, klass);
  ob->size = num;
  for (int i = 0; i < num; i++) {
    init_nil_tval(ob->items + i);
  }
  return (Object *)ob;
}

static void struct_free(Object *ob)
{
  assert(OB_KLASS(OB_KLASS(ob)) == &Klass_Klass);
  free(ob);
}

static uint32_t struct_hash(TValue *tv)
{
  assert(tval_isobject(tv));
  Object *ob = TVAL_OVAL(tv);
  assert(OB_KLASS(OB_KLASS(ob)) == &Struct_Klass);
  return (uint32_t)ob;
}

static int struct_compare(TValue *tv1, TValue *tv2)
{
  assert(tval_isobject(tv1) && tval_isobject(tv2));
  Object *ob1 = TVAL_OVAL(tv1);
  assert(OB_KLASS(OB_KLASS(ob1)) == &Struct_Klass);
  Object *ob2 = TVAL_OVAL(tv2);
  assert(OB_KLASS(OB_KLASS(ob2)) == &Struct_Klass);
  return ob1 != ob2;
}

static Object *struct_tostring(TValue *tv)
{
  assert(tval_isobject(tv));
  Object *ob = TVAL_OVAL(tv);
  assert(OB_KLASS(OB_KLASS(ob)) == &Struct_Klass);
  return NULL;
}

Klass *Struct_Klass_New(const char *name)
{
  Klass *klass = Klass_New(name, sizeof(StructObject), sizeof(TValue));

  klass->ob_mark = struct_mark;

  klass->ob_alloc = struct_alloc;

  klass->ob_hash = struct_hash;
  klass->ob_cmp = struct_compare;

  klass->ob_tostr = struct_tostring;

  return klass;
}
