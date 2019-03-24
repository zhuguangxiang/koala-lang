
#include "koala.h"

void test_package(void)
{
  Object *pkg = New_Package("skeleton");
  TypeDesc *desc = TypeDesc_Get_Base(BASE_INT);
  Pkg_Add_Var(pkg, "Foo", desc);
  Pkg_Add_Var(pkg, "Bar", desc);
  desc = TypeDesc_Get_Base(BASE_STRING);
  Object *so = String_New("/opt/");
  Pkg_Add_Const(pkg, "FILENAME", desc, so);
  OB_DECREF(so);
  Install_Package("skeleton", pkg);
  Object *barvalue = Integer_New(100);
  Pkg_Set_Value(pkg, "Bar", barvalue);
  OB_DECREF(barvalue);
  barvalue = Pkg_Get_Value(pkg, "Bar");
  assert(barvalue && Integer_Raw(barvalue) == 100);
  assert(((PkgObject *)pkg)->nrvars == 2);
  Object *ob = Pkg_Get_Value(pkg, "FILENAME");
  assert(ob);
  assert(!strcmp("/opt/", String_Raw(ob)));
  //ob = To_String((Object *)pkg);
  //printf("%s\n", String_Raw(ob));
  //OB_DECREF(ob);

  //Klass *klazz = &String_Klass;
  //ob = To_String((Object *)klazz);
  //printf("%s\n", String_Raw(ob));
  //OB_DECREF(ob);
}

int main(int argc, char *argv[])
{
  Koala_Initialize();
  test_package();
  Koala_Finalize();
  return 0;
}
