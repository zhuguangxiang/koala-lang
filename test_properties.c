
#include "properties.h"

int main(int argc, char *argv[])
{
  UNUSED_PARAMETER(argc);
  UNUSED_PARAMETER(argv);
  Properties prop;
  Properties_Init(&prop);
  Properties_Put(&prop, "path", "~/koala-repo");
  Properties_Put(&prop, "path", "./");
  char **values = Properties_Get(&prop, "path");
  char **val = values;
  while (val && *val) {
    printf("path = %s\n", *val);
    val++;
  }
  free(values);

  Properties_Put(&prop, "config", "~/koala.properties");
  values = Properties_Get(&prop, "config");
  val = values;
  while (val && *val) {
    assert(!strcmp(*val, "~/koala.properties"));
    printf("config = %s\n", *val);
    val++;
  }
  free(values);

  values = Properties_Get(&prop, "koala.debug");
  assert(!values);

  return 0;
}
