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

  typedesc *proto = str_to_proto("Llang.Tuple;si", "z");
  vector *vec = proto->proto.args;
  typedesc *desc = vector_get(vec, 0);
  assert(desc->kind == TYPE_KLASS);
  desc = vector_get(vec, 1);
  assert(desc->kind == TYPE_BASE);
  assert(desc->base == BASE_STR);
  desc = vector_get(vec, 2);
  assert(desc->kind == TYPE_BASE);
  assert(desc->base == BASE_INT);
  desc = vector_get(vec, 3);
  assert(!desc);
  vector_iterator(iter, vec);
  iter_for_each(&iter, desc) {
    desc_decref(desc);
  }
  desc_decref(proto);

  fini_typedesc();
  fini_atom();

  return 0;
}
