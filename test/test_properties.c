
#include "properties.h"

void test_properties(void)
{
  Properties properties;
  Properties_Init(&properties);
  Properties_Put(&properties, "foo", "foo1");
  char *v = Properties_GetOne(&properties, "foo");
  assert(!strcmp("foo1", v));
  Properties_Put(&properties, "foo", "foo2");
  Properties_Put(&properties, "bar", "bar1");
  v = Properties_GetOne(&properties, "bar");
  assert(!strcmp("bar1", v));
  Vector *vec = Properties_Get(&properties, "foo");
  assert(vec);
  char *val;
  Vector_ForEach(val, vec) {
    if (i == 0) {
      assert(!strcmp("foo1", val));
    } else if (i == 1) {
      assert(!strcmp("foo2", val));
    } else {
      assert(0);
    }
  }
  Properties_Fini(&properties);
}

int main(int argc, char *argv[])
{
  AtomString_Init();
  test_properties();
  AtomString_Fini();
  return 0;
}
