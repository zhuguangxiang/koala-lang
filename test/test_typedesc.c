
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "typedesc.h"
#include "log.h"

void test_typedesc(void)
{
	assert(TypeDesc_Equal(&Int_Type, &Int_Type));
	assert(!TypeDesc_Equal(&Int_Type, &String_Type));
	char buf[256];
	TypeDesc_ToString(&Int_Type, buf);
	assert(!strcmp("int", buf));
	TypeDesc_Free(&Int_Type);

	TypeDesc *type;
	TypeDesc *type2;

	type = TypeDesc_New_Klass("koala/lang", "Tuple");
	TypeDesc_ToString(type, buf);
	assert(!strcmp("koala/lang.Tuple", buf));

	type2 = TypeDesc_Dup(type);
	TypeDesc_ToString(type, buf);
	assert(!strcmp("koala/lang.Tuple", buf));
	assert(TypeDesc_Equal(type, type2));
	TypeDesc_Free(type);
	TypeDesc_Free(type2);

	type = TypeDesc_New_Array(1, &Bool_Type);
	TypeDesc_ToString(type, buf);
	assert(!strcmp("[bool]", buf));

	type2 = TypeDesc_New_Array(2, type);
	TypeDesc_ToString(type2, buf);
	assert(!strcmp("[[bool]]", buf));

	type = TypeDesc_Dup(type2);
	assert(TypeDesc_Equal(type, type2));
	TypeDesc_Free(type);
	TypeDesc_Free(type2);

	Vector *arg = Vector_New();
	Vector_Append(arg, TypeDesc_New_Array(1, &Int_Type));
	Vector_Append(arg, &String_Type);

	Vector *ret = Vector_New();
	Vector_Append(ret, &Int_Type);
	Vector_Append(ret, &String_Type);

	type = TypeDesc_New_Proto(arg, ret);
	TypeDesc_ToString(type, buf);
	assert(!strcmp("func([int], string) (int, string)", buf));

	type2 = TypeDesc_Dup(type);
	assert(TypeDesc_Equal(type, type2));
	TypeDesc_Free(type);
	TypeDesc_Free(type2);

	Vector *vec = String_To_TypeDescList("i[sOkoala/lang.Tuple;z");
	assert(Vector_Size(vec) == 4);

	type = Vector_Get(vec, 0);
	assert(TypeDesc_Equal(type, &Int_Type));

	type = Vector_Get(vec, 1);
	TypeDesc_ToString(type, buf);
	assert(!strcmp("[string]", buf));

	type = Vector_Get(vec, 2);
	TypeDesc_ToString(type, buf);
	assert(!strcmp("koala/lang.Tuple", buf));

	type = Vector_Get(vec, 3);
	assert(TypeDesc_Equal(type, &Bool_Type));
}

int main(int argc, char *argv[])
{
	AtomString_Init();
	test_typedesc();
	AtomString_Fini();
	return 0;
}
