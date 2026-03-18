/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_PASS_H_
#define _KOALA_PASS_H_

#include "parser.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*PassFunc)(KlrFunc *fn, void *data);

// clang-format off

#define KLR_PASS_HEAD \
    const char *name; PassFunc run; void *data; int dump;

// clang-format on

typedef struct _KlrPass {
    KLR_PASS_HEAD
} KlrPass;

typedef struct _KlrPassManager {
    KLR_PASS_HEAD
    KlrPass **passes;
    int count;
    int capacity;
} KlrPassManager;

void pm_init(KlrPassManager *pm, const char *name);
void pm_add_pass(KlrPassManager *pm, KlrPass *pass, int dump);
void pm_add_pm_as_pass(KlrPassManager *pm, KlrPassManager *pm_pass, int dump);
void pm_fini(KlrPassManager *pm);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_PASS_H_ */
