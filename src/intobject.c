/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "eval.h"
#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

static int _int_str(KoalaState *ks) {}

TypeObject int_type = {
    OBJECT_HEAD_INIT(&type_type), .name = "Int",
    // .hash = int_hash,
    // .cmp = int_cmp,
    // .str = int_str,
    // .init = int_init,
};

#ifdef __cplusplus
}
#endif
