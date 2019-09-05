/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include <assert.h>
#include "vector.h"

int main(int argc, char *argv[])
{
  VECTOR(int_vec);

  int numbers[100];
  for (int i = 0; i < 100; ++i)
    numbers[i] = 1001 + i;

  int res = 0;

  for (int j = 0; j < 100; ++j) {
    res = vector_push_back(&int_vec, &numbers[j]);
    assert(res == 0);
  }

  int *val;
  VECTOR_ITERATOR(iter, &int_vec);
  int k = 0;
  iter_for_each(&iter, val) {
    assert(*val == numbers[k]);
    ++k;
  }
  assert(k == 100);

  int *v;
  v = vector_pop_back(&int_vec);
  assert(*v == 1100);

  iter_reset(&iter);
  k = 0;
  iter_for_each(&iter, val) {
    assert(*val == numbers[k]);
    ++k;
  }
  assert(k == 99);

  v = vector_get(&int_vec, 99);
  assert(v == NULL);

  v = vector_get(&int_vec, 98);
  assert(*v == 1099);

  int num2[20];
  for (int i = 0; i < 20; ++i)
    num2[i] = 101 + i;

  VECTOR(num2_vec);
  for (int j = 0; j < 20; ++j) {
    res = vector_push_back(&num2_vec, &num2[j]);
    assert(res == 0);
  }
  res = vector_concat(&int_vec, &num2_vec);
  assert(res == 0);
  assert(vector_size(&int_vec) == 99 + 20);

  iter_reset(&iter);
  k = 0;
  int vv;
  iter_for_each_as(&iter, int, vv) {
    if (k < 99)
      assert(vv == numbers[k]);
    else
      assert(vv == num2[k - 99]);
    ++k;
  }
  assert(k == 99 + 20);
  vector_fini(&int_vec, NULL, NULL);
  vector_fini(&num2_vec, NULL, NULL);
}
