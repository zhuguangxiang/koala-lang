/*
 * This file is part of the koala project, under the MIT License.
 * Copyright (c) 2020-2021 James <zhuguangxiang@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
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

static void __str_free_cb__(void *entry, void *data)
{
    free(entry);
}

static void random_string(char *data, int len)
{
    static const char char_set[] =
        "0123456789abcdefghijklmnopqrstuvwxyz"
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
    HashMapInit(&map, __str_cmp_cb__);

    struct str *s;
    int ret;
    struct str *s2;

    for (int i = 0; i < 10000; i++) {
        s = malloc(sizeof(struct str) + 11);
        random_string((char *)(s + 1), 10);
        HashMapEntryInit(s, StrHash((char *)(s + 1)));
        ret = HashMapPutAbsent(&map, s);
        assert(!ret);
        s2 = HashMapGet(&map, s);
        assert(__str_cmp_cb__(s2, s));
    }

    HashMapFini(&map, __str_free_cb__, &map);
}

int main(int argc, char *argv[])
{
    test_hashmap();
    return 0;
}

#ifdef __cplusplus
}
#endif
