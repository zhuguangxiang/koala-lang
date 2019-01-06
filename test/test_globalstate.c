
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "stringobject.h"
#include "globalstate.h"
#include "intobject.h"

void test_globalstate(void)
{
  PackageObject *pkg = (PackageObject *)Koala_Get_Package("lang");
  TypeDesc *desc = TypeDesc_Get_Basic(BASIC_INT);
  Package_Add_Var(pkg, "Foo", desc, 0);
  Package_Add_Var(pkg, "Bar", desc, 0);
  OB_INCREF(pkg);
  Koala_Add_Package("github.com/koala", pkg);
  Koala_Set_Value(pkg, "Foo", Integer_New(100));
  Object *ob = Koala_Get_Value(pkg, "Foo");
  assert(ob);
  assert(100 == Integer_ToCInt(ob));
  OB_DECREF(ob);
  Koala_Set_Value(pkg, "Foo", String_New("hello"));
  ob = Koala_Get_Value(pkg, "Foo");
  assert(ob);
  assert(!strcmp("hello", String_RawString(ob)));
  OB_INCREF(pkg);
  Koala_Add_Package("github.com/", pkg);
  pkg = Package_New("io");
  Koala_Add_Package("github.com/home/zgx/koala/", pkg);
  OB_INCREF(pkg);
  Koala_Add_Package("github.com/", pkg);
  OB_INCREF(pkg);
  Koala_Add_Package("github.com/koala", pkg);
  Koala_Show_Packages();
  ob = Koala_Get_Package("github.com/koala/io");
  assert(ob);
  ob = Koala_Get_Package("github.com/koala/lang");
  assert(ob);
}

int main(int argc, char *argv[])
{
  Koala_Initialize();
  test_globalstate();
  Koala_Finalize();
  return 0;
}
