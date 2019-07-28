/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include <assert.h>
#include "iterator.h"

static void *number_next(struct iterator *iter)
{
  int *ptr = iter->iterable;
  if (iter->index < 100) {
    iter->item = ptr + iter->index;
    iter->index++;
  } else {
    iter->item = NULL;
  }
  return iter->item;
}

int main(int argc, char *argv[])
{
  int numbers[100];
  for (int i = 0; i < 100; ++i)
    numbers[i] = 1001 + i;

  ITERATOR(iter, numbers, number_next);
  int j = 0;
  int val;
  iter_for_each_as(&iter, int, val) {
    assert(val == numbers[j]);
    ++j;
  }
  assert(j == 100);

  iter_reset(&iter);
  j = 0;
  int *pval;
  iter_for_each(&iter, pval) {
    assert(*pval == numbers[j]);
    ++j;
  }
  assert(j == 100);
}
