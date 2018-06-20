
#include "moduleobject.h"
#include "stringobject.h"
#include "tupleobject.h"
#include "tableobject.h"
#include "listobject.h"
#include "koalastate.h"
#include "log.h"

static Object *__lang_typeof(Object *ob, Object *args)
{
	printf("call lang.TypeOf()\n");
	OB_ASSERT_KLASS(ob, Module_Klass);
	OB_ASSERT_KLASS(args, Tuple_Klass);
	int size = Tuple_Size(args);
	if (size == 1) {
		debug("typeof(obj)");
		TValue val = Tuple_Get(args, 0);
		Object *o = val.ob;
		return Tuple_Build("O", OB_KLASS(o));
	} else {
		assert(size == 2);
		debug("typeof(obj, type)");
		return NULL;
	}
}

static Object *__string_concat(Object *ob, Object *args)
{
	OB_ASSERT_KLASS(ob, Module_Klass);
	TValue v1 = Tuple_Get(args, 0);
	TValue v2 = Tuple_Get(args, 1);
	assert(v1.klazz == &String_Klass);
	StringObject *s1 = (StringObject *)v1.ob;
	assert(v2.klazz == &String_Klass);
	StringObject *s2 = (StringObject *)v2.ob;
	char buf[s1->len + s2->len + 1];
	strcpy(buf, s1->str);
	strcat(buf, s2->str);
	Object *res = String_New(buf);
	return Tuple_Build("O", res);
}

static FuncDef lang_funcs[] = {
	{"TypeOf", "Okoala/lang.Class;", "...A", __lang_typeof},
	{"Concat", "s", "ss", __string_concat},
	{NULL}
};

void Init_Lang_Module(void)
{
	Object *m = Koala_New_Module("lang", "koala/lang");
	assert(m);
	Module_Add_CFunctions(m, lang_funcs);

	Module_Add_Class(m, &String_Klass);
	Module_Add_Class(m, &Tuple_Klass);
	Module_Add_Class(m, &Table_Klass);
	Init_String_Klass();
	Init_Tuple_Klass();
	Init_List_Klass();
	Init_Table_Klass();
}
