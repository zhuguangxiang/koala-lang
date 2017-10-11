
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "hash_table.h"

struct number {
  uint32_t value;
  struct hash_node hnode;
};

struct number number_mem[2000000];
int mem_num_idx;

uint32_t number_hash(void *key) {
  return hash_uint32(*(uint32_t *)key, 32);
}

int number_equal(void *k1, void *k2) {
  int res = (*(uint32_t *)k1 - *(uint32_t *)k2 == 0);
  return res;
}

struct number *number_alloc(void) {
  return &number_mem[mem_num_idx++];
}

void number_free(struct hash_node *hnode) {
  //printf("free:0x%x\t", num->value);
}

uint32_t idx = 0;
uint32_t values[2000000];

int check(uint32_t val) {
  uint32_t i = 0;
  for (i = 0; i < idx; i++) {
    if (values[i] == val) {
      printf("%d-%d\n", idx, val);
      return -1;
    }
  }
  values[idx++] = val;
  return 0;
}

void number_show(struct hlist_head *head, int size, void *arg)
{
  struct number *num;
  struct hash_node *hnode;
  for (int i = 0; i < size; i++) {
    if (!hlist_empty(head)) {
      fprintf(stdout, "[%d]:", i);
      hlist_for_each_entry(hnode, head, link) {
        num = container_of(hnode, struct number, hnode);
        fprintf(stdout, "%d\t", num->value);
      }
      fprintf(stdout, "\n");
    }
    head++;
  }
}

void test_number_hash_table(void) {
  struct hash_table *table;
  struct number *num;
  int failed;
  int i;
  int value = 100;
  table = hash_table_create(number_hash, number_equal);
  assert(table);

  srand(time(NULL) + getpid());

  printf("max:%d\n", RAND_MAX);

  int duplicated = 0;

  for (i = 0; i < 1000000; i++) {
    value++; //= rand();
    #if 0
    if (check(value) != 0) {
      srand(time(NULL) + getpid());
      duplicated++;
      continue;
    }
    #endif

    num = number_alloc();
    assert(num);
    num->value = value;
    init_hash_node(&num->hnode, &num->value);
    failed = hash_table_insert(table, &num->hnode);
    //assert(!failed);
    if (failed != 0) {
      //srand(time(NULL) + getpid());
      sleep(1);
      duplicated++;
    }

    struct hash_node *hnode = hash_table_find(table, &value);
    assert(hnode != NULL);
  }

  hash_table_traverse(table, number_show, NULL);

  value = 100;
  for (i = 0; i < 1000000; i++) {
    value++;
    struct hash_node *hnode = hash_table_find(table, &value);
    assert(hnode != NULL);
    int res = hash_table_remove(table, hnode);
    assert(res == 0);
  }
  printf("num:%d\n", table->nr_nodes);
  printf("duplicated:%d\n", duplicated);
  hash_table_destroy(table, NULL, NULL);
}
/*---------------------------------------------------------------------------
string test
---------------------------------------------------------------------------*/
struct string {
  char *value;
  struct hash_node hnode;
};

uint32_t string_hash_fn(void *key) {
  uint32_t rt = hash_string(*(char **)key);
  return rt;
}

int string_equal_fn(void *k1, void *k2) {
  return strcmp(*(char **)k1, *(char **)k2) ? 0 : 1;
}

int random_string(char *data, int len) {
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

char string_mem[(sizeof(struct string) + 20) * 10000000];
int string_idx;

struct string *string_alloc(int len) {
    int failed;
    struct string *str = (struct string *)&string_mem[string_idx * (sizeof(struct string) + 20)];   //malloc(sizeof(*str) + len + 1);
    if (!str)
        return NULL;
    failed = random_string((char *)(str + 1), len);
    if (failed) {
        //free(str);
        return NULL;
    }

    str->value = (char *)(str + 1);
    string_idx++;
    return str;
}

void string_free(struct hash_node *hnode) {
    //free(str);
}

void test_string_hash_table(void) {
    struct string *str;
    int failed;
    struct hash_table *string_hash;
    int i;
    //struct ht_iterator iter;
    struct hash_node *hnode;

    string_hash = hash_table_create(string_hash_fn, string_equal_fn);

    srand(time(NULL));

    for (i = 0; i < 1000000; i++ ) {
        str = string_alloc(10);
        assert(str);
        init_hash_node(&str->hnode, &str->value);
        failed = hash_table_insert(string_hash, &str->hnode);
        assert(!failed);
    }

#if 0
    ht_iterator_init(&string_hash, &iter);
    while ((hnode = ht_iterator_next(&iter))) {
        printf("%s\n", ((struct string *)hnode)->value);
    }
#endif

    hash_table_destroy(string_hash, NULL, NULL);
}

int main(int argc, char *argv[]) {
  printf("start:%d\n", (unsigned)time(0));
  test_number_hash_table();
  printf("end:%d\n", (unsigned)time(0));
  test_string_hash_table();
  printf("end2:%d\n", (unsigned)time(0));
  return 0;
}
