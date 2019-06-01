
#include <unistd.h>
#include <time.h>
#include "vector.h"

int random_string(char *data, int len)
{
  static const char char_set[] = "0123456789abcdefghijklmnopqrstuvwxyz" \
                                 "ABCDEFGHIJKLMNOPQRSTUWXYZ";
  int i;
  int idx;

  for (i = 0; i < len; i++) {
    idx = rand() % (nr_elts(char_set) - 1);
    data[i] = char_set[idx];
  }

  data[i] = 0;

  return 0;
}

char mem[40][10];
char *strings[40];

void string_check(char *s, int index)
{
  assert(strcmp(s, strings[index]) == 0);
}

void test_vector_append(void)
{
  Vector *vec = Vector_New(sizeof(char *));
  int res;
  int i;
  for (i = 0; i < 30; i++) {
    res = Vector_Append(vec, &strings[i]);
    assert(res >= 0);
  }

  for (i = 0; i < Vector_Size(vec); i++) {
    char *s = Vector_Get_As(vec, i, char *);
    assert(s);
    string_check(s, i);
  }

  Vector_Free_Self(vec);
}

void test_vector_set(void)
{
  VECTOR(vec, sizeof(char *));
  int res;
  int i;
  for (i = 0; i < 30; i++) {
    res = Vector_Set(&vec, i, &strings[i]);
    assert(res >= 0);
  }

  for (i = 0; i < Vector_Size(&vec); i++) {
    char *s = Vector_Get_As(&vec, i, char *);
    string_check(s, i);
  }
  assert(Vector_Size(&vec) == 30);

  res = Vector_Set(&vec, -1, &strings[30]);
  assert(res < 0);

  res = Vector_Set(&vec, 10, &strings[31]);
  assert(res >= 0);
  assert(Vector_Size(&vec) == 30);

  res = Vector_Set(&vec, 20, &strings[32]);
  assert(res >= 0);
  assert(Vector_Size(&vec) == 30);

  for (i = 0; i < Vector_Size(&vec); i++) {
    char *s = Vector_Get_As(&vec, i, char *);
    if (i == 10)
      string_check(s, 31);
    else if (i == 20)
      string_check(s, 32);
    else
      string_check(s, i);
  }

  Vector_Fini(&vec, NULL, NULL);
}

void test_vector_concat(void)
{
  Vector *vec = Vector_New(sizeof(char *));
  assert(vec);
  int res;
  int i;
  for (i = 0; i < 30; i++) {
    res = Vector_Set(vec, i, &strings[i]);
    assert(res >= 0);
  }

  for (i = 0; i < Vector_Size(vec); i++) {
    char *s = Vector_Get_As(vec, i, char *);
    string_check(s, i);
  }

  Vector *vec2 = Vector_New(sizeof(char *));
  assert(vec2);
  for (i = 0; i < 10; i++) {
    res = Vector_Set(vec2, i, &strings[30 + i]);
    assert(res >= 0);
  }

  Vector_Concat(vec, vec2);

  for (i = 0; i < Vector_Size(vec); i++) {
    char *s = Vector_Get_As(vec, i, char *);
    string_check(s, i);
  }

  Vector_Free_Self(vec);
  Vector_Free_Self(vec2);
}

void init_strings(void)
{
  srand(time(NULL));
  for (int i = 0; i < 40; i++) {
    strings[i] = mem[i];
    random_string(strings[i], 9);
  }
}

int main(int argc, char *argv[])
{
  UNUSED_PARAMETER(argc);
  UNUSED_PARAMETER(argv);
  init_strings();
  test_vector_append();
  test_vector_set();
  test_vector_concat();
  return 0;
}
