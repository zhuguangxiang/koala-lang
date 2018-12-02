
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "intobject.h"
#include "stringobject.h"

void test_intobject(void)
{
	Object *ob = Integer_New(100);
	Object *s = OB_KLASS(ob)->ob_str(ob);
	assert(!strcmp("100", String_RawString(s)));
	OB_DECREF(ob);
	OB_DECREF(s);
}

int main(int argc, char *argv[])
{
	AtomString_Init();
	Init_String_Klass();
	Init_Integer_Klass();
	test_intobject();
	Fini_Integer_Klass();
	Fini_String_Klass();
	AtomString_Fini();
	return 0;
}
