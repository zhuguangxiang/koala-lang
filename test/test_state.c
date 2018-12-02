
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "stringobject.h"
#include "state.h"
#include "intobject.h"

void test_state(void)
{
	Package *pkg = Package_New("lang");
	Package_Add_Var(pkg, "Foo", &Int_Type, 0);
	Package_Add_Var(pkg, "Bar", &Int_Type, 0);
	Koala_Add_Package("github.com/koala", pkg);
	Koala_Set_Value(pkg, "Foo", Integer_New(100));
	Object *ob = Koala_Get_Value(pkg, "Foo");
	assert(ob);
	assert(100 == Integer_ToCInt(ob));
	Koala_Set_Value(pkg, "Foo", String_New("hello"));
	ob = Koala_Get_Value(pkg, "Foo");
	assert(ob);
	assert(!strcmp("hello", String_RawString(ob)));
	Koala_Add_Package("github.com/", pkg);
	pkg = Package_New("io");
	Koala_Add_Package("github.com/home/zgx/koala/", pkg);
	Koala_Add_Package("github.com/", pkg);
	Koala_Add_Package("github.com/koala", pkg);
	Koala_Show_Packages();
	ob = Koala_Get_Package("github.com/koala/io");
	assert(ob);
	ob = Koala_Get_Package("github.com/koala/lang");
	assert(ob);
}

int main(int argc, char *argv[])
{
	Koala_Initialize();
	test_state();
	Koala_Finalize();
	Fini_Package_Klass();
	Fini_Integer_Klass();
	Fini_String_Klass();
	AtomString_Fini();
	return 0;
}
