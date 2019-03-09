
#include "koala.h"

void test_intobject(void)
{
  Object *ob = Integer_New(100);
  Object *s = To_String(ob);
  assert(!strcmp("100", String_Raw(s)));
  OB_DECREF(ob);
  OB_DECREF(s);
}

int main(int argc, char *argv[])
{
  Koala_Initialize();
  test_intobject();
  Koala_Finalize();
  return 0;
}
