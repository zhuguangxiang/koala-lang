/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "koala.h"

static void init_types(void)
{
  int res;

  res = Type_Ready(&Any_Type);
  panic(res, "Cannot initalize 'Any' type.");

  res = Type_Ready(&Class_Type);
  panic(res, "Cannot initalize 'Class' type.");

  res = Type_Ready(&Field_Type);
  panic(res, "Cannot initalize 'Field' type.");

  res = Type_Ready(&Method_Type);
  panic(res, "Cannot initalize 'Method' type.");

  res = Type_Ready(&Proto_Type);
  panic(res, "Cannot initalize 'Proto' type.");

  res = Type_Ready(&Integer_Type);
  panic(res, "Cannot initalize 'Integer' type.");

  res = Type_Ready(&String_Type);
  panic(res, "Cannot initalize 'String' type.");

  res = Type_Ready(&Tuple_Type);
  panic(res, "Cannot initalize 'Tuple' type.");

  res = Type_Ready(&Dict_Type);
  panic(res, "Cannot initalize 'Dict' type.");
}

void Koala_Initialize(void)
{
  atom_init();
  node_init();
  init_types();

  //init_lang_module();
  //init_sys_module();
  //init_io_module();
  //init_fmt_module();
}

void Koala_Finalize(void)
{
  node_fini();
  atom_fini();
}

/*
 * The following commands are valid.
 * ~$ koala a/b/foo.kl [a/b/foo.klc] [a/b/foo]
 */
void Koala_Run(char *path)
{

}
