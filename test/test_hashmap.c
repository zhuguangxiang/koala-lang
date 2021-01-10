/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangxiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "hashmap.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct str {
    HashMapEntry entry;
    char value[0];
};

static int __str_cmp_cb__(void *k1, void *k2)
{
    struct str *s1 = k1;
    struct str *s2 = k2;
    return s1 == s2 || !strcmp(s1->value, s2->value);
}

static void __str_free_cb__(void *entry, void *data)
{
    free(entry);
}

static void random_string(char *data, int len)
{
    static const char char_set[] = "0123456789abcdefghijklmnopqrstuvwxyz"
                                   "ABCDEFGHIJKLMNOPQRSTUWXYZ";
    int i;
    int idx;

    for (i = 0; i < len; ++i) {
        idx = rand() % (sizeof(char_set) / sizeof(char_set[0]) - 1);
        data[i] = char_set[idx];
    }
    data[i] = 0;
}

void test_hashmap(void)
{
    srand(time(NULL));

    HashMap map;
    hashmap_init(&map, __str_cmp_cb__);

    struct str *s;
    int ret;
    struct str *s2;

    for (int i = 0; i < 10000; i++) {
        s = malloc(sizeof(struct str) + 11);
        random_string((char *)(s + 1), 10);
        hashmap_entry_init(s, strhash((char *)(s + 1)));
        ret = hashmap_put_absent(&map, s);
        assert(!ret);
        s2 = hashmap_get(&map, s);
        assert(__str_cmp_cb__(s2, s));
    }

    hashmap_fini(&map, __str_free_cb__, &map);
}

int main(int argc, char *argv[])
{
    test_hashmap();
    return 0;
}
