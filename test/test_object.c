
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

  Klass *dog = Class_New("Dog", animal, NULL);
  Klass_Add_Field(dog, "TailLength", &Int_Type);
  Object *dog_ob = dog->ob_alloc(dog);
  Object_Set_Value(dog_ob, "TailLength", dog, Integer_New(20));
  Object_Set_Value(dog_ob, "super.Age", dog, Integer_New(6));
  Object_Set_Value(dog_ob, "Age", dog, Integer_New(8));
  Object_Set_Value(dog_ob, "Age", animal, Integer_New(5));
  field = Object_Get_Value(dog_ob, "Age", dog);
  assert(field);
  assert(5 == Integer_ToCInt(field));
}

int main(int argc, char *argv[])
{
  AtomString_Init();
  Init_String_Klass();
  test_object();
  AtomString_Fini();
  return 0;
}
