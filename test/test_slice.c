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

// gcc test_slice.c ../src/slice.c -I../include -lkoala

#include <assert.h>
#include "slice.h"

typedef struct sarray {
  // Pointer to the stored memory
  char *elems;
  // This struct's reference count
  int refcnt;
  // Size of each element in bytes
  int objsize;
  // Capacity in number of elements
  int capacity;
} SArray;

static void test_slice(void)
{
  int res;
  Slice slice;
  SArray *sarr;
  slice_init(&slice, sizeof(int));
  assert(slice.offset == 0);
  assert(slice_len(&slice) == 0);

  sarr = slice.ptr;
  assert(sarr->refcnt == 1);
  assert(sarr->objsize == sizeof(int));
  assert(sarr->capacity == 8);

  slice_reserve(&slice, 20);
  assert(slice.offset == 0);
  assert(slice_len(&slice) == 20);
  assert(sarr->capacity == 32);

  Slice slice2;
  slice_slice(&slice2, &slice, 5, 5);
  assert(slice2.offset == 5);
  assert(slice_len(&slice2) == 5);
  assert(sarr->refcnt == 2);

  Slice slice3;
  res = slice_slice(&slice3, &slice2, 1, 5);
  assert(res != 0);

  res = slice_slice(&slice3, &slice2, 1, 3);
  assert(res == 0);
  assert(slice3.offset == 6);
  assert(slice_len(&slice3) == 3);
  assert(sarr->refcnt == 3);

  slice_fini(&slice3);
  assert(sarr->refcnt == 2);
  slice_fini(&slice2);
  assert(sarr->refcnt == 1);
  slice_fini(&slice);
}

static int int_cmp(const void *v1, const void *v2)
{
  return *(int *)v1 - *(int *)v2;
}

void test_int(void)
{
  int res;
  Slice slice;
  slice_init(&slice, sizeof(int));

  int numbers[20];
  for (int i = 0; i < 20; ++i)
    numbers[i] = 100 + i;

  for (int i = 0; i < 20; ++i) {
    slice_push_back(&slice, &numbers[i]);
  }

  int val;
  slice_get(&slice, 0, &val);
  assert(val == numbers[0]);

  slice_pop_front(&slice, &val);
  assert(val == numbers[0]);
  slice_push_front(&slice, &val);

  int i = slice_index(&slice, &numbers[5], int_cmp);
  assert(i == 5);

  int *ptr;
  slice_foreach(ptr, i, &slice) {
    assert(*ptr == numbers[i]);
  }

  slice_reverse(&slice);

  i = slice_index(&slice, &numbers[5], int_cmp);
  assert(i == 14);

  i = slice_last_index(&slice, &numbers[5], int_cmp);
  assert(i == 14);

  slice_reverse(&slice);

  Slice slice2;
  slice_slice(&slice2, &slice, 1, 5);
  slice_foreach(ptr, i, &slice2) {
    assert(*ptr == numbers[1 + i]);
  }

  slice_reverse(&slice2);
  slice_sort(&slice2, int_cmp);
  slice_foreach(ptr, i, &slice2) {
    assert(*ptr == numbers[1 + i]);
  }

  for (int i = 0; i < 20; ++i) {
    slice_pop_back(&slice, &val);
    assert(val == numbers[20 - i - 1]);
  }

  slice_fini(&slice2);
  slice_fini(&slice);
}

int main(int argc, char *argv[])
{
  test_slice();
  test_int();
  return 0;
}
