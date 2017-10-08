
#include "module_object.h"
#include "method_object.h"
#include "tuple_object.h"
#include "integer_object.h"
#include "gstate.h"

void init_builtin_klasses(void)
{
  /* koala/lang.kl */
  init_klass_klass();
  init_method_klass();
  //init_field_klass();
  init_integer_klass();
  init_tuple_klass();
  /*

  init_float_klass();
  init_string_klass();
  init_table_klass();
  init_tuple_klass();
  init_list_klass();
  */
}

void koala_main(char *arg)
{
  init_gstate();
  init_builtin_klasses();
}
