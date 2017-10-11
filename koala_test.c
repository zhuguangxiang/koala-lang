
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
gcc -g -std=c99 -Wall hash_table.c hash.c vector.c namei.c koala_test.c koala.c \
object.c gstate.c \
module_object.c method_object.c tuple_object.c \
integer_object.c string_object.c io_module.c sys_module.c
*/
void koala_main(char *arg);

void tuple_test(void)
{
  struct object *tuple = new_tuple(10);
  tuple_set(tuple, 0, integer_from_int64(123));
  tuple_set(tuple, 1, integer_from_int64(456));
  int64_t val, v2;
  tuple_parse(tuple, "ii", &val, &v2);
  assert(val == 123 && v2 == 456);
}

void type_system_test(void)
{
  struct object *mo = NULL;
  gstate_get_module("koala/lang", &mo, NULL);
  assert(mo && OB_KLASS(mo) == &module_klass);

  struct namei ni = {
    .name = "Klass",
    .type = NT_KLASS,
  };
  struct object *ko = NULL;
  int offset = 0;
  module_get(mo, &ni, &ko, &offset);
  assert(ko && OB_KLASS(ko) == &klass_klass && offset > 0);
  printf("Klass class:\n");
  klass_display(ko);
  printf("----------------------------------------\n");

  ni.name = "ToString";
  ni.type = NT_FUNC;
  struct object *ob = NULL;
  klass_get(ko, &ni, &ob, NULL);
  assert(ob == NULL);

  ni.name = "Name";
  ni.type = NT_VAR;
  klass_get(ko, &ni, &ob, NULL);
  assert(ob == NULL);

  ni.name = "GetMethods";
  ni.type = NT_FUNC;
  offset = 0;
  klass_get(ko, &ni, &ob, &offset);;
  assert(ob != NULL && OB_KLASS(ob) == &method_klass && offset > 0);

  ni.name = "Method";
  ni.type = NT_KLASS;
  ko = NULL;
  offset = 0;
  module_get(mo, &ni, &ko, &offset);
  assert(ko && offset > 0);
  printf("Method class:\n");
  klass_display(ko);
  printf("----------------------------------------\n");

  ni.name = "Integer";
  ni.type = NT_KLASS;
  ko = NULL;
  offset = 0;
  module_get(mo, &ni, &ko, &offset);
  assert(ko && offset > 0);
  printf("Integer class:\n");
  klass_display(ko);
  printf("----------------------------------------\n");

  ni.name = "ToString";
  ni.type = NT_FUNC;
  ob = NULL;
  offset = 0;
  klass_get(ko, &ni, &ob, &offset);
  assert(ob && offset > 0);
  struct method_object *meth = (struct method_object *)ob;
  assert(meth->flags & METH_CFUNC);
  cfunc_t tostr_func = *(cfunc_t *)(meth + 1);
  ob = tostr_func(integer_from_int64(LONG_MIN), NULL);
  struct string_object *so = (struct string_object *)ob;
  printf("%d:%s\n", so->length, so->str);

  ni.name = "Tuple";
  ni.type = NT_KLASS;
  ko = NULL;
  offset = 0;
  module_get(mo, &ni, &ko, &offset);
  assert(ko && offset > 0);
  printf("Tuple class:\n");
  klass_display(ko);
  printf("----------------------------------------\n");

  ni.name = "Get";
  ni.type = NT_FUNC;
  ob = NULL; offset = 0;
  klass_get(ko, &ni, &ob, &offset);
  assert(ob && OB_KLASS(ob) == &method_klass && offset > 0);

  module_display(mo);
  module_display(find_module("koala/io"));
  module_display(find_module("koala/sys"));
}

int main(int argc, char *argv[])
{
  koala_main("/opt/libs/koala_lib:/opt/libs/third_part_klib");
  type_system_test();
  tuple_test();
  int64_t i = -1;
  printf("%lld\n", i);
  printf("%llu\n", ULLONG_MAX);
  return 0;
}
