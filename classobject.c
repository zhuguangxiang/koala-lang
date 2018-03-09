
#include "classobject.h"
#include "log.h"

TValue Object_Get_Value(Object *ob, char *name)
{
	OB_ASSERT_KLASS(OB_KLASS(ob), Klass_Klass);
	Symbol *sym = Klass_Get_FieldSymbol(OB_KLASS(ob), name);
	if (!sym) return NilValue;
	if (sym->kind != SYM_VAR && sym->kind != SYM_IPROTO)
		return NilValue;
	int index = sym->index;
	KlassObject *kob = (KlassObject *)ob;
	assert(index >= 0 && index < kob->size);
	return kob->items[index];
}

int Object_Set_Value(Object *ob, char *name, TValue *val)
{
	OB_ASSERT_KLASS(OB_KLASS(ob), Klass_Klass);
	Symbol *sym = Klass_Get_FieldSymbol(OB_KLASS(ob), name);
	if (!sym) {
		error("cannot find field : %s", name);
		return -1;
	}
	if (sym->kind != SYM_VAR && sym->kind != SYM_PROTO) {
		error("field '%s' is not a var and imethod", name);
		return -1;
	}

	int index = sym->index;
	KlassObject *kob = (KlassObject *)ob;
	assert(index >= 0 && index < kob->size);
	kob->items[index] = *val;
	return 0;
}

Object *Object_Get_Method(Object *ob, char *name)
{
	return Klass_Get_Method(OB_KLASS(ob), name);
}

/*-------------------------------------------------------------------------*/

static void object_mark(Object *ob)
{
	OB_ASSERT_KLASS(OB_KLASS(ob), Klass_Klass);
	KlassObject *kob = (KlassObject *)ob;

	//FIXME: inc_ref

	for (int i = 0; i < kob->size; i++) {
		if (VALUE_ISOBJECT(kob->items + i)) {
			Object *tmp = VALUE_OBJECT(kob->items + i);
			OB_KLASS(tmp)->ob_mark(tmp);
		}
	}
}

static Object *object_alloc(Klass *klazz, int nr)
{
	OB_ASSERT_KLASS(klazz, Klass_Klass);
	int size = klazz->size + sizeof(TValue) * nr;
	KlassObject *ob = calloc(1, size);
	init_object_head(ob, klazz);
	ob->size = nr;
	for (int i = 0; i < nr; i++) {
		initnilvalue(ob->items + i);
	}
	return (Object *)ob;
}

static void object_free(Object *ob)
{
	OB_ASSERT_KLASS(OB_KLASS(ob), Klass_Klass);
	free(ob);
}

static uint32 object_hash(TValue *v)
{
	Object *ob = VALUE_OBJECT(v);
	OB_ASSERT_KLASS(OB_KLASS(ob), Klass_Klass);
	return (uint32)ob;
}

static int object_equal(TValue *v1, TValue *v2)
{
	Object *ob1 = VALUE_OBJECT(v1);
	OB_ASSERT_KLASS(OB_KLASS(ob1), Klass_Klass);
	Object *ob2 = VALUE_OBJECT(v2);
	OB_ASSERT_KLASS(OB_KLASS(ob2), Klass_Klass);
	return ob1 == ob2;
}

static Object *object_tostring(TValue *v)
{
	Object *ob = VALUE_OBJECT(v);
	OB_ASSERT_KLASS(OB_KLASS(ob), Klass_Klass);
	//FIXME
	return NULL;
}

static Klass *klass_new(char *name, int size, Klass *super)
{
	Klass *klazz = calloc(1, sizeof(Klass));
	init_object_head(klazz, &Klass_Klass);
	klazz->super = super;
	klazz->name = name;
	klazz->size = size;
	return klazz;
}

Klass *Class_New(char *name, Klass *super)
{
	Klass *klazz = klass_new(name, sizeof(KlassObject), super);
	klazz->ob_mark = object_mark;

	klazz->ob_alloc = object_alloc;
	klazz->ob_free = object_free,

	klazz->ob_hash = object_hash;
	klazz->ob_eq = object_equal;

	klazz->ob_tostr = object_tostring;

	return klazz;
}
