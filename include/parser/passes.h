/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_PASSES_H_
#define _KOALA_PASSES_H_

#include "ir.h"

#ifdef __cplusplus
extern "C" {
#endif

void register_dot_passes(KlrPassGroup *grp);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_PASSES_H_ */
