
#include "module_object.h"
#include "method_object.h"
#include "tuple_object.h"
#include "integer_object.h"
#include "io_module.h"
#include "sys_module.h"
#include "gstate.h"

void init_lang_module(void)
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

void init_modules(void)
{
  init_lang_module();
  init_io_module();
  init_sys_module();
}

void koala_main(char *arg)
{
  init_gstate();
  init_modules();
}
