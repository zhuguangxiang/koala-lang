
#include "tupleobject.h"
#include "symbol.h"
#include "debug.h"

Object *Tuple_New(int size)
{
  int sz = sizeof(TupleObject) + size * sizeof(TValue);
  TupleObject *tuple = malloc(sz);
  assert(tuple);
  init_object_head(tuple, &Tuple_Klass);
  tuple->size = size;
  for (int i = 0; i < size; i++) {
    init_nil_value(tuple->items + i);
  }
  //Object_Add_GCList(tuple);
  return (Object *)tuple;
}

int Tuple_Get(Object *ob, int index, TValue *ret)
{
  OB_ASSERT_KLASS(ob, Tuple_Klass);
  TupleObject *tuple = (TupleObject *)ob;

  if (index < 0 || index >= tuple->size) {
    debug_error("index %d out of bound\n", index);
    return -1;
  }

  *ret = tuple->items[index];
  return 0;
}

Object *Tuple_Get_Slice(Object *ob, int min, int max)
{
  OB_ASSERT_KLASS(ob, Tuple_Klass);
  if (min > max) return NULL;

  Object *tuple = Tuple_New(max - min + 1);
  int index = min;
  TValue val;
  if (Tuple_Get(ob, index, &val) < 0) return NULL;

  int i = 0;
  while (index <= max) {
    Tuple_Set(tuple, i, &val);
    ++i; index++;
  }
  return tuple;
}

int Tuple_Size(Object *ob)
{
  if (ob == NULL) return 0;
  OB_ASSERT_KLASS(ob, Tuple_Klass);
  TupleObject *tuple = (TupleObject *)ob;
  return tuple->size;
}

int Tuple_Set(Object *ob, int index, TValue *val)
{
  OB_ASSERT_KLASS(ob, Tuple_Klass);
  TupleObject *tuple = (TupleObject *)ob;

  if (index < 0 || index >= tuple->size) {
    debug_error("index %d out of bound\n", index);
    return -1;
  }

  tuple->items[index] = *val;
  return 0;
}

static int get_args(Object *tuple, int min, int max, va_list *ap)
{
  int index = min;
  while (index <= max) {
    TValue *val = va_arg(*ap, TValue *);
    Tuple_Set(tuple, index, val);
    ++index;
  }
  return 0;
}

Object *Tuple_From_Va_TValues(int count, ...)
{
  Object *tuple = Tuple_New(count);
  va_list vp;
  va_start(vp, count);
  get_args(tuple, 0, count - 1, &vp);
  va_end(vp);
  return tuple;
}

Object *Tuple_From_TValues(TValue *arr, int size)
{
  if (size <= 0) return NULL;
  Object *tuple = Tuple_New(size);
  for (int i = 0; i < size; i++) {
    Tuple_Set(tuple, i, arr + i);
  }
  return tuple;
}

static int count_format(char *format)
{
  char ch;
  char *fmt = format;
  while ((ch = *fmt)) { ++fmt; }
  return fmt - format;
}

Object *Tuple_Build(char *format, ...)
{
  va_list vp;
  char *fmt = format;
  char ch;
  int i = 0;
  int n = count_format(format);
  if (n <= 0) return NULL;
  TValue val;
  Object *ob = Tuple_New(n);

  va_start(vp, format);
  while ((ch = *fmt++)) {
    Va_Build_Value(&val, ch, &vp);
    Tuple_Set(ob, i++, &val);
  }
  va_end(vp);

  return ob;
}

int Tuple_Parse(Object *ob, char *format, ...)
{
  OB_ASSERT_KLASS(ob, Tuple_Klass);

  va_list vp;
  char *fmt = format;
  char ch;
  int i = 0;
  int res;
  TValue val;

  va_start(vp, format);
  while ((ch = *fmt++)) {
    res = Tuple_Get(ob, i++, &val);
    assert(res >= 0);
    Va_Parse_Value(&val, ch, &vp);
  }
  va_end(vp);

  return 0;
}

/*-------------------------------------------------------------------------*/
/*
static Object *__tuple_get(Object *ob, Object *args)
{
  TValue val;
  OB_ASSERT_KLASS(ob, Tuple_Klass);
  OB_ASSERT_KLASS(args, Tuple_Klass);

  val = Tuple_Get(args, 0);
  if (!VALUE_ISINT(&val)) return NULL;

  val = Tuple_Get(ob, VALUE_INT(&val));
  if (VALUE_ISNIL(&val)) return NULL;

  return Tuple_From_Va_TValues(1, &val);
}

static Object *__tuple_size(Object *ob, Object *args)
{
  OB_ASSERT_KLASS(ob, Tuple_Klass);
  assert(args == NULL);
  TupleObject *tuple = (TupleObject *)ob;
  return Tuple_Build("i", tuple->size);
}
*/

// static CMethodStruct tuple_cmethods[] = {
//   {"Get", "a", "i", ACCESS_PUBLIC, __tuple_get},
//   {"Size", "i", "v", ACCESS_PUBLIC, __tuple_size},
//   {NULL, NULL, NULL, 0, NULL}
// };

void Init_Tuple_Klass(void)
{
  //Klass_Add_CMethods(&Tuple_Klass, tuple_cmethods);
}

/*-------------------------------------------------------------------------*/

static void tuple_free(Object *ob)
{
  OB_ASSERT_KLASS(ob, Tuple_Klass);
  free(ob);
}

Klass Tuple_Klass = {
  OBJECT_HEAD_INIT(&Klass_Klass),
  .name = "Tuple",
  .bsize = sizeof(TupleObject),
  .isize = sizeof(TValue),

  .ob_free  = tuple_free,
};
