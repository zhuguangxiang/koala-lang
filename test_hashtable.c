
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "common.h"
#include "hashtable.h"

/*
 ./build_lib.sh
 gcc -g -std=c99 test_hashtable.c -lkoala -L.
 valgrind ./a.out
 */
struct number {
  uint32 value;
  struct hash_node hnode;
};

struct number number_mem[2000000];
int mem_num_idx;

uint32 number_hash(void *key) {
  return hash_uint32(*(uint32 *)key, 32);
}

int number_equal(void *k1, void *k2) {
  int res = (*(uint32 *)k1 - *(uint32 *)k2 == 0);
  return res;
}

struct number *number_alloc(void) {
  return &number_mem[mem_num_idx++];
}

void number_free(struct hash_node *hnode) {
  UNUSED_PARAMETER(hnode);
}

int idx = 0;
uint32 values[2000000];

int check(uint32 val) {
  int i = 0;
  for (i = 0; i < idx; i++) {
    if (values[i] == val) {
      printf("%d-%d\n", idx, val);
      return -1;
    }
  }
  values[idx++] = val;
  return 0;
}

void number_show(HashTable *table)
{
  struct number *num;
  HashNode *hnode;
  int size = HashTable_SlotSize(table);
  struct hlist_head *head;
  for (int i = 0; i < size; i++) {
    head = table->entries + i;
    if (!HashList_Empty(head)) {
      fprintf(stdout, "[%d]:", i);
      HashList_ForEach(hnode, head) {
        num = container_of(hnode, struct number, hnode);
        fprintf(stdout, "%d\t", num->value);
      }
      fprintf(stdout, "\n");
    }
  }
}

void test_number_hash_table(void) {
  struct hash_table *table;
  struct number *num;
  int failed;
  int i;
  int value = 100;
  Decl_HashInfo(hashinfo, number_hash, number_equal);
  table = HashTable_Create(&hashinfo);
  ASSERT(table);

  srand(time(NULL) + getpid());

  printf("max:%d\n", RAND_MAX);

  int duplicated = 0;

  for (i = 0; i < 1000000; i++) {
    value++; //= rand();
    num = number_alloc();
    ASSERT(num);
    num->value = value;
    Init_HashNode(&num->hnode, &num->value);
    failed = HashTable_Insert(table, &num->hnode);
    //ASSERT(!failed);
    if (failed != 0) {
      //srand(time(NULL) + getpid());
      sleep(1);
      duplicated++;
    }

    struct hash_node *hnode = HashTable_Find(table, &value);
    ASSERT(hnode != NULL);
  }

  number_show(table);

  value = 100;
  for (i = 0; i < 1000000; i++) {
    value++;
    struct hash_node *hnode = HashTable_Find(table, &value);
    ASSERT_PTR(hnode);
    int res = HashTable_Remove(table, hnode);
    ASSERT(res == 0);
  }
  printf("num:%d\n", table->nodes);
  printf("duplicated:%d\n", duplicated);
  HashTable_Destroy(table, NULL, NULL);
}
/*---------------------------------------------------------------------------
string test
---------------------------------------------------------------------------*/
struct string {
  char *value;
  struct hash_node hnode;
};

uint32 string_hash_fn(void *key) {
  uint32 rt = hash_string(*(char **)key);
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
    struct string *str = (struct string *)&string_mem[
      string_idx * (sizeof(struct string) + 20)];
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
  UNUSED_PARAMETER(hnode);
}

void test_string_hash_table(void) {
    struct string *str;
    int failed;
    struct hash_table *string_hash;
    int i;
    Decl_HashInfo(hashinfo, string_hash_fn, string_equal_fn);
    string_hash = HashTable_Create(&hashinfo);

    srand(time(NULL));

    for (i = 0; i < 1000000; i++ ) {
        str = string_alloc(10);
        ASSERT_PTR(str);
        Init_HashNode(&str->hnode, &str->value);
        failed = HashTable_Insert(string_hash, &str->hnode);
        ASSERT(!failed);
    }

    HashTable_Destroy(string_hash, NULL, NULL);
}

int main(int argc, char *argv[]) {
  UNUSED_PARAMETER(argc);
  UNUSED_PARAMETER(argv);
  printf("start:%d\n", (unsigned)time(0));
  test_number_hash_table();
  printf("end:%d\n", (unsigned)time(0));
  test_string_hash_table();
  printf("end2:%d\n", (unsigned)time(0));
  return 0;
}
