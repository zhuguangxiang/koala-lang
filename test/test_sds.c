/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include <stdio.h>
#include <string.h>
#include "sds.h"

#ifdef __cplusplus
extern "C" {
#endif

int main(int argc, char *argv[])
{
    sds s1 = sdsnew("abcd");
    sds s2 = sdsnew("abcd");
    printf("%s, %p, %p\n", s1, s1, s2);
    return 0;
}

#ifdef __cplusplus
}
#endif
