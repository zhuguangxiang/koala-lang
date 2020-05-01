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

// gcc test_vec.c -I../include -lkoala

#include <assert.h>
#include "vec.h"

static void test_int(void)
{
  Vec vec;
  vec_init(&vec, sizeof(int));

  int numbers[20];
  for (int i = 0; i < 20; ++i)
    numbers[i] = 100 + i;

  for (int i = 0; i < 20; ++i) {
    vec_push_back(&vec, &numbers[i]);
  }

  int val = 0;
  for (int i = 0; i < 20; ++i) {
    vec_pop_back(&vec, &val);
    assert(val == 119 - i);
  }

  vec_fini(&vec);
}

static void test_string(void)
{
  Vec vec;
  vec_init(&vec, sizeof(char));

  int val = 'a';
  for (int i = 0; i < 26; ++i) {
    vec_push_back(&vec, &val);
    ++val;
  }

  for (int i = 0; i < 26; ++i) {
    vec_pop_back(&vec, &val);
    assert(val == 'z'- i);
  }

  vec_fini(&vec);
}

struct person {
  char *name;
  int age;
};

static void test_struct(void)
{
  Vec vec;
  vec_init(&vec, sizeof(struct person));

  struct person p = {"foo", 0};

  for (int i = 0; i < 20; ++i) {
    p.age = i + 1;
    vec_push_back(&vec, &p);
  }

  for (int i = 0; i < 20; ++i) {
    vec_pop_back(&vec, &p);
    assert(p.age == 20 - i);
  }

  vec_fini(&vec);
}

static void test_ptr(void)
{
  Vec vec;
  vec_init(&vec, sizeof(void *));

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
    vec_push_back(&vec, &ps);
  }

  for (int i = 0; i < 8; ++i) {
    vec_pop_back(&vec, &ps);
    assert(ps == &p[7-i]);
    assert(ps->age = 8 - i);
  }

  vec_fini(&vec);
}

static void test_push_pop(void)
{
  Vec vec;
  vec_init(&vec, sizeof(int));

  int i = 0;
  int v;
  for (v = 1; v <= 20; ++v) {
    vec_push_back(&vec, &v);
    ++i;
  }

  int *pv;
  vec_foreach(pv, i, &vec) {
    assert(*pv == i + 1);
  }

  vec_top_back(&vec, &v);
  assert(v == 20);
  vec_top_front(&vec, &v);
  assert(v == 1);

  v = 0;
  vec_push_front(&vec, &v);
  v = -1;
  vec_push_front(&vec, &v);

  vec_top_front(&vec, &v);
  assert(v == -1);

  i = -1;
  while (!vec_empty(&vec)) {
    vec_pop_front(&vec, &v);
    assert(v == i);
    ++i;
  }

  vec_fini(&vec);
}

static void test_insert_remove(void)
{
  Vec vec;
  vec_init(&vec, sizeof(int));

  int v;
  for (v = 1; v <= 5; ++v) {
    vec_push_back(&vec, &v);
  }

  for (v = 7; v <= 10; ++v) {
    vec_push_back(&vec, &v);
  }

  v = 6;
  vec_insert(&vec, 5, &v);

  int i;
  int *pv;
  vec_foreach(pv, i, &vec) {
    assert(*pv == i + 1);
  }

  vec_remove(&vec, 7, NULL);
  vec_remove(&vec, 7, NULL);
  vec_remove(&vec, 7, NULL);

  i = 1;
  while (!vec_empty(&vec)) {
    vec_pop_front(&vec, &v);
    assert(v == i);
    ++i;
  }

  vec_fini(&vec);
}

static void test_join(void)
{
  Vec vec1;
  vec_init(&vec1, sizeof(int));

  int v;
  for (v = 1; v <= 5; ++v) {
    vec_push_back(&vec1, &v);
  }

  Vec vec2;
  vec_init(&vec2, sizeof(int));

  for (v = 6; v <= 8; ++v) {
    vec_push_back(&vec2, &v);
  }

  vec_join(&vec1, &vec2);

  int i = 8;
  while (vec_empty(&vec1)) {
    vec_pop_back(&vec1, &v);
    assert(v == i);
    --i;
  }

  i = 8;
  while (vec_empty(&vec2)) {
    vec_pop_back(&vec1, &v);
    assert(v == i);
    --i;
  }

  vec_fini(&vec1);
  vec_fini(&vec2);
}

static int int_cmp(const void *v1, const void *v2)
{
  int iv1 = *(int *)v1;
  int iv2 = *(int *)v2;
  return (iv1 - iv2);
}

static void test_sort(void)
{
  Vec vec;
  vec_init(&vec, sizeof(int));

  int v;
  for (v = 12; v >= 1; --v) {
    vec_push_back(&vec, &v);
  }

  vec_sort(&vec, int_cmp);

  int i;
  int *pv;
  vec_foreach(pv, i, &vec) {
    assert(*pv == i + 1);
  }

  vec_fini(&vec);
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
  Vec vec;
  vec_init(&vec, sizeof(RawValue));

  RawValue v;
  for (int i = 0; i < 10; ++i) {
    v.cval = 100 + i;
    vec_push_back(&vec, &v);
  }

  for (int i = 0; i < 10; ++i) {
    vec_pop_front(&vec, &v);
    assert(v.cval == 100 + i);
  }

  vec_fini(&vec);
}

void test_reverse(void)
{
  Vec vec;
  vec_init(&vec, sizeof(int));

  int v;
  for (v = 12; v >= 1; --v) {
    vec_push_back(&vec, &v);
  }

  vec_reverse(&vec);

  int i;
  int *pv;
  vec_foreach(pv, i, &vec) {
    assert(*pv == i + 1);
  }

  vec_fini(&vec);
}

int main(int argc, char *argv[])
{
  test_int();
  test_string();
  test_struct();
  test_ptr();
  test_push_pop();
  test_insert_remove();
  test_join();
  test_sort();
  test_union();
  test_reverse();
  return 0;
}
