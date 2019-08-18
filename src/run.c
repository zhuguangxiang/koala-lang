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

  int res = Type_Ready(&Type_Type);
  if (res != 0)
    panic("Cannot initalize 'Class' type.");

  res = Type_Ready(&Any_Type);
  if (res != 0)
    panic("Cannot initalize 'Any' type.");

  res = Type_Ready(&Byte_Type);
  if (res != 0)
    panic("Cannot initalize 'Byte' type.");

  res = Type_Ready(&Integer_Type);
  if (res != 0)
    panic("Cannot initalize 'Integer' type.");

  res = Type_Ready(&Bool_Type);
  if (res != 0)
    panic("Cannot initalize 'Bool' type.");

  res = Type_Ready(&String_Type);
  if (res != 0)
    panic("Cannot initalize 'String' type.");

  //res = Type_Ready(&Float_Type);
  //panic("Cannot initalize 'Float' type.");

  res = Type_Ready(&Array_Type);
  if (res != 0)
    panic("Cannot initalize 'Array' type.");

  res = Type_Ready(&Tuple_Type);
  if (res != 0)
    panic("Cannot initalize 'Tuple' type.");

  res = Type_Ready(&Dict_Type);
  if (res != 0)
    panic("Cannot initalize 'Dict' type.");

  res = Type_Ready(&Field_Type);
  if (res != 0)
    panic("Cannot initalize 'Field' type.");

  res = Type_Ready(&Method_Type);
  if (res != 0)
    panic("Cannot initalize 'Method' type.");

  res = Type_Ready(&Proto_Type);
  if (res != 0)
    panic("Cannot initalize 'Proto' type.");

  res = Type_Ready(&Class_Type);
  if (res != 0)
    panic("Cannot initalize 'Class' type.");

  res = Type_Ready(&Module_Type);
  if (res != 0)
    panic("Cannot initalize 'Module' type.");

  res = Type_Ready(&Code_Type);
  if (res != 0)
    panic("Cannot initalize 'Code' type.");

  res = Type_Ready(&Fmtter_Type);
  if (res != 0)
    panic("Cannot initalize 'Formatter' type.");

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
  //Type_Fini(&Float_Type);
  Type_Fini(&Array_Type);
  Type_Fini(&Tuple_Type);
  Type_Fini(&Dict_Type);
  Type_Fini(&Field_Type);
  Type_Fini(&Method_Type);
  Type_Fini(&Proto_Type);
  Type_Fini(&Class_Type);
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
 * The following commands are valid.
 * ~$ koala a/b/foo.kl [a/b/foo.klc] [a/b/foo]
 */
void Koala_Run(char *path)
{

}

KoalaState *Koala_GetState(void)
{

}