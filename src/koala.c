/*===-- koala.c - Main Of Koala -----------------------------------*- C -*-===*\
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) 2020 James, https://github.com/zhuguangxiang                 *|
|*                                                                            *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* This file is the main of koala                                             *|
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
