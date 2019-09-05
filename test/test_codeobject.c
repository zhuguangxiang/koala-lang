/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "codeobject.h"
#include "atom.h"

int main(int argc, char *argv[])
{
  atom_init();

  struct cfuncdef func = {"hello", "Llang.String;si", "s", NULL};
  /*
  Object *code = NULL; //code_from_cfunc(&func);
  CodeObject *co = (CodeObject *)code;
  assert(co->proto->refcnt == 1);
  TYPE_DECREF(co->proto);
  kfree(code);
  */
  fini_typedesc();
  atom_fini();
  return 0;
}
