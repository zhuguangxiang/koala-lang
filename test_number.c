
#include "numberobject.h"
#include "stringobject.h"

void test_number(void)
{
	NumberFunctions *ops = Int_Klass.numops;

	TValue v;

	TValue i1, i2;
	setivalue(&i1, 100); setivalue(&i2, 200);
	v = ops->add(&i1, &i2);
	assert(v.klazz == &Int_Klass);
	assert(v.ival == 300);

	setivalue(&i1, -100); setivalue(&i2, -200);
	v = ops->add(&i1, &i2);
	assert(v.klazz == &Int_Klass);
	assert(v.ival == -300);

	TValue f2;
	setivalue(&i1, 100); setfltvalue(&f2, 200.123);
	v = ops->add(&i1, &f2);
	assert(v.klazz == &Int_Klass);
	assert(v.ival == 300);

	TValue s2;
	Object *strobj = String_New("200");
	setivalue(&i1, 100); setobjvalue(&s2, strobj);
	v = ops->add(&i1, &s2);
	assert(v.klazz == &Int_Klass);
	assert(v.ival == 300);

	strobj = String_New("-200");
	setivalue(&i1, 100); setobjvalue(&s2, strobj);
	v = ops->add(&i1, &s2);
	assert(v.klazz == &Int_Klass);
	assert(v.ival == -100);

	strobj = String_New("-0xc8");
	setivalue(&i1, 100); setobjvalue(&s2, strobj);
	v = ops->add(&i1, &s2);
	assert(v.klazz == &Int_Klass);
	assert(v.ival == -100);

	strobj = String_New("200.123");
	setivalue(&i1, 100); setobjvalue(&s2, strobj);
	v = ops->add(&i1, &s2);
	assert(v.klazz == &Int_Klass);
	assert(v.ival == 300);

	strobj = String_New("-200.123");
	setivalue(&i1, 100); setobjvalue(&s2, strobj);
	v = ops->add(&i1, &s2);
	assert(v.klazz == &Int_Klass);
	assert(v.ival == -100);
}

int main(int argc, char *argv[])
{
	UNUSED_PARAMETER(argc);
	UNUSED_PARAMETER(argv);
	test_number();
	return 0;
}
