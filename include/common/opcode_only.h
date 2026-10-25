/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_OPCODE_ONLY_H_
#define _KOALA_OPCODE_ONLY_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _OpCode {
#define X(name, fmt, str) name,
#include "opcode_list.h"
#undef X
} OpCode;

typedef enum _InternTag {
    INTERN_TUPLE,
    INTERN_RANGE,
    INTERN_SLICE,
    INTERN_LIST,
    /* map literal: operands are key-value pairs in source order,
     * i.e. k0, v0, k1, v1, ... and nargs is 2 * number-of-entries */
    INTERN_MAP,
} InternTag;

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_OPCODE_ONLY_H_ */
