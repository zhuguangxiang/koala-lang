/*
MIT License

Copyright (c) 2018 James, https://github.com/zhuguangxiang

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <assert.h>
#include "vector.h"

int main(int argc, char *argv[])
{
  VECTOR(int_vec, sizeof(int));

  int numbers[100];
  for (int i = 0; i < 100; i++)
    numbers[i] = 1001 + i;

  int res = 0;

  for (int j = 0; j < 100; j++) {
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

  int v;
  res = vector_pop_back(&int_vec, &v);
  assert(res == 0);
  assert(v == 1100);

  iter_reset(&iter);
  k = 0;
  iter_for_each(&iter, val) {
    assert(*val == numbers[k]);
    ++k;
  }
  assert(k == 99);

  res = vector_get(&int_vec, 99, NULL);
  assert(res != 0);

  res = vector_get(&int_vec, 98, &v);
  assert(res == 0);
  assert(v == 1099);

  int num2[20];
  for (int i = 0; i < 20; i++)
    num2[i] = 101 + i;

  VECTOR(num2_vec, sizeof(int));
  for (int j = 0; j < 20; j++) {
    res = vector_push_back(&num2_vec, &num2[j]);
    assert(res == 0);
  }
  res = vector_concat(&int_vec, &num2_vec);
  assert(res == 0);
  assert(vector_size(&int_vec) == 99 + 20);

  iter_reset(&iter);
  k = 0;
  iter_for_each_as(&iter, int, v) {
    if (k < 99)
      assert(v == numbers[k]);
    else
      assert(v == num2[k - 99]);
    ++k;
  }
  assert(k == 99 + 20);
  vector_fini(&int_vec, NULL);
  vector_fini(&num2_vec, NULL);
}
