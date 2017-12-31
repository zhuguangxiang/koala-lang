
#include <unistd.h>
#include <time.h>
#include "types.h"
#include "vector.h"
/*
 ./build_lib.sh
 gcc -g -std=c99 vector_test.c -lkoala -L.
 valgrind ./a.out
 */
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

char mem[30][10];
char *strings[30];

void test_vector_set(void) {
  struct vector *vec;
  vec = vector_create();
  assert(vec);
  int res;
  for (int i = 0; i < 30; i++) {
    res = vector_set(vec, i, strings+i);
    assert(res >= 0);
  }

  for (int i = 0; i < vector_capacity(vec); i++) {
    char **s = (char **)vector_get(vec, i);
    if (s) printf("%s\n", *s);
  }

  vector_destroy(vec, NULL, NULL);
}

void init_strings(void) {
  srand(time(NULL));
  for (int i = 0; i < 30; i++) {
    strings[i] = mem[i];
    random_string(strings[i], 9);
  }
}

int main(int argc, char *argv[]) {
  init_strings();
  test_vector_set();
  return 0;
}
