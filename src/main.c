/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "log.h"
#include "run.h"

int main(int argc, char *argv[])
{
    init_log(LOG_INFO, NULL, 0);
    kl_init(argc, argv);
    kl_run_file(argv[1]);
    kl_fini();
    return 0;
}
