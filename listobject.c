
#include "listobject.h"
#include "tupleobject.h"
#include "stringobject.h"
#include "log.h"

Object *List_New(Klass *klazz)
{
  int sz = sizeof(ListObject);
  ListObject *list = malloc(sz);
  assert(list);
  Init_Object_Head(list, &List_Klass);
  list->size = 0;
  list->type = klazz;
  list->capacity = LIST_DEFAULT_SIZE;
  list->items = malloc(sizeof(TValue) * LIST_DEFAULT_SIZE);
  for (int i = 0; i < LIST_DEFAULT_SIZE; i++) {
    initnilvalue(list->items + i);
  }
  return (Object *)list;
}

void List_Free(Object *ob)
{
  if (!ob) return;
  ListObject *list = OB_TYPE_OF(ob, ListObject, List_Klass);
  free(list->items);
  free(list);
}

int List_Set(Object *ob, int index, TValue *val)
{
  ListObject *list = OB_TYPE_OF(ob, ListObject, List_Klass);

  //FIXME:
#if 0
  if (index < 0 || index >= list->size) {
    error("index %d out of bound", index);
    return -1;
  }
#endif

  list->size++;

  list->items[index] = *val;
  return 0;
}

TValue List_Get(Object *ob, int index)
{
  ListObject *list = OB_TYPE_OF(ob, ListObject, List_Klass);

  if (index < 0 || index >= list->size) {
    error("index %d out of bound", index);
    return NilValue;
  }

  return list->items[index];
}

TValue List_Remove(Object *ob, int index)
{
  ListObject *list = OB_TYPE_OF(ob, ListObject, List_Klass);

  if (index < 0 || index >= list->size) {
    error("index %d out of bound", index);
    return NilValue;
  }

  return list->items[index];
}

/*---------------------------------------------------------------------------*/

static Object *__list_set(Object *ob, Object *args)
{
  TValue index;
  TValue value;
  OB_ASSERT_KLASS(ob, List_Klass);
  OB_ASSERT_KLASS(args, Tuple_Klass);
  assert(Tuple_Size(args) == 2);

  index = Tuple_Get(args, 0);
  if (!VALUE_ISINT(&index)) return NULL;

  value = Tuple_Get(args, 1);
  if (VALUE_ISNIL(&value)) return NULL;

  List_Set(ob, VALUE_INT(&index), &value);

  return Tuple_From_TValues(&value, 1);
}

static Object *__list_get(Object *ob, Object *args)
{
  TValue val;
  OB_ASSERT_KLASS(ob, List_Klass);
  OB_ASSERT_KLASS(args, Tuple_Klass);
  assert(Tuple_Size(args) == 1);

  val = Tuple_Get(args, 0);
  if (!VALUE_ISINT(&val)) return NULL;

  val = List_Get(ob, VALUE_INT(&val));
  if (VALUE_ISNIL(&val)) return NULL;

  return Tuple_From_TValues(&val, 1);
}

static Object *__list_remove(Object *ob, Object *args)
{
  OB_ASSERT_KLASS(ob, List_Klass);
  OB_ASSERT_KLASS(args, Tuple_Klass);
  assert(Tuple_Size(args) == 1);

  TValue val = Tuple_Get(args, 0);
  if (!VALUE_ISNIL(&val)) return NULL;

  val = List_Remove(ob, VALUE_INT(&val));
  if (VALUE_ISNIL(&val)) return NULL;

  return Tuple_From_TValues(&val, 1);
}

Object *__list_size(Object *ob, Object *args)
{
  assert(!args);
  ListObject *list = OB_TYPE_OF(ob, ListObject, List_Klass);
  return Tuple_Build("i", list->size);
}

static FuncDef list_funcs[] = {
  {"Set", "A", "iA", __list_set},
  {"Get", "i", "A", __list_get},
  {"Remove", "A", "i", __list_remove},
  {"Size", NULL, NULL, __list_size},
  {NULL}
};

void Init_List_Klass(void)
{
  Klass_Add_CFunctions(&List_Klass, list_funcs);
}

/*---------------------------------------------------------------------------*/

static Object *list_tostring(TValue *val)
{
  //FIXME
  char buf[128];
  ListObject *lo = val->ob;
  int count = 0;
  TValue *v;
  count = snprintf(buf, 127, "[");
  for (int i = 0; i < lo->size; i++) {
    v = lo->items + i;
    count += TValue_Print(buf + count, 127 - count, v, 0);
    if (i < lo->size - 1)
      count += snprintf(buf + count, 127 - count, ", ");
  }
  snprintf(buf + count, 127 - count, "]");
  return Tuple_Build("O", String_New(buf));
}

static void list_free(Object *ob)
{
  List_Free(ob);
}

void list_setitem(TValue *o, TValue *k, TValue *v)
{
  List_Set(o->ob, k->ival, v);
}

TValue list_getitem(TValue *o, TValue *k)
{
  return List_Get(o->ob, k->ival);
}

MapOperations list_map_ops = {
  .get = list_getitem,
  .set = list_setitem
};

Klass List_Klass = {
  OBJECT_HEAD_INIT(&List_Klass, &Klass_Klass)
  .name = "List",
  .basesize = sizeof(ListObject),
  .mapops = &list_map_ops,
  .ob_free = list_free,
  .ob_tostr = list_tostring,
};
