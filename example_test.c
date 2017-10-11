
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include "gstate.h"
#include "module_object.h"
#include "method_object.h"
#include "integer_object.h"
#include "tuple_object.h"
#include "string_object.h"

/*
gcc -g -std=c99 -Wall hash_table.c hash.c vector.c namei.c example_test.c koala.c \
object.c gstate.c \
module_object.c method_object.c tuple_object.c \
integer_object.c string_object.c io_module.c
*/

void koala_main(char *arg);

void example_test(void)
{
  struct object *mo = find_module("koala/io");
  module_display(mo);
  struct object *ob = NULL;
  int offset = 0;
  NAMEI_DEFINE(ni, "Print", NT_FUNC);
  module_get(mo, &ni, &ob, &offset);
  assert(ob && OB_KLASS(ob) == &method_klass);
  //call function

  module_display(find_module("koala/sys"));
}

int main(int argc, char *argv[])
{
  koala_main("/opt/libs/koala_lib:/opt/libs/third_part_klib");
  example_test();
  return 0;
}
