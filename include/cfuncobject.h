/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_CFUNC_OBJECT_H_
#define _KOALA_CFUNC_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _CFuncObject {
    OBJECT_HEAD
    /* cfunc definition */
    MethodDef *def;
    /* module */
    Object *module;
    /* class, can be null */
    TypeObject *cls;
} CFuncObject;

extern TypeObject cfunc_type;
#define IS_CFUNC(ob) IS_TYPE((ob), &cfunc_type)

Object *kl_new_cfunc(MethodDef *def, Object *m, TypeObject *cls);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_CFUNC_OBJECT_H_ */
