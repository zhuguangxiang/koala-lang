/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "code_object.h"
#include "atom.h"

int main(int argc, char *argv[])
{
  atom_initialize();
  struct cfuncdef func = {"hello", "Llang.String;si", "s", NULL};
  struct object *code = code_from_cfunc(&func);
  struct code_object *co = (struct code_object *)code;
  assert(co->proto->refcnt == 1);
  TYPE_DECREF(co->proto);
  kfree(code);
  typedesc_destroy();
  atom_destroy();
  return 0;
}
