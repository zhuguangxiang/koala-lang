
#include "packageobject.h"
#include "stringobject.h"

void test_package(void)
{
  PackageObject *pkg = Package_New("lang");
  Package_Add_Var(pkg, "Foo", &Int_Type, 0);
  Package_Add_Var(pkg, "Bar", &Int_Type, 0);
  assert(pkg->varcnt == 2);
  Object *ob = OB_KLASS(pkg)->ob_str((Object *)pkg);
  printf("%s\n", String_RawString(ob));
  OB_DECREF(ob);
  Package_Free(pkg);
  Klass *klazz = &String_Klass;
  ob = OB_KLASS(klazz)->ob_str((Object *)klazz);
  printf("%s\n", String_RawString(ob));
  OB_DECREF(ob);
}

int main(int argc, char *argv[])
{
  AtomString_Init();
  Init_String_Klass();
  Init_Package_Klass();
  test_package();
  Fini_Package_Klass();
  Fini_String_Klass();
  AtomString_Fini();
  return 0;
}
