/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangXiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "koala.h"

int main(int argc, char *argv[])
{
    /* init koala */
    koala_init();

    koala_run_file(argv[1]);

    /* fini koala */
    koala_fini();
    return 0;
}
