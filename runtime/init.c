/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "typedesc.h"
#include "util/atom.h"
#include "util/common.h"

#ifdef __cplusplus
extern "C" {
#endif

static void init_empty_2(void)
{
}

INIT_FUNC_LEVEL_2(init_empty_2);

extern initfunc __start__init_func_0[];
extern initfunc __stop__init_func_0[];

extern initfunc __start__init_func_1[];
extern initfunc __stop__init_func_1[];

extern initfunc __start__init_func_2[];
extern initfunc __stop__init_func_2[];

void init_builtin_types(void)
{
    initfunc *fn;

    fn = __start__init_func_0;
    while (fn < __stop__init_func_0) {
        (*fn)();
        fn++;
    }

    fn = __start__init_func_1;
    while (fn < __stop__init_func_1) {
        (*fn)();
        fn++;
    }

    fn = __start__init_func_2;
    while (fn < __stop__init_func_2) {
        (*fn)();
        fn++;
    }
}

void kl_init(void)
{
    init_atom();
    init_desc();
    init_builtin_types();
}

void kl_fini(void)
{
    fini_desc();
    fini_atom();
}

#ifdef __cplusplus
}
#endif
