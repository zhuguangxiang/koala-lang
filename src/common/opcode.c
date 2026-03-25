/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "opcode.h"

#ifdef __cplusplus
extern "C" {
#endif

char *__op_names[] = {
#include "opcode_list_lowercase.h"
};

OpFormat __op_formats[] = {
#define X(name, fmt) fmt,
#include "opcode_list.h"
#undef X
};

#ifdef __cplusplus
}
#endif
