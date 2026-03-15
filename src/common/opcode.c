/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "opcode.h"

#ifdef __cplusplus
extern "C" {
#endif

char *opcode_names[] = {
#define X(name, fmt, cmt) #name,
#include "opcode_list.h"
#undef X
};

int opcode_formats[] = {
#define X(name, fmt, cmt) fmt,
#include "opcode_list.h"
#undef X
};

#ifdef __cplusplus
}
#endif
