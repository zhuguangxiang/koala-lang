/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "typedesc.h"
#include "atom.h"

int main(int argc, char *argv[])
{
  atom_initialize();

  struct typedesc **desc = typestr_toarr("Llang.Tuple;si");
  assert(desc[0]->kind == TYPE_KLASS);
  assert(desc[1]->kind == TYPE_STR);
  assert(desc[2]->kind == TYPE_INT);
  assert(!desc[3]);
  for (int i = 0; i < 3; i++) {
    TYPE_DECREF(desc[i]);
  }
  kfree(desc);

  desc = typestr_toarr("s");
  assert(desc[0]->kind == TYPE_STR);
  assert(!desc[1]);
  TYPE_DECREF(desc[0]);
  kfree(desc);

  typedesc_destroy();
  atom_destroy();
  return 0;
}
