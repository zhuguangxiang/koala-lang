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

  Vector *list = string_to_descs("Llang.Tuple;si");
  TypeDesc *desc = vector_get(list, 0);
  assert(desc->kind == TYPE_KLASS);
  desc = vector_get(list, 1);
  assert(desc->kind == TYPE_BASE);
  assert(desc->base.type == BASE_STR);
  desc = vector_get(list, 2);
  assert(desc->kind == TYPE_BASE);
  assert(desc->base.type == BASE_INT);
  desc = vector_get(list, 3);
  assert(!desc);
  VECTOR_ITERATOR(iter, list);
  iter_for_each(&iter, desc) {
    TYPE_DECREF(desc);
  }
  vector_fini(list, NULL, NULL);
  kfree(list);

  list = string_to_descs("s");
  desc = vector_get(list, 0);
  assert(desc->kind == TYPE_BASE);
  assert(desc->base.type == BASE_STR);
  desc = vector_get(list, 1);
  assert(!desc);
  VECTOR_ITERATOR(iter2, list);
  iter_for_each(&iter2, desc) {
    TYPE_DECREF(desc);
  }
  vector_fini(list, NULL, NULL);
  kfree(list);

  fini_typedesc();
  fini_atom();

  return 0;
}
