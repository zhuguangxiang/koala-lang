/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "log.h"
#include "run.h"

int main(int argc, const char *argv[])
{
    init_log(LOG_DEBUG, "log.txt", 0);
    kl_init(argc, argv);
    kl_run(argv[1]);
    kl_fini();
    return 0;
}
