/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include <pthread.h>
#include "koala.h"
#include "langmodule.h"
#include "osmodule.h"
#include "iomodule.h"
#include "fmtmodule.h"

static void init_types(void)
{
  print("########\n");

  int res = type_ready(&Type_Type);
  if (res != 0)
    panic("Cannot initalize 'Type' type.");

  init_any_type();
  init_desc_type();
  init_integer_type();
  init_byte_type();
  init_bool_type();
  init_string_type();
  init_char_type();
  init_float_type();

  init_array_type();
  init_tuple_type();
  init_dict_type();
  init_field_type();
  init_method_type();
  init_proto_type();
  init_class_type();
  init_module_type();

  res = type_ready(&Code_Type);
  if (res != 0)
    panic("Cannot initalize 'Code' type.");

  init_fmtter_type();

  print("########\n");
}

static void fini_types(void)
{
  print("########\n");
  Type_Fini(&Type_Type);
  Type_Fini(&Any_Type);
  Type_Fini(&Byte_Type);
  Type_Fini(&Integer_Type);
  Type_Fini(&Bool_Type);
  Type_Fini(&String_Type);
  Type_Fini(&Char_Type);
  Type_Fini(&Float_Type);
  Type_Fini(&array_type);
  Type_Fini(&tuple_type);
  Type_Fini(&Dict_Type);
  Type_Fini(&Field_Type);
  Type_Fini(&method_type);
  Type_Fini(&proto_type);
  Type_Fini(&class_type);
  Type_Fini(&Module_Type);
  Type_Fini(&Code_Type);
  Type_Fini(&Fmtter_Type);
  print("########\n");
}

void Koala_Initialize(void)
{
  init_atom();
  init_typedesc();
  init_types();
  init_lang_module();
  init_os_module();
  init_io_module();
  init_fmt_module();
  init_parser();
}

void Koala_Finalize(void)
{
  fini_parser();
  fini_fmt_moudle();
  fini_io_module();
  fini_os_module();
  fini_lang_module();
  fini_types();
  fini_typedesc();
  fini_atom();
}

/*
static void run_klc(char *klc)
{
  if (exist(klc)) {
    run();
    return;
  }

  char *kl = str_ndup(klc, strlen(klc) - 1);
  if (exist(kl)) {
    compile();
    run();
    kfree(kl);
    return;
  }

  char *dir = str_ndup(klc, strlen(klc) - 4);
  if (exist(dir)) {
    compile();
    run();
    kfree(dir);
    return;
  }

  error();
}

static void run_kl(char *kl)
{
  char *klc;
  if (exist(klc)) {
    return;
  }

  if (!exist(kl)) {
    return;
  }

  compile();
  run_klc(klc);
}

static void run_dir(char *dir)
{

}
*/

/*
 * The following commands are valid.
 * ~$ koala a/b/foo.kl [a/b/foo.klc] [a/b/foo]
 */
void Koala_Run(char *path)
{
  /*
  if (isdotklc(path)) {
    run_klc(path);
  } else if (isdotkl(path)) {
    run_kl(path);
  } else {
    run_dir(path);
  }
  */
}
