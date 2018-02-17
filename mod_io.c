
#include "mod_io.h"

static Object *__io_print(Object *ob, Object *args)
{
  UNUSED_PARAMETER(ob);
  if (args == NULL) {
    fprintf(stdout, "(nil)\n");
    return NULL;
  }

#define BUF_SIZE  512

  char temp[BUF_SIZE];
  char *buf = temp;
  int sz = Tuple_Size(args);
  TValue val;
  int avail = 0;
  for (int i = 0; i < sz; i++) {
    val = Tuple_Get(args, i);
    avail = TValue_Print(buf, BUF_SIZE - (buf - temp), &val);
    buf += avail;
    if (i + 1 < sz) {
      avail = snprintf(buf, BUF_SIZE - (buf - temp), " ");
      buf += avail;
    }
  }
  temp[buf - temp] = '\0';

  fwrite(temp, strlen(temp), 1, stdout);
  fflush(stdout);
  return NULL;
}

static FuncDef io_funcs[] = {
  {"Print", 0, NULL, 1, "...A", __io_print},
  {NULL}
};

void Init_IO_Module(void)
{
  Object *ob = Koala_New_Module("io", "koala/io");
  ASSERT_PTR(ob);
  Module_Add_CFunctions(ob, io_funcs);
}
