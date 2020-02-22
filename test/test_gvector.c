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
#include "gvector.h"

static void test_int(void)
{
  GVector vec;
  gvector_init(&vec, 4, sizeof(int));

  int numbers[20];
  for (int i = 0; i < 20; ++i)
    numbers[i] = 100 + i;

  for (int i = 0; i < 20; ++i) {
    gvector_push_back(&vec, &numbers[i]);
  }

  int val = 0;
  for (int i = 0; i < 20; ++i) {
    gvector_pop_back(&vec, &val);
    expect(val == 119 - i);
  }

  gvector_fini(&vec);
}

static void test_string(void)
{
  GVector vec;
  gvector_init(&vec, 4, sizeof(char));

  int val = 'a';
  for (int i = 0; i < 26; ++i) {
    gvector_push_back(&vec, &val);
    ++val;
  }

  for (int i = 0; i < 26; ++i) {
    gvector_pop_back(&vec, &val);
    expect(val == 'z'- i);
  }

  gvector_fini(&vec);
}

struct person {
  char *name;
  int age;
};

static void test_struct(void)
{
  GVector vec;
  gvector_init(&vec, 4, sizeof(struct person));

  struct person p = {"foo", 0};

  for (int i = 0; i < 20; ++i) {
    p.age = i + 1;
    gvector_push_back(&vec, &p);
  }

  for (int i = 0; i < 20; ++i) {
    gvector_pop_back(&vec, &p);
    expect(p.age == 20 - i);
  }

  gvector_fini(&vec);
}

static void test_ptr(void)
{
  GVector vec;
  gvector_init(&vec, 4, sizeof(void *));

  struct person p[8] = {
    {"foo", 1},
    {"bar", 2},
    {"bar1", 3},
    {"bar2", 4},
    {"bar3", 5},
    {"bar4", 6},
    {"bar5", 7},
    {"bar6", 8},
  };

  struct person *ps;
  for (int i = 0; i < 8; ++i) {
    ps = &p[i];
    gvector_push_back(&vec, &ps);
  }

  for (int i = 0; i < 8; ++i) {
    gvector_pop_back(&vec, &ps);
    expect(ps == &p[7-i]);
    expect(ps->age = 8 - i);
  }

  gvector_fini(&vec);
}

static void test_push_pop(void)
{
  GVector vec;
  gvector_init(&vec, 4, sizeof(int));

  int i = 0;
  int v;
  for (v = 1; v <= 20; ++v) {
    gvector_push_back(&vec, &v);
    ++i;
  }

  gvector_foreach(v, &vec) {
    expect(v == idx + 1);
  }

  gvector_foreach_reverse(v, &vec) {
    expect(v == idx + 1);
  }

  gvector_top_back(&vec, &v);
  expect(v == 20);
  gvector_top_front(&vec, &v);
  expect(v == 1);

  v = 0;
  gvector_push_front(&vec, &v);
  v = -1;
  gvector_push_front(&vec, &v);

  gvector_top_front(&vec, &v);
  expect(v == -1);

  i = -1;
  while (!gvector_empty(&vec)) {
    gvector_pop_front(&vec, &v);
    expect(v == i);
    ++i;
  }

  gvector_fini(&vec);
}

static void test_insert_remove(void)
{
  GVector vec;
  gvector_init(&vec, 4, sizeof(int));

  int v;
  for (v = 1; v <= 5; ++v) {
    gvector_push_back(&vec, &v);
  }

  for (v = 7; v <= 10; ++v) {
    gvector_push_back(&vec, &v);
  }

  v = 6;
  gvector_insert(&vec, 5, &v);

  gvector_foreach(v, &vec) {
    expect(v == idx + 1);
  }

  gvector_remove(&vec, 7, NULL);
  gvector_remove(&vec, 7, NULL);
  gvector_remove(&vec, 7, NULL);

  int i = 1;
  while (!gvector_empty(&vec)) {
    gvector_pop_front(&vec, &v);
    expect(v == i);
    ++i;
  }

  gvector_fini(&vec);
}

static void test_concat(void)
{
  GVector vec1;
  gvector_init(&vec1, 4, sizeof(int));

  int v;
  for (v = 1; v <= 5; ++v) {
    gvector_push_back(&vec1, &v);
  }

  GVector vec2;
  gvector_init(&vec2, 2, sizeof(int));

  for (v = 6; v <= 8; ++v) {
    gvector_push_back(&vec2, &v);
  }

  gvector_concat(&vec1, &vec2);

  int i = 8;
  while (gvector_empty(&vec1)) {
    gvector_pop_back(&vec1, &v);
    expect(v == i);
    --i;
  }

  i = 8;
  while (gvector_empty(&vec2)) {
    gvector_pop_back(&vec1, &v);
    expect(v == i);
    --i;
  }

  gvector_fini(&vec1);
  gvector_fini(&vec2);
}

static void test_slice(void)
{
  GVector vec;
  gvector_init(&vec, 4, sizeof(int));

  int v;
  for (v = 1; v <= 13; ++v) {
    gvector_push_back(&vec, &v);
  }

  GVector *slice = gvector_split(&vec, 8, 3);
  expect(gvector_size(slice) == 3);
  gvector_foreach(v, slice) {
    expect(v == idx + 9);
  }

  gvector_foreach(v, &vec) {
    expect(v == idx + 1);
  }

  gvector_fini(&vec);
  gvector_free(slice);
}

static int int_cmp(const void *v1, const void *v2)
{
  int iv1 = *(int *)v1;
  int iv2 = *(int *)v2;
  return (iv2 - iv1);
}

static void test_sort(void)
{
  GVector vec;
  gvector_init(&vec, 4, sizeof(int));

  int v;
  for (v = 1; v <= 13; ++v) {
    gvector_push_back(&vec, &v);
  }

  gvector_sort(&vec, int_cmp);

  gvector_foreach_reverse(v, &vec) {
    expect(v == 13 - idx);
  }

  gvector_fini(&vec);
}

typedef union rawvalue {
  long long ival;
  int bval;
  unsigned int cval;
  int zval;
  double fval;
  void *obj;
} RawValue;

static void test_union()
{
  GVector vec;
  gvector_init(&vec, 4, sizeof(char));

  RawValue v;
  for (int i = 0; i < 10; ++i) {
    v.cval = 100 + i;
    gvector_push_back(&vec, &v);
  }

  for (int i = 0; i < 10; ++i) {
    gvector_pop_front(&vec, &v);
    expect(v.cval == 100 + i);
  }

  gvector_fini(&vec);
}

int main(int argc, char *argv[])
{
  test_int();
  test_string();
  test_struct();
  test_ptr();
  test_push_pop();
  test_insert_remove();
  test_concat();
  test_slice();
  test_sort();
  test_union();
  return 0;
}
