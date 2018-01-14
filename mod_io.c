
#include "mod_io.h"

static Object *__io_print(Object *ob, Object *args)
{
  UNUSED_PARAMETER(ob);
  OB_ASSERT_KLASS(args, String_Klass);
  fprintf(stdout, "%s", String_To_CString(args));
  fflush(stdout);
  return NULL;
}

static FunctionStruct io_functions[] = {
  {"Print", "v", "s", ACCESS_PUBLIC, __io_print},
  {NULL}
};

void Init_IO_Module(void)
{
  Object *ob = Module_New("io", "koala/io", 0);
  Module_Add_CFunctions(ob, io_functions);
}
