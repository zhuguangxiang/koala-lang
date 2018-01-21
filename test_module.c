
#include "koala.h"

/* gcc -g -std=c99 test_module.c -lkoala -L. */

void test_module(void)
{
  Object *ob = Koala_Get_Module("koala/lang");
  Module_Display(ob);
  Klass *klazz = Module_Get_Class(ob, "Tuple");
  KLASS_ASSERT(klazz, Tuple_Klass);
}

int main(int argc, char *argv[])
{
  UNUSED_PARAMETER(argc);
  UNUSED_PARAMETER(argv);

  Koala_Init();
  test_module();
  Koala_Fini();

  return 0;
}
