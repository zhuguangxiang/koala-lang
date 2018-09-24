
#include "koala.h"

/* gcc -g -std=c99 test_string.c -lkoala -L. */

void test_string(void)
{
  Object *ob = String_New("hello");
  char *s = String_RawString(ob);
  assert(strcmp(s, "hello") == 0);
  String_Free(ob);
}

int main(int argc, char *argv[]) {
  UNUSED_PARAMETER(argc);
  UNUSED_PARAMETER(argv);

  Koala_Initialize();
  test_string();
  Koala_Finalize();

  return 0;
}
