/*----------------------------------------------------------------------------*\
|* This file is part of the koala project, under the MIT License.             *|
|* Copyright (c) 2021-2021 James <zhuguangxiang@gmail.com>                    *|
\*----------------------------------------------------------------------------*/

#include "klvm.h"

#ifdef __cplusplus
extern "C" {
#endif

static void dot_pass(klvm_func_t *fn, void *data)
{
    if (!strmcp(fn->name, "__init__")) return;
}

void dot_pass_register(klvm_module_t *m)
{
    klvm_pass_t pass = { dot_pass, NULL };
    klvm_register_pass(m, &pass);
}

#ifdef __cplusplus
}
#endif
