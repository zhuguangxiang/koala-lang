
#include "koala.h"
#include "codeobject.h"

void test_object(void)
{
  /*
    class Animal {
      Name string
      Age int
    }
  */
  Klass *animal = Class_New_Self("Animal");
  TypeDesc *desc = TypeDesc_Get_Base(BASE_STRING);
  Klass_Add_Field(animal, "Name", desc);
  desc = TypeDesc_Get_Base(BASE_INT);
  Klass_Add_Field(animal, "Age", desc);
  Object *ob = animal->ob_alloc(animal);
  Object_Set_Field(ob, NULL, "Name", String_New("wangwang"));
  Object *field = Object_Get_Field(ob, NULL, "Name");
  assert(!strcmp("wangwang", String_Raw(field)));

  CodeObject *code = (CodeObject *)Object_Get_Method(ob, NULL, "__tostring__");
  Object *so = code->cfunc(ob, NULL);
  printf("%s\n", String_Raw(so));
  OB_DECREF(so);

  code = (CodeObject *)Object_Get_Method(ob, NULL, "__hash__");
  Object *into = code->cfunc(ob, NULL);
  printf("%lld\n", Integer_Raw(into));
  OB_DECREF(into);

  code = (CodeObject *)Object_Get_Method(ob, NULL, "__cmp__");
  Object *bo = code->cfunc(ob, ob);
  //so = To_String(bo);
  //printf("%s\n", String_Raw(so));
  //OB_DECREF(so);

  OB_DECREF(ob);

  /*
    class Dog extends Animal {
      TailLength int
    }
   */
  Klass *dog = Class_New("Dog", animal, NULL);
  desc = TypeDesc_Get_Base(BASE_INT);
  Klass_Add_Field(dog, "TailLength", desc);
  Klass_Add_Field(dog, "Age", desc);
  Object *dog_ob = dog->ob_alloc(dog);
  Object *val = Integer_New(20);
  Object_Set_Field(dog_ob, NULL, "TailLength", val);
  OB_DECREF(val);
  val = Integer_New(6);
  Object_Set_Field(dog_ob, NULL, "super.Age", val);
  OB_DECREF(val);
  val = Integer_New(8);
  Object_Set_Field(dog_ob, NULL, "Age", val);
  OB_DECREF(val);
  val = Integer_New(5);
  Object_Set_Field(dog_ob, animal, "Age", val);
  OB_DECREF(val);
  field = Object_Get_Field(dog_ob, NULL, "Age");
  assert(field);
  assert(8 == Integer_Raw(field));
  field = Object_Get_Field(dog_ob, NULL, "TailLength");
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
