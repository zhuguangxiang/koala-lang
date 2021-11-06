/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "klvm.h"

#ifdef __cplusplus
extern "C" {
#endif

KLVMInterval *interval_alloc(KLVMValDef *owner)
{
    KLVMInterval *interval = mm_alloc_obj(interval);
    interval->owner = owner;
    init_list(&interval->range_list);
    init_list(&interval->use_list);
    init_list(&interval->link);
    return interval;
}

#ifdef __cplusplus
}
#endif
