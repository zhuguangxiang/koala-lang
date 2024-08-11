/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_UTF8_H_
#define _KOALA_UTF8_H_

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

// return count of utf8 characters, or -1 if not valid.
int check_utf8(void *str, int len);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_UTF8_H_ */
