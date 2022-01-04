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

extern initfunc __start__init_0[];
extern initfunc __stop__init_0[];

extern initfunc __start__init_1[];
extern initfunc __stop__init_1[];

extern initfunc __start__init_2[];
extern initfunc __stop__init_2[];

void init_builtins(void)
{
    initfunc *fn;

    fn = __start__init_0;
    while (fn < __stop__init_0) {
        (*fn)();
        fn++;
    }

    fn = __start__init_1;
    while (fn < __stop__init_1) {
        (*fn)();
        fn++;
    }

    fn = __start__init_2;
    while (fn < __stop__init_2) {
        (*fn)();
        fn++;
    }
}

void kl_init(void)
{
    init_atom();
    init_desc();
    init_builtins();
}

void kl_fini(void)
{
    fini_desc();
    fini_atom();
}

#ifdef __cplusplus
}
#endif
