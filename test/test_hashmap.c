/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include <time.h>
#include "hashmap.h"

#ifdef __cplusplus
extern "C" {
#endif

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

static void __str_free_cb__(void *entry, void *data) { free(entry); }

static void random_string(char *str, int length)
{
    static const char charset[] =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    int charset_length = strlen(charset);

    for (int i = 0; i < length; i++) {
        int random_index = rand() % charset_length;
        str[i] = charset[random_index];
    }

    str[length] = '\0'; // Null-terminate the string
}

void test_hashmap(void)
{
    srand(time(NULL)); // Seed the random number generator

    HashMap map;
    hashmap_init(&map, __str_cmp_cb__);

    struct str *s;
    int ret;
    struct str *s2;

    for (int i = 0; i < 20000; i++) {
        s = malloc(sizeof(struct str) + 21);
        random_string((char *)(s + 1), 20);
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
    test_hashmap();
    // printf("hashmap test done\n");
    return 0;
}

#ifdef __cplusplus
}
#endif
