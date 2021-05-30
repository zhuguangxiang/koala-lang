/*
 * This file is part of the koala-lang project, under the MIT License.
 *
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
 */

#include <stdlib.h>
#include "util/readline.h"

#ifdef __cplusplus
extern "C" {
#endif

void test_readline(void)
{
    init_readline();
    char buf[128];
    while (readline("> ", buf, 127))
        ;
    fini_readline();
}

int main(int argc, char *argv[])
{
    test_readline();
    return 0;
}

#ifdef __cplusplus
}
#endif
