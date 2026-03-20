/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "opcode.h"

#ifdef __cplusplus
extern "C" {
#endif

char *opcode_names[] = {
#define X(name, fmt, s0, s1) #name,
#include "opcode_list.h"
#undef X
};

OpFormat opcode_formats[] = {
#define X(name, fmt, s0, s1) fmt,
#include "opcode_list.h"
#undef X
};

#ifdef __cplusplus
}
#endif
