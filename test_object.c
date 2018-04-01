
#include "koala.h"

/* gcc -g -std=c99 test_object.c -lkoala -L. */

void test_tvalue(void)
{
	TValue val = NIL_VALUE_INIT();
	val = TValue_Build('i', 200);
	assert(VALUE_ISINT(&val) && VALUE_INT(&val) == 200);

	val = TValue_Build('f', 200);
	assert(VALUE_ISFLOAT(&val));

	val = TValue_Build('z', 1);
	assert(VALUE_ISBOOL(&val) && VALUE_BOOL(&val) == 1);

	val = TValue_Build('s', "jimmy");
	assert(VALUE_ISOBJECT(&val));
	OB_ASSERT_KLASS(VALUE_OBJECT(&val), String_Klass);
	char *s = String_RawString(VALUE_OBJECT(&val));
	assert(!strcmp(s, "jimmy"));

	val = TValue_Build('O', String_New("hello"));
	assert(VALUE_ISOBJECT(&val));
	OB_ASSERT_KLASS(VALUE_OBJECT(&val), String_Klass);
	s = String_RawString(VALUE_OBJECT(&val));
	assert(!strcmp(s, "hello"));
}

void test_object(void)
{
	STable *stbl = STable_New(NULL);
	Klass *animal = Klass_New("Animal", NULL, 0);
	animal->stbl.atbl = stbl->atbl;
	TypeDesc *desc = TypeDesc_From_Primitive(PRIMITIVE_STRING);
	Klass_Add_Field(animal, "name", desc);
	desc = TypeDesc_From_Primitive(PRIMITIVE_INT);
	Klass_Add_Field(animal, "age", desc);
	Klass *dog = Klass_New("Dog", animal, 0);
	dog->stbl.atbl = stbl->atbl;
	TypeDesc_From_Primitive(PRIMITIVE_STRING);
	Klass_Add_Field(dog, "name", desc);
	desc = TypeDesc_From_Primitive(PRIMITIVE_INT);
	Klass_Add_Field(dog, "age", desc);
	desc = TypeDesc_From_Primitive(PRIMITIVE_STRING);
	Klass_Add_Field(dog, "color", desc);
	Object *ob = dog->ob_alloc(dog);

	assert(OB_KLASS(ob) == dog);
	assert(OB_Head(ob) == ob);
	assert(OB_Base(ob));
	assert(OB_HasBase(ob));

	Object *base = OB_Base(ob);
	assert(OB_KLASS(base) == animal);
	assert(OB_Head(base) == ob);
	assert(OB_Base(base));
	assert(!OB_HasBase(base));

	assert(ob);
}

int main(int argc, char *argv[]) {
	UNUSED_PARAMETER(argc);
	UNUSED_PARAMETER(argv);

	Koala_Initialize();
	test_tvalue();
	test_object();
	Koala_Finalize();

	return 0;
}
