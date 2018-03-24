
#include "classobject.h"
#include "log.h"

TValue Object_Get_Value(Object *ob, Klass *klazz, char *name)
{
	OB_ASSERT_KLASS(OB_KLASS(ob), Klass_Klass);
	Symbol *sym;

	char *dot = strchr(name, '.');
	if (dot) {
		char *classname = strndup(name, dot - name);
		char *funcname = strndup(dot + 1, strlen(name) - (dot - name) - 1);
		while (klazz) {
			if (!strcmp(klazz->name, classname))
				break;
			klazz = klazz->super;
		}
		sym = Klass_Get_FieldSymbol(klazz, funcname);
		free(classname);
		free(funcname);
	} else {
		while (klazz) {
			sym = Klass_Get_FieldSymbol(klazz, name);
			if (sym) break;
			klazz = klazz->super;
		}
	}

	if (!sym) {
		error("cannot find field : %s", name);
		return NilValue;
	}
	if (sym->kind != SYM_VAR && sym->kind != SYM_IPROTO) {
		error("field '%s' is not a var and imethod", name);
		return NilValue;
	}

	int index = sym->index;
	ClassObject *cob = (ClassObject *)ob;
	assert(index >= 0 && index < cob->size);
	return cob->items[index];
}

int Object_Set_Value(Object *ob, Klass *klazz, char *name, TValue *val)
{
	OB_ASSERT_KLASS(OB_KLASS(ob), Klass_Klass);
	Symbol *sym;

	char *dot = strchr(name, '.');
	if (dot) {
		char *classname = strndup(name, dot - name);
		char *funcname = strndup(dot + 1, strlen(name) - (dot - name) - 1);
		while (klazz) {
			if (!strcmp(klazz->name, classname))
				break;
			klazz = klazz->super;
		}
		sym = Klass_Get_FieldSymbol(klazz, funcname);
		free(classname);
		free(funcname);
	} else {
		while (klazz) {
			sym = Klass_Get_FieldSymbol(klazz, name);
			if (sym) break;
			klazz = klazz->super;
		}
	}

	if (!sym) {
		error("cannot find field : %s", name);
		return -1;
	}
	if (sym->kind != SYM_VAR && sym->kind != SYM_PROTO) {
		error("field '%s' is not a var and imethod", name);
		return -1;
	}

	int index = sym->index;
	ClassObject *cob = (ClassObject *)ob;
	assert(index >= 0 && index < cob->size);
	cob->items[index] = *val;
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
	ClassObject *cob = (ClassObject *)ob;

	//FIXME: inc_ref

	for (int i = 0; i < cob->size; i++) {
		if (VALUE_ISOBJECT(cob->items + i)) {
			Object *tmp = VALUE_OBJECT(cob->items + i);
			OB_KLASS(tmp)->ob_mark(tmp);
		}
	}
}

static Object *object_alloc(Klass *klazz)
{
	OB_ASSERT_KLASS(klazz, Klass_Klass);
	int nrfields = klazz->stbl.varcnt;
	Klass *super = klazz->super;
	while (super) {
		nrfields += super->stbl.varcnt;
		super = super->super;
	}

	debug("number of object's fields:%d", nrfields);

	int size = klazz->size + sizeof(TValue) * nrfields;
	ClassObject *ob = calloc(1, size);
	init_object_head(ob, klazz);
	ob->size = nrfields;
	for (int i = 0; i < nrfields; i++) {
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

static Klass *klass_new(char *name, Klass *super)
{
	Klass *klazz = calloc(1, sizeof(Klass));
	init_object_head(klazz, &Klass_Klass);
	klazz->super = super;
	klazz->name = name;
	klazz->size = sizeof(ClassObject);
	return klazz;
}

Klass *Class_New(char *name, Klass *super)
{
	Klass *klazz = klass_new(name, super);
	klazz->ob_mark = object_mark;

	klazz->ob_alloc = object_alloc;
	klazz->ob_free = object_free,

	klazz->ob_hash = object_hash;
	klazz->ob_eq = object_equal;

	klazz->ob_tostr = object_tostring;

	return klazz;
}
