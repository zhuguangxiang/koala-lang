/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "hashmap.h"

int fast_compare(const char *ptr0, const char *ptr1, int len)
{
    int fast = len / sizeof(size_t) + 1;
    int offset = (fast - 1) * sizeof(size_t);
    int current_block = 0;

    if (len <= sizeof(size_t)) {
        fast = 0;
    }

    size_t *lptr0 = (size_t *)ptr0;
    size_t *lptr1 = (size_t *)ptr1;

    while (current_block < fast) {
        if ((lptr0[current_block] ^ lptr1[current_block])) {
            int pos;
            for (pos = current_block * sizeof(size_t); pos < len; ++pos) {
                if ((ptr0[pos] ^ ptr1[pos]) || (ptr0[pos] == 0) || (ptr1[pos] == 0)) {
                    return (int)((unsigned char)ptr0[pos] - (unsigned char)ptr1[pos]);
                }
            }
        }

        ++current_block;
    }

    while (len > offset) {
        if ((ptr0[offset] ^ ptr1[offset])) {
            return (int)((unsigned char)ptr0[offset] - (unsigned char)ptr1[offset]);
        }
        ++offset;
    }

    return 0;
}

static void generate_random_string(char *str, int length)
{
    static const char charset[] =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    int charset_length = strlen(charset);

    // srand(time(NULL)); // Seed the random number generator

    for (int i = 0; i < length; i++) {
        int random_index = rand() % charset_length;
        str[i] = charset[random_index];
    }

    str[length] = '\0'; // Null-terminate the string
}

struct str {
    HashMapEntry entry;
    char value[0];
};

static int __str_cmp_cb__(void *k1, void *k2)
{
    struct str *s1 = k1;
    struct str *s2 = k2;
    return s1 == s2 || !fast_compare(s1->value, s2->value, 10);
}

static void __str_free_cb__(void *entry, void *data) { free(entry); }

static void test_hashmap_string_compare(void)
{
    srand(time(NULL)); // Seed the random number generator

    HashMap map;
    hashmap_init(&map, __str_cmp_cb__);

    struct str *s;
    int ret;
    struct str *s2;

    for (int i = 0; i < 20000; i++) {
        s = malloc(sizeof(struct str) + 21);
        generate_random_string((char *)(s + 1), 20);
        hashmap_entry_init(s, str_hash((char *)(s + 1)));
        ret = hashmap_put_absent(&map, s);
        assert(!ret);
        s2 = hashmap_get(&map, s);
        assert(__str_cmp_cb__(s2, s));
    }

    hashmap_fini(&map, __str_free_cb__, &map);
}

int main(int argc, char *argv[])
{
    test_hashmap_string_compare();
    // printf("hashmap_fast_string_compare test done\n");
    return 0;
}
