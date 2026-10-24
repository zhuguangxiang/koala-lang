/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "collections/bitsetobj.h"
#include "collections/dequeobj.h"
#include "collections/linkedlistobj.h"
#include "collections/queueobj.h"
#include "collections/stackobj.h"

#ifdef __cplusplus
extern "C" {
#endif

void collections_native_lib_init(NativeLib *lib)
{
    kl_reg_type(lib, &bitset_type);
    kl_reg_type(lib, &stack_type);
    kl_reg_type(lib, &queue_type);
    kl_reg_type(lib, &linkedlist_type);
    kl_reg_type(lib, &linkedlist_iter_type);
    kl_reg_type(lib, &linkedlist_rev_iter_type);
    kl_reg_type(lib, &deque_type);
    kl_reg_type(lib, &deque_iter_type);
    kl_reg_type(lib, &deque_rev_iter_type);
}

#ifdef __cplusplus
}
#endif
