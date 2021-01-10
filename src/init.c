/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangxiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "gc.h"
#include "module.h"
#include "task.h"

void kl_init(void)
{
    /* init coroutine */
    init_procs(1);

    /* init strtab */

    /* init gc */
    gc_init();

    /* init core modules */
    init_core_modules();
}

void kl_fini(void)
{
    /* fini modules */
    fini_modules();

    /* fini gc */
    gc_fini();

    /* fini coroutine */
    fini_procs();
}
