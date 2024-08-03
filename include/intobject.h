/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_INT_OBJECT_H_
#define _KOALA_INT_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

Object *kl_new_nstr(const char *s, int len);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_INT_OBJECT_H_ */
