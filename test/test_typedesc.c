/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "typedesc.h"
#include "atom.h"
#include <assert.h>

int main(int argc, char *argv[])
{
  init_atom();
  init_typedesc();

  TypeDesc *proto = str_to_proto("Llang.Tuple;si", "z");
  Vector *vec = proto->proto.args;
  TypeDesc *desc = vector_get(vec, 0);
  assert(desc->kind == TYPE_KLASS);
  desc = vector_get(vec, 1);
  assert(desc->kind == TYPE_BASE);
  assert(desc->base == BASE_STR);
  desc = vector_get(vec, 2);
  assert(desc->kind == TYPE_BASE);
  assert(desc->base == BASE_INT);
  desc = vector_get(vec, 3);
  assert(!desc);
  TYPE_DECREF(proto);

  fini_typedesc();
  fini_atom();

  return 0;
}
