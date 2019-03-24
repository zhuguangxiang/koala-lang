
#include "koala.h"
#include "codeobject.h"
#include "stringbuf.h"

/*
  class Animal {
    name string
    age int
    func bark(msg string) {
      println("Animal ${Name} is ${msg}");
    }
  }
*/

Object *animal_bark(Object *ob, Object *args)
{
  Object *name = Object_Get_Field(ob, NULL, "name");
  Object *msg = args;
  char *out = AtomString_Format("Animal # is #",
                                String_Raw(name), String_Raw(msg));
  printf("%s\n", out);
  return NULL;
}

void new_animal(void)
{
  Klass *animal = Class_New_Self("Animal");
  TypeDesc *desc = TypeDesc_Get_Base(BASE_STRING);
  Klass_Add_Field(animal, "name", desc);
  desc = TypeDesc_Get_Base(BASE_INT);
  Klass_Add_Field(animal, "age", desc);
  desc = TypeDesc_Get_Base(BASE_STRING);
  TypeDesc *proto = TypeDesc_Get_SProto(desc, NULL);
  Object *bark = CFunc_New("bark", proto, animal_bark);
  Klass_Add_Method(animal, bark);

  //Object *so = To_String((Object *)animal);
  //printf("%s\n", String_Raw(so));
  //OB_DECREF(so);

  Object *instance = animal->ob_alloc(animal);

  Object *val = String_New("Cat");
  Object_Set_Field(instance, NULL, "name", val);
  OB_DECREF(val);

  CodeObject *code = (CodeObject *)Object_Get_Method(instance, NULL, "bark");
  Object *msg = String_New("miming");
  code->cfunc(instance, msg);
  OB_DECREF(msg);

  OB_DECREF(instance);

  Object *pkg = Find_Package("skeleton");
  Pkg_Add_Class(pkg, animal);
  OB_DECREF(animal);
}

/*
  class Dog extends Animal {
    func bark(msg string) {
      println("Dog ${Name} is ${msg}");
    }
  }
*/

Object *dog_bark(Object *ob, Object *args)
{
  Object *name = Object_Get_Field(ob, NULL, "name");
  Object *msg = args;
  char *out = AtomString_Format("Dog # is #",
                                String_Raw(name), String_Raw(msg));
  printf("%s\n", out);
  return NULL;
}

void new_dog(void)
{
  Object *pkg = Find_Package("skeleton");

  Klass *animal = Pkg_Get_Class(pkg, "Animal");
  assert(animal);

  Klass *dog = Class_New("Dog", animal, NULL);
  TypeDesc *desc = TypeDesc_Get_Base(BASE_STRING);
  TypeDesc *proto = TypeDesc_Get_SProto(desc, NULL);
  Object *bark = CFunc_New("bark", proto, dog_bark);
  Klass_Add_Method(dog, bark);
  Object *instance = dog->ob_alloc(dog);

  Object *val = String_New("ErHa");
  Object_Set_Field(instance, NULL, "name", val);
  OB_DECREF(val);

  CodeObject *code = (CodeObject *)Object_Get_Method(instance, NULL, "bark");
  Object *msg = String_New("wangwanging");
  code->cfunc(instance, msg);
  OB_DECREF(msg);

  OB_DECREF(instance);

  Pkg_Add_Class(pkg, dog);
  OB_DECREF(dog);
}

void test_inherit(void)
{
  Object *so;
  Object *pkg;

  pkg = Find_Package("lang");
  //so = To_String((Object *)pkg);
  //printf("%s\n", String_Raw(so));
  //OB_DECREF(so);

  pkg = Find_Package("io");
  //so = To_String((Object *)pkg);
  //printf("%s\n", String_Raw(so));
  //OB_DECREF(so);

  pkg = New_Package("skeleton");
  Install_Package("skeleton", pkg);

  new_animal();
  new_dog();

  //so = To_String((Object *)pkg);
  //printf("%s\n", String_Raw(so));
  //OB_DECREF(so);
}

int main(int argc, char *argv[])
{
  Koala_Initialize();
  test_inherit();
  Koala_Finalize();
  return 0;
}
