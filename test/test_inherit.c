
#include "koala.h"

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
  Object *name = Get_Field(ob, NULL, "name");
  Object *msg = args;
  char *out = AtomString_Format("Animal # is #",
                                String_Raw(name), String_Raw(msg));
  printf("%s\n", out);
  return NULL;
}

void new_animal(void)
{
  Klass *animal = Klass_New("Animal", NULL);
  TypeDesc *desc = TypeDesc_Get_Base(BASE_STRING);
  Klass_Add_Field(animal, "name", desc);
  desc = TypeDesc_Get_Base(BASE_INT);
  Klass_Add_Field(animal, "age", desc);
  desc = TypeDesc_Get_Base(BASE_STRING);
  TypeDesc *proto = TypeDesc_Get_SProto(desc, NULL);
  Object *bark = CFunc_New("bark", proto, animal_bark);
  Klass_Add_Method(animal, bark);

  Object *so = To_String((Object *)animal);
  printf("%s\n", String_Raw(so));
  OB_DECREF(so);

  Object *instance = animal->ob_alloc(animal);

  Object *val = String_New("Cat");
  Set_Field(instance, NULL, "name", val);
  OB_DECREF(val);

  CodeObject *code = (CodeObject *)Get_Method(instance, NULL, "bark");
  Object *msg = String_New("miming");
  code->cfunc(instance, msg);
  OB_DECREF(msg);

  OB_DECREF(instance);

  Package *pkg = Koala_Get_Package("skeleton");
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
  Object *name = Get_Field(ob, NULL, "name");
  Object *msg = args;
  char *out = AtomString_Format("Dog # is #",
                                String_Raw(name), String_Raw(msg));
  printf("%s\n", out);
  return NULL;
}

void new_dog(void)
{
  Klass *animal = Koala_Get_Klass("skeleton", "Animal");
  assert(animal);

  VECTOR(bases);
  Vector_Append(&bases, animal);
  Klass *dog = Klass_New("Dog", &bases);
  Vector_Fini_Self(&bases);

  TypeDesc *desc = TypeDesc_Get_Base(BASE_STRING);
  TypeDesc *proto = TypeDesc_Get_SProto(desc, NULL);
  Object *bark = CFunc_New("bark", proto, dog_bark);
  Klass_Add_Method(dog, bark);
  Object *instance = dog->ob_alloc(dog);

  Object *val = String_New("ErHa");
  Set_Field(instance, NULL, "name", val);
  OB_DECREF(val);

  CodeObject *code = (CodeObject *)Get_Method(instance, NULL, "bark");
  Object *msg = String_New("wangwanging");
  code->cfunc(instance, msg);
  OB_DECREF(msg);

  OB_DECREF(instance);

  Package *pkg = Koala_Get_Package("skeleton");
  Pkg_Add_Class(pkg, dog);
  OB_DECREF(dog);
}

void test_inherit(void)
{
  Object *so;
  Package *pkg;

  pkg = Koala_Get_Package("lang");
  so = To_String((Object *)pkg);
  printf("%s\n", String_Raw(so));
  OB_DECREF(so);

  pkg = Koala_Get_Package("io");
  so = To_String((Object *)pkg);
  printf("%s\n", String_Raw(so));
  OB_DECREF(so);

  pkg = Pkg_New("skeleton");
  Koala_Add_Package("skeleton", pkg);

  Show_PkgTree();

  new_animal();
  new_dog();

  so = To_String((Object *)pkg);
  printf("%s\n", String_Raw(so));
  OB_DECREF(so);
}

int main(int argc, char *argv[])
{
  Koala_Initialize();
  test_inherit();
  Koala_Finalize();
  return 0;
}
