/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangxiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "readline.h"
#include <stdlib.h>

void test_readline(void)
{
    init_readline();
    char buf[128];
    while (readline("> ", buf, 127)) { }
    fini_readline();
}

int main(int argc, char *argv[])
{
    test_readline();
    return 0;
}
