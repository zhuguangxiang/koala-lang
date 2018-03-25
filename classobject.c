
#include "classobject.h"
#include "tupleobject.h"
#include "stringobject.h"
#include "log.h"

static Symbol *get_field_symbol(Object *ob, char *name, Object **rob)
{
	Check_Klass(OB_KLASS(ob));
	ClassObject *instance = (ClassObject *)ob;
	Object *super;
	Symbol *sym = NULL;
	char *dot = strchr(name, '.');
	if (dot) {
		char *classname = strndup(name, dot - name);
		char *fieldname = strndup(dot + 1, strlen(name) - (dot - name) - 1);
		super = instance->super;
		assert(super && !strcmp(OB_KLASS(super)->name, classname));
		sym = Klass_Get_FieldSymbol(OB_KLASS(super), fieldname);
		*rob = super;
		free(classname);
		free(fieldname);
	} else {
		super = ob;
		while (super) {
			sym = Klass_Get_FieldSymbol(OB_KLASS(super), name);
			if (sym) {
				*rob = super;
				break;
			}
			super = ((ClassObject *)super)->super;
		}
	}

	assert(sym && sym->kind == SYM_VAR);
	return sym;
}

TValue Object_Get_Value(Object *ob, char *name)
{
	Object *rob = NULL;
	Symbol *sym = get_field_symbol(ob, name, &rob);
	assert(rob);
	ClassValue *value = (ClassValue *)((ClassObject *)rob + 1);
	int index = sym->index;
	assert(index >= 0 && index < value->size);
	return value->items[index];
}

int Object_Set_Value(Object *ob, char *name, TValue *val)
{
	Object *rob = NULL;
	Symbol *sym = get_field_symbol(ob, name, &rob);
	assert(rob);
	ClassValue *value = (ClassValue *)((ClassObject *)rob + 1);
	int index = sym->index;
	assert(index >= 0 && index < value->size);
	value->items[index] = *val;
	return 0;
}

Object *Object_Get_Method(Object *ob, char *name, Object **rob)
{
	Check_Klass(OB_KLASS(ob));
	ClassObject *instance = (ClassObject *)ob;
	Object *super;
	Object *code;
	char *dot = strchr(name, '.');
	if (dot) {
		char *classname = strndup(name, dot - name);
		char *funcname = strndup(dot + 1, strlen(name) - (dot - name) - 1);
		super = instance->super;
		assert(super);
		assert(!strcmp(OB_KLASS(super)->name, classname));
		code = Klass_Get_Method(OB_KLASS(super), funcname);
		assert(code);
		*rob = super;
		free(classname);
		free(funcname);
		return code;
	} else {
		if (!strcmp(name, "__init__")) {
			//__init__ method is allowed to be searched only in current class
			code = Klass_Get_Method(OB_KLASS(ob), name);
			//FIXME: class with no __init__ ?
			//assert(code);
			*rob = ob;
			return code;
		} else {
			super = ob;
			while (super) {
				code = Klass_Get_Method(OB_KLASS(super), name);
				if (code) {
					*rob = super;
					return code;
				}
				super = ((ClassObject *)super)->super;
			}
			assertm(0, "cannot find func '%s'", name);
			return NULL;
		}
	}
}

/*-------------------------------------------------------------------------*/

static void object_mark(Object *ob)
{
	Check_Klass(OB_KLASS(ob));
	ClassValue *value = (ClassValue *)((ClassObject *)ob + 1);

	//FIXME: inc_ref

	for (int i = 0; i < value->size; i++) {
		if (VALUE_ISOBJECT(value->items + i)) {
			Object *tmp = VALUE_OBJECT(value->items + i);
			OB_KLASS(tmp)->ob_mark(tmp);
		}
	}
}

static Object *object_alloc(Klass *klazz)
{
	Check_Klass(klazz);
	int nrfields = klazz->stbl.varcnt;
	debug("number of object's fields:%d", nrfields);
	int size = klazz->size + sizeof(ClassValue) + sizeof(TValue) * nrfields;
	ClassObject *ob = calloc(1, size);
	init_object_head(ob, klazz);
	ClassValue *value = (ClassValue *)(ob + 1);
	value->size = nrfields;
	for (int i = 0; i < nrfields; i++) {
		initnilvalue(value->items + i);
	}

	//allocate super memory
	Klass *super = OB_KLASS(klazz);
	if (super && !Klass_IsRoot(super)) {
		debug("call super '%s' alloc", super->name);
		ob->super = super->ob_alloc(super);
	}

	return (Object *)ob;
}

static void object_free(Object *ob)
{
	Check_Klass(OB_KLASS(ob));
	free(ob);
}

static uint32 object_hash(TValue *v)
{
	Object *ob = VALUE_OBJECT(v);
	Check_Klass(OB_KLASS(ob));
	return (uint32)ob;
}

static int object_equal(TValue *v1, TValue *v2)
{
	Object *ob1 = VALUE_OBJECT(v1);
	Check_Klass(OB_KLASS(ob1));
	Object *ob2 = VALUE_OBJECT(v2);
	Check_Klass(OB_KLASS(ob2));
	return ob1 == ob2;
}

static Object *object_tostring(TValue *v)
{
	Object *ob = VALUE_OBJECT(v);
	Klass *klazz = OB_KLASS(ob);
	Check_Klass(klazz);
	//FIXME:
	char buf[64];
	sprintf(buf, "%p", ob);
	return Tuple_Build("O", String_New(buf));
}

static Klass *klass_new(char *name, Klass *super)
{
	Klass *klazz = calloc(1, sizeof(Klass));
	init_object_head(klazz, super);
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
