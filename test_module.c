
#include "koala.h"

/* gcc -g -std=c99 test_module.c -lkoala -L. */

void test_module(void)
{
  Object *ob = Koala_Get_Module("koala/lang");
  Module_Show(ob);
  Klass *klazz = Module_Get_Class(ob, "Tuple");
  assert(klazz == &Tuple_Klass);
  ob = Koala_Get_Module("koala/io");
  Module_Show(ob);
}

int main(int argc, char *argv[])
{
  UNUSED_PARAMETER(argc);
  UNUSED_PARAMETER(argv);

  Koala_Initialize();
  test_module();
  Koala_Finalize();

  return 0;
}
