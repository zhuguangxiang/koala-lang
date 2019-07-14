/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "module_object.h"
#include "string_object.h"
#include "int_object.h"
#include "tuple_object.h"
#include "code_object.h"

static void mod_lang_initialize(void)
{
  init_module_type();
  init_klass_type();
  init_string_type();
  init_int_type();
  init_tuple_type();
  init_code_type();
}

static void mod_lang_destroy(void)
{
  free_module_type();
  free_klass_type();
  free_string_type();
  free_int_type();
  free_tuple_type();
  free_code_type();
}

void koala_initialize(void)
{
  module_initialize();
  //init_lang_module();
  //init_sys_module();
  //init_io_module();
  //init_fmt_module();
}

void koala_destroy(void)
{
  //free_lang_module();
  module_destroy();
}
