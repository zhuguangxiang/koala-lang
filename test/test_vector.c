/*
 MIT License

 Copyright (c) 2018 James, https://github.com/zhuguangxiang

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
*/

#include "log.h"
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
    expect(res == 0);
  }

  int *val;
  VECTOR_ITERATOR(iter, &int_vec);
  int k = 0;
  iter_for_each(&iter, val) {
    expect(*val == numbers[k]);
    ++k;
  }
  expect(k == 100);

  int *v;
  v = vector_pop_back(&int_vec);
  expect(*v == 1100);

  iter_reset(&iter);
  k = 0;
  iter_for_each(&iter, val) {
    expect(*val == numbers[k]);
    ++k;
  }
  expect(k == 99);

  v = vector_get(&int_vec, 99);
  expect(v == NULL);

  v = vector_get(&int_vec, 98);
  expect(*v == 1099);

  int num2[20];
  for (int i = 0; i < 20; ++i)
    num2[i] = 101 + i;

  VECTOR(num2_vec);
  for (int j = 0; j < 20; ++j) {
    res = vector_push_back(&num2_vec, &num2[j]);
    expect(res == 0);
  }
  res = vector_concat(&int_vec, &num2_vec);
  expect(res == 0);
  expect(vector_size(&int_vec) == 99 + 20);

  iter_reset(&iter);
  k = 0;
  int vv;
  iter_for_each_as(&iter, int, vv) {
    if (k < 99)
      expect(vv == numbers[k]);
    else
      expect(vv == num2[k - 99]);
    ++k;
  }
  expect(k == 99 + 20);
  vector_fini(&int_vec);
  vector_fini(&num2_vec);
}
