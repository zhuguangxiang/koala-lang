/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "hashmap.h"

struct str {
  struct hashmap_entry entry;
  char value[0];
};

static int __str_cmp_cb__(void *k1, void *k2)
{
  struct str *s1 = k1;
  struct str *s2 = k2;
  return strcmp(s1->value, s2->value);
}

static void __str_free_cb__(void *entry, void *data)
{
  kfree(entry);
}

static void random_string(char *data, int len)
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
  hashmap_init(&strmap, __str_cmp_cb__);
  srand(time(NULL));
  struct str *s;
  int res;
  void *s2;

  for (int i = 0; i < 10000; i++) {
    s = kmalloc(sizeof(struct str) + 11);
    random_string((char *)(s + 1), 10);
    hashmap_entry_init(s, strhash((char *)(s + 1)));
    res = hashmap_add(&strmap, s);
    assert(!res);
    s2 = hashmap_get(&strmap, s);
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

  hashmap_free(&strmap, __str_free_cb__, NULL);
}

int main(int argc, char *argv[])
{
  test_string_hash();
  return 0;
}
