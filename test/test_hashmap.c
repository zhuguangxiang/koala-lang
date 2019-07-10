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

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "hashmap.h"

struct str {
  struct hashmap_entry entry;
  char value[0];
};

int str_cmp(void *k1, void *k2)
{
  struct str *s1 = k1;
  struct str *s2 = k2;
  return strcmp(s1->value, s2->value);
}

void str_free(struct hashmap_entry *e, void *data)
{
  kfree(e);
}

void random_string(char *data, int len)
{
  static const char char_set[] = "0123456789abcdefghijklmnopqrstuvwxyz" \
    "ABCDEFGHIJKLMNOPQRSTUWXYZ";
  int i;
  int idx;

  for (i = 0; i < len; i++) {
    idx = rand() % (sizeof(char_set)/sizeof(char_set[0]) - 1);
    data[i] = char_set[idx];
  }
  data[i] = 0;
}

void test_string_hash(void)
{
  struct hashmap strmap;
  hashmap_init(&strmap, str_cmp);
  srand(time(NULL));
  struct str *s;
  int res;
  void *s2;

  for (int i = 0; i < 1000000; i++) {
    s = kmalloc(sizeof(struct str) + 11);
    random_string((char *)(s + 1), 10);
    hashmap_entry_init(&s->entry, strhash((char *)(s + 1)));
    res = hashmap_add(&strmap, &s->entry);
    assert(!res);
    s2 = hashmap_get(&strmap, (struct hashmap_entry *)s);
    assert(s2);
  }

#if 0
  for (int i = 0; i < strmap.size; i++) {
    struct str *s = (struct str *)strmap.entries[i];
    printf("[%d]:", i);
    while (s) {
      printf("%s\t", s->value);
      s = (struct str *)(s->entry.next);
    }
    printf("\n");
  }
#endif

  hashmap_free(&strmap, str_free, NULL);
}

int main(int argc, char *argv[])
{
  test_string_hash();
  return 0;
}
