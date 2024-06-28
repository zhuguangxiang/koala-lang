/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "log.h"
#include "run.h"
#include "tupleobject.h"

#ifdef __cplusplus
extern "C" {
#endif

void test_tuple(void) {}

int main(int argc, char *argv[])
{
    init_log(LOG_INFO, NULL, 0);
    kl_init(argc, argv);
    test_tuple();
    kl_fini();
    return 0;
}

#ifdef __cplusplus
}
#endif
