/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

static char *repeat(char *s, int times)
{
    if (times <= 0) return NULL;

    int len = strlen(s);
    char *result = (char *)malloc(len * times + 1);

    if (result == NULL) return NULL;

    char *ptr = result;
    for (int i = 0; i < times; i++) {
        strcpy(ptr, s);
        ptr += len;
    }

    *ptr = '\0';
    return result;
}

static char *repeat2(char *s, int times)
{
    if (times <= 0) return NULL;

    int len = strlen(s);
    char *result = (char *)malloc(len * times + 1);
    if (result == NULL) return NULL;

    if (times == 1) {
        memcpy(result, s, len);
        return result;
    }

    memcpy(result, s, len);

    char *ptr = result;
    int i;
    for (i = 1; 2 * i < times; i = i * 2) {
        memcpy(ptr + i * len, ptr, i * len);
    }

    memcpy(ptr + i * len, ptr, len * (times - i));

    *(ptr + len * times) = '\0';
    return result;
}

void test_cstr_repeat(void)
{
    // char *r = repeat("abcdef", 1000000);
    // for (int i = 1000000; i <= 1000000; i++) {
    char *r = repeat2("abcdef", 1000000);
    // if (strlen(r) != (size_t)(6 * i)) {
    //     abort();
    // }
    // }
}

int main(int argc, char *argv[])
{
    test_cstr_repeat();
    return 0;
}

#ifdef __cplusplus
}
#endif
