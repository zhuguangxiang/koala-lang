
#include <stdio.h>
#include "module_object.h"
#include "string_object.h"

static struct object *io_print(struct object *ob, struct object *args)
{
  struct string_object *so = (struct string_object *)args;
  printf("%s\n", so->str);
  return NULL;
}

static struct cfunc_struct io_module_functions[] = {
  {"Print", "(Okoala/lang.String)(V)", 0, io_print},
  {NULL, NULL, 0, NULL}
};

void init_io_module(void)
{
  struct object *mo = new_module("koala/io");
  module_add_cfunctions(mo, io_module_functions);
}
