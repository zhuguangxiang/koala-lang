
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "object.h"
#include "stringobject.h"
#include "intobject.h"

void test_object(void)
{
  /*
    class Animal {
      Name string
      Age int
    }
  */
  Klass *animal = Klass_New("Animal", NULL);
  TypeDesc *desc = TypeDesc_Get_Base(BASE_STRING);
  Klass_Add_Field(animal, "Name", desc);
  desc = TypeDesc_Get_Base(BASE_INT);
  Klass_Add_Field(animal, "Age", desc);
  Object *ob = animal->ob_alloc(animal);
  Set_Field(ob, NULL, "Name", String_New("wangwang"));
  Object *field = Get_Field(ob, NULL, "Name");
  assert(!strcmp("wangwang", String_RawString(field)));
  OB_DECREF(ob);

  /*
    class Dog extends Animal {
      TailLength int
    }
   */
  VECTOR(bases);
  Vector_Append(&bases, animal);
  Klass *dog = Klass_New("Dog", &bases);
  Vector_Fini_Self(&bases);
  desc = TypeDesc_Get_Base(BASE_INT);
  Klass_Add_Field(dog, "TailLength", desc);
  Klass_Add_Field(dog, "Age", desc);
  Object *dog_ob = dog->ob_alloc(dog);
  Object *val = Integer_New(20);
  Set_Field(dog_ob, NULL, "TailLength", val);
  OB_DECREF(val);
  val = Integer_New(6);
  Set_Field(dog_ob, NULL, "super.Age", val);
  OB_DECREF(val);
  val = Integer_New(8);
  Set_Field(dog_ob, NULL, "Age", val);
  OB_DECREF(val);
  val = Integer_New(5);
  Set_Field(dog_ob, animal, "Age", val);
  OB_DECREF(val);
  field = Get_Field(dog_ob, NULL, "Age");
  assert(field);
  assert(8 == Integer_ToCInt(field));
  field = Get_Field(dog_ob, NULL, "TailLength");
  assert(field);
  assert(20 == Integer_ToCInt(field));
  OB_DECREF(dog_ob);
  Klass_Free(animal);
  Klass_Free(dog);
}

int main(int argc, char *argv[])
{
  AtomString_Init();
  Init_TypeDesc();
  Init_String_Klass();
  Init_Integer_Klass();
  test_object();
  Fini_String_Klass();
  Fini_Integer_Klass();
  Fini_TypeDesc();
  AtomString_Fini();
  return 0;
}
