/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "koala.h"

int main(int argc, char *argv[])
{
    koala_initialize();
    koala_run_file(argv[1]);
    koala_finalize();
    return 0;
}
