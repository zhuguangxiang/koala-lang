
#include "packageobject.h"
#include "stringobject.h"

void test_package(void)
{
  PackageObject *pkg = Package_New("lang");
  TypeDesc *desc = TypeDesc_Get_Base(BASE_INT);
  Package_Add_Var(pkg, "Foo", desc, 0);
  Package_Add_Var(pkg, "Bar", desc, 0);
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
  Init_TypeDesc();
  Init_String_Klass();
  Init_Package_Klass();
  test_package();
  Fini_Package_Klass();
  Fini_String_Klass();
  Fini_TypeDesc();
  AtomString_Fini();
  return 0;
}
