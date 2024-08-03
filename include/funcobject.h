/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_FUNC_OBJECT_H_
#define _KOALA_FUNC_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

/* This represents koala function
 * CFuncObject represents c-defined function
 */
typedef struct _FuncObject {
    OBJECT_HEAD
    /* code object, if code is null, this is an abstract method. */
    Object *code;
    /* module */
    Object *mod;
    /* class, can be null */
    TypeObject *cls;
} FuncObject;

extern TypeObject func_type;
#define IS_FUNC(ob) IS_TYPE((ob), &func_type)

Object *kl_new_func(Object *code, Object *mod, TypeObject *cls);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_FUNC_OBJECT_H_ */
