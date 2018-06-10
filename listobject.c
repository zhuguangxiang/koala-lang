
#include "listobject.h"
#include "tupleobject.h"
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

	if (index < 0 || index >= list->size) {
		error("index %d out of bound", index);
		return -1;
	}

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

static Object *__list_size(Object *ob, Object *args)
{
	assert(!args);
	ListObject *list = OB_TYPE_OF(ob, ListObject, List_Klass);
	return Tuple_Build("i", list->size);
}

static FuncDef list_funcs[] = {
	{"Set", "iA", "A", __list_set},
	{"Get", "A", "i", __list_get},
	{"Remove", "A", "i", __list_get},
	{"Size", "i", NULL, __list_size},
	{NULL}
};

void Init_List_Klass(void)
{
	Klass_Add_CFunctions(&List_Klass, list_funcs);
}

/*---------------------------------------------------------------------------*/

static void list_free(Object *ob)
{
	List_Free(ob);
}

Klass List_Klass = {
	OBJECT_HEAD_INIT(&List_Klass, &Klass_Klass)
	.name = "List",
	.basesize = sizeof(ListObject),
	.ob_free = list_free,
};
