/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_JIT_H_
#define _KOALA_JIT_H_

#include "codeobject.h"
#include <libgccjit.h>
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _KoalaJit {
    CodeObject *code;
    gcc_jit_context *ctx;
    Vector leaders;
    Vector blocks;
} KoalaJit;

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_JIT_H_ */
