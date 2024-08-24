/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "eval.h"
#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

extern TypeObject int_type;

void init_bltin_module(KoalaState *ks) { init_types(); }

void init_sys_module(KoalaState *ks) { init_types(); }

#ifdef __cplusplus
}
#endif
