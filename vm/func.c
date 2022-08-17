/*
 * This file is part of the koala-lang project, under the MIT License.
 *
 * Copyright (c) 2022-2032 James <zhuguangxiang@gmail.com>
 */

#include "func.h"

#ifdef __cplusplus
extern "C" {
#endif

void kl_func_update_stacksize(KlFunc *func)
{
    int size = 0;
    KlLocal *item;
    // vector_foreach(item, &func->locals, { size += item->size; });
    func->stack_size = size;
}

#ifdef __cplusplus
}
#endif
