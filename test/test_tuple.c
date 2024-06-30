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

void test_tuple(void)
{
    Object *x = kl_new_tuple(3);
    ASSERT(TUPLE_SIZE(x) == 3);

    Value *val = TUPLE_ITEMS(x);

    for (int i = 0; i < TUPLE_SIZE(x); i++) {
        ASSERT(IS_NONE(val + i));
        val[i] = Int32Value(100 + i);
    }

    for (int i = 0; i < TUPLE_SIZE(x); i++) {
        ASSERT(IS_INT32(val + i));
    }
}

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
