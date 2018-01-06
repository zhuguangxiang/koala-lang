
#include "koala.h"

/* gcc -g -std=c99 test_method.c -lkoala -L. */

void test_method(void)
{
  /*
    var tuple = Tuple(123, true);
    var clazz = tuple.GetClass();
    var m Method = klazz.GetMethod("Size");
    var size = m.invoke(tuple);
  */

}

int main(int argc, char *argv[]) {
  UNUSED_PARAMETER(argc);
  UNUSED_PARAMETER(argv);

  Koala_Initialize();
  test_method();
  Koala_Finalize();

  return 0;
}
