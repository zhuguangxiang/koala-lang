/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "collections/bitsetobj.h"
#include "collections/stackobj.h"

#ifdef __cplusplus
extern "C" {
#endif

void collections_native_lib_init(NativeLib *lib)
{
    kl_reg_type(lib, &bitset_type);
    kl_reg_type(lib, &stack_type);
}

#ifdef __cplusplus
}
#endif
