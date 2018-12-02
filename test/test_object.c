
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "object.h"
#include "stringobject.h"
#include "intobject.h"

void test_object(void)
{
	Klass *animal = Class_New("Animal", NULL, NULL);
	Klass_Add_Field(animal, "Name", &String_Type);
	Klass_Add_Field(animal, "Age", &Int_Type);
	Object *ob = animal->ob_alloc(animal);
	Object_Set_Value(ob, "Name", animal, String_New("wangwang"));
	Object *field = Object_Get_Value(ob, "Name", animal);
	assert(!strcmp("wangwang", String_RawString(field)));
	OB_DECREF(ob);

	Klass *dog = Class_New("Dog", animal, NULL);
	Klass_Add_Field(dog, "TailLength", &Int_Type);
	Object *dog_ob = dog->ob_alloc(dog);
	Object *old = Object_Set_Value(dog_ob, "TailLength", dog, Integer_New(20));
	assert(!old);
	old = Object_Set_Value(dog_ob, "super.Age", dog, Integer_New(6));
	assert(!old);
	old = Object_Set_Value(dog_ob, "Age", dog, Integer_New(8));
	OB_DECREF(old);
	old = Object_Set_Value(dog_ob, "Age", animal, Integer_New(5));
	OB_DECREF(old);
	field = Object_Get_Value(dog_ob, "Age", dog);
	assert(field);
	assert(5 == Integer_ToCInt(field));
	OB_DECREF(field);
	field = Object_Get_Value(dog_ob, "TailLength", dog);
	OB_DECREF(field);
	OB_DECREF(dog_ob);
	Klass_Free(animal);
	Klass_Free(dog);
}

int main(int argc, char *argv[])
{
	AtomString_Init();
	Init_String_Klass();
	Init_Integer_Klass();
	test_object();
	Fini_String_Klass();
	Fini_Integer_Klass();
	AtomString_Fini();
	return 0;
}
