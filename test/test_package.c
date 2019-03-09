
#include "koala.h"

void test_package(void)
{
  Package *pkg = Pkg_New("lang");
  TypeDesc *desc = TypeDesc_Get_Base(BASE_INT);
  Pkg_Add_Var(pkg, "Foo", desc);
  Pkg_Add_Var(pkg, "Bar", desc);
  desc = TypeDesc_Get_Base(BASE_STRING);
  Object *so = String_New("/opt/");
  Pkg_Add_Const(pkg, "FILENAME", desc, so);
  OB_DECREF(so);
  assert(pkg->nrvars == 2);
  Object *ob = Koala_Get_Value(pkg, "FILENAME");
  assert(ob);
  assert(!strcmp("/opt/", String_RawString(ob)));
  ob = OB_KLASS(pkg)->ob_str((Object *)pkg);
  printf("%s\n", String_RawString(ob));
  OB_DECREF(ob);
  Pkg_Free_Func(pkg, NULL);
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
  Init_Pkg_Klass();
  test_package();
  Fini_Pkg_Klass();
  Fini_String_Klass();
  Fini_TypeDesc();
  AtomString_Fini();
  return 0;
}
