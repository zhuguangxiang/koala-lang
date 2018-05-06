
#include "tupleobject.h"
#include "moduleobject.h"
#include "log.h"

Object *Tuple_New(int size)
{
	int sz = sizeof(TupleObject) + size * sizeof(TValue);
	TupleObject *tuple = malloc(sz);
	assert(tuple);
	Init_Object_Head(tuple, &Tuple_Klass);
	tuple->size = size;
	for (int i = 0; i < size; i++) {
		initnilvalue(tuple->items + i);
	}
	return (Object *)tuple;
}

void Tuple_Free(Object *ob)
{
	if (!ob) return;
	OB_ASSERT_KLASS(ob, Tuple_Klass);
	free(ob);
}

TValue Tuple_Get(Object *ob, int index)
{
	TupleObject *tuple = OB_TYPE_OF(ob, TupleObject, Tuple_Klass);

	if (index < 0 || index >= tuple->size) {
		error("index %d out of bound", index);
		return NilValue;
	}

	return tuple->items[index];
}

Object *Tuple_Get_Slice(Object *ob, int min, int max)
{
	OB_ASSERT_KLASS(ob, Tuple_Klass);
	if (min > max) return NULL;

	Object *tuple = Tuple_New(max - min + 1);
	int index = min;
	TValue val;
	int i = 0;
	while (index <= max) {
		val = Tuple_Get(ob, index);
		if (VALUE_ISNIL(&val)) return NULL;
		Tuple_Set(tuple, i, &val);
		i++; index++;
	}
	return tuple;
}

int Tuple_Size(Object *ob)
{
	if (!ob) return 0;
	TupleObject *tuple = OB_TYPE_OF(ob, TupleObject, Tuple_Klass);
	return tuple->size;
}

int Tuple_Set(Object *ob, int index, TValue *val)
{
	TupleObject *tuple = OB_TYPE_OF(ob, TupleObject, Tuple_Klass);

	if (index < 0 || index >= tuple->size) {
		error("index %d out of bound", index);
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
		val = Va_Build_Value(ch, &vp);
		Tuple_Set(ob, i++, &val);
	}
	va_end(vp);

	return ob;
}

int Tuple_Parse(Object *ob, char *format, ...)
{
	va_list vp;
	char *fmt = format;
	char ch;
	int i = 0;
	TValue val;

	va_start(vp, format);
	while ((ch = *fmt++)) {
		val = Tuple_Get(ob, i++);
		Va_Parse_Value(&val, ch, &vp);
	}
	va_end(vp);

	return 0;
}

/*-------------------------------------------------------------------------*/

static Object *__tuple_init(Object *ob, Object *args)
{
	UNUSED_PARAMETER(ob);
	UNUSED_PARAMETER(args);
	return NULL;
}

static Object *__tuple_get(Object *ob, Object *args)
{
	TValue val;
	OB_ASSERT_KLASS(ob, Tuple_Klass);
	OB_ASSERT_KLASS(args, Tuple_Klass);
	assert(Tuple_Size(args) == 1);

	val = Tuple_Get(args, 0);
	if (!VALUE_ISINT(&val)) return NULL;

	val = Tuple_Get(ob, VALUE_INT(&val));
	if (VALUE_ISNIL(&val)) return NULL;

	return Tuple_From_TValues(&val, 1);
}

static Object *__tuple_size(Object *ob, Object *args)
{
	assert(!args);
	TupleObject *tuple = OB_TYPE_OF(ob, TupleObject, Tuple_Klass);
	return Tuple_Build("i", tuple->size);
}

static FuncDef tuple_funcs[] = {
	{"__init__", NULL, "...A", __tuple_init},
	{"Get", "A", "i", __tuple_get},
	{"Size", "i", NULL, __tuple_size},
	{NULL}
};

void Init_Tuple_Klass(void)
{
	Klass_Add_CFunctions(&Tuple_Klass, tuple_funcs);
}

/*-------------------------------------------------------------------------*/

static void tuple_free(Object *ob)
{
	Tuple_Free(ob);
}

Klass Tuple_Klass = {
	OBJECT_HEAD_INIT(&Tuple_Klass, &Klass_Klass)
	.name = "Tuple",
	.basesize = sizeof(TupleObject),
	.ob_free = tuple_free,
};
