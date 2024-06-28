/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_ABSTRACT_H_
#define _KOALA_ABSTRACT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

Value object_call(Value *self, Value *args, int nargs, Object *kwargs);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_ABSTRACT_H_ */
