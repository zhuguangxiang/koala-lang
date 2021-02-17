/*----------------------------------------------------------------------------*\
|* This file is part of the koala project, under the MIT License.             *|
|* Copyright (c) 2021-2021 James <zhuguangxiang@gmail.com>                    *|
\*----------------------------------------------------------------------------*/

#include "fib.c"
#include "klvm_dot.h"

#ifdef __cplusplus
extern "C" {
#endif

int main(int argc, char *argv[])
{
    klvm_module_t *m;

    m = klvm_create_module("test");
    test_fib(m);
    klvm_dump_module(m);

    dot_pass_register(m);
    klvm_run_passes(m);

    klvm_destroy_module(m);

    return 0;
}

#ifdef __cplusplus
}
#endif
