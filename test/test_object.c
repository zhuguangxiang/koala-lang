
#include "koala.h"

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
  assert(!strcmp("wangwang", String_Raw(field)));
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
  assert(8 == Integer_Raw(field));
  field = Get_Field(dog_ob, NULL, "TailLength");
  assert(field);
  assert(20 == Integer_Raw(field));
  OB_DECREF(dog_ob);
  OB_DECREF(animal);
  OB_DECREF(dog);
}

int main(int argc, char *argv[])
{
  Koala_Initialize();
  test_object();
  Koala_Finalize();
  return 0;
}
