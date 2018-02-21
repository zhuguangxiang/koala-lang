
#include "koala.h"

/* gcc -g -std=c99 test_object.c -lkoala -L. */

void test_object(void)
{
	TValue val = NIL_VALUE_INIT();
	val = TValue_Build('i', 200);
	assert(VALUE_ISINT(&val) && VALUE_INT(&val) == 200);

	int ival;
	TValue_Parse(&val, 'i', &ival);
	assert(ival == 200);

	val = TValue_Build('f', 200);
	assert(VALUE_ISFLOAT(&val));

	val = TValue_Build('z', 1);
	assert(VALUE_ISBOOL(&val) && VALUE_BOOL(&val) == 1);

	val = TValue_Build('s', "jimmy");
	assert(VALUE_ISOBJECT(&val));
	OB_ASSERT_KLASS(VALUE_OBJECT(&val), String_Klass);
	char *s = String_RawString(VALUE_OBJECT(&val));
	assert(strcmp(s, "jimmy") == 0);

	TValue_Parse(&val, 's', &s);
	assert(strcmp(s, "jimmy") == 0);

	val = TValue_Build('O', String_New("hello"));
	assert(VALUE_ISOBJECT(&val));
	OB_ASSERT_KLASS(VALUE_OBJECT(&val), String_Klass);
	s = String_RawString(VALUE_OBJECT(&val));
	assert(strcmp(s, "hello") == 0);

	Object *ob;
	TValue_Parse(&val, 'O', &ob);
	assert(ob);
	s = String_RawString(ob);
	assert(strcmp(s, "hello") == 0);

	ob = Koala_Get_Module("koala/lang");
	Klass *klazz = Module_Get_Class(ob, "Tuple");
	KLASS_ASSERT(klazz, Tuple_Klass);
	ob = Klass_Get_Method(klazz, "Get");
	assert(ob);
	OB_ASSERT_KLASS(ob, Code_Klass);
}

int main(int argc, char *argv[]) {
	UNUSED_PARAMETER(argc);
	UNUSED_PARAMETER(argv);

	Koala_Initialize();
	test_object();
	Koala_Finalize();

	return 0;
}
