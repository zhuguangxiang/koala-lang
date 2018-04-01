
#include "moduleobject.h"
#include "stringobject.h"
#include "tupleobject.h"
#include "tableobject.h"
#include "koala_state.h"
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
		Object *o = VALUE_OBJECT(&val);
		return Tuple_Build("O", OB_KLASS(o));
	} else {
		assert(size == 2);
		debug("typeof(obj, type)");
		return NULL;
	}
}

static FuncDef lang_funcs[] = {
	{"TypeOf", 1, "Okoala/lang.Class;", 1, "...A", __lang_typeof},
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
	Init_Table_Klass();
}
