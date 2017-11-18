
#include "tupleobject.h"

Object *Tuple_New(int size)
{
  int sz = sizeof(TupleObject) + size * sizeof(TValue);
  TupleObject *tuple = malloc(sz);
  assert(tuple);
  init_object_head(tuple, &Tuple_Klass);
  tuple->size = size;
  for (int i = 0; i < size; i++) {
    init_nil_tval(tuple->items + i);
  }
  return (Object *)tuple;
}

int Tuple_Get(Object *ob, int index, TValue *val)
{
  assert(OB_KLASS(ob) == &Tuple_Klass);
  TupleObject *tuple = (TupleObject *)ob;

  if (index < 0 || index >= tuple->size) {
    fprintf(stderr, "[ERROR] index %d out of bound\n", index);
    return -1;
  }

  *val = tuple->items[index];
  return 0;
}

static int get_args(TupleObject *tuple, int min, int max, va_list ap)
{
  int index = min;
  while (index <= max) {
    TValue **val = va_arg(ap, TValue **);
    *val = (tuple->items + index);
    ++index;
  }
  return 0;
}

int Tuple_Get_Range(Object *ob, int min, int max, ...)
{
  int ret;
  va_list vp;
  va_start(vp, max);
  ret = get_args(ob, min, max, vp);
  va_end(vp);
  return ret;
}

int Tuple_Size(Object *ob)
{
  assert(OB_KLASS(ob) == &Tuple_Klass);
  TupleObject *tuple = (TupleObject *)ob;
  return tuple->size;
}

int Tuple_Set(Object *ob, int index, TValue *val)
{
  assert(OB_KLASS(ob) == &Tuple_Klass);
  TupleObject *tuple = (TupleObject *)ob;

  if (index < 0 || index >= tuple->size) {
    fprintf(stderr, "[ERROR] index %d out of bound\n", index);
    return -1;
  }

  tuple->items[index] = *val;

  return 0;
}

Object *Tuple_Pack(TValue *v)
{
  Object *tuple = Tuple_New(1);
  Tuple_Set(tuple, 0, v);
  return tuple;
}

static int set_args(TupleObject *tuple, int min, int max, va_list ap)
{
  int index = min;
  while (index <= max) {
    TValue *val = va_arg(ap, TValue *);
    tuple->items[index] = *val;
    ++index;
  }
  return 0;
}

Object *Tuple_Pack_Range(int count, ...)
{
  Object *tuple = Tuple_New(count);
  va_list vp;
  va_start(vp, count);
  set_args(tuple, 0, count - 1, vp);
  va_end(vp);
  return tuple;
}

/*-------------------------------------------------------------------------*/

static Object *tuple_get(Object *ob, Object *args)
{
  TValue *val;
  assert(OB_KLASS(ob) == &Tuple_Klass);
  assert(OB_KLASS(args) == &Tuple_Klass);

  if (Tupe_Get(args, 0, &val)) return NULL;
  assert(tval_isint(val));

  if (Tuple_Get(ob, TVAL_IVAL(val), &val)) return NULL;

  return Tuple_Pack(val);
}

static Object *tuple_size(Object *ob, Object *args)
{
  TupleObject *tuple = (TupleObject *)ob;
  TValue val = {TYPE_INT, tuple->size};
  return Tuple_Pack(&val);
}

static MethodStruct tuple_methods[] = {
  {"Get", "(I)(Okoala/lang.Any)", ACCESS_PUBLIC, tuple_get},
  {"Size", "(V)(I)", ACCESS_PUBLIC, tuple_size},
  {NULL, NULL, 0, NULL}
};

void Init_Tuple_Klass(void)
{
  Klass_Add_Methods(&Tuple_Klass, tuple_methods);
}

/*-------------------------------------------------------------------------*/

static void tuple_free(Object *ob)
{
  assert(OB_KLASS(ob) == &Tuple_Klass);
  free(ob);
}

Klass Tuple_Klass = {
  OBJECT_HEAD_INIT(&Klass_Klass),
  .name = "Tuple",
  .bsize = sizeof(TupleObject),
  .isize = sizeof(TValue),

  .ob_free  = tuple_free,
};
