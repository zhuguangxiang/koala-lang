/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "atom.h"
#include "log.h"
#include "parser.h"

int main(int argc, char *argv[])
{
    init_atom();
    init_log(LOG_INFO, NULL, 0);
    compile(argc, argv);
    fini_log();
    fini_atom();
    return 0;
}
