/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_STACK_OBJECT_H_
#define _KOALA_STACK_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _StackObject {
    OBJECT_HEAD
    /* Elements are `TValue`; the top of the stack is the vector's back. */
    Vector vec;
} StackObject;

extern TypeObject stack_type;
#define IS_STACK(ob) IS_TYPE((ob), &stack_type)

Object *kl_new_stack(void);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_STACK_OBJECT_H_ */