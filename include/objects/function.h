/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_FUNCTION_H_
#define _KOALA_FUNCTION_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _CFuncObject {
    OBJECT_HEAD
    Object *owner;
    char *name;
    NativeFunc func;
} CFuncObject;

typedef struct _CodeObject {
    OBJECT_HEAD
    Object *owner;
    CodeSpec cs;
} CodeObject;

extern TypeObject cfunc_type;
extern TypeObject code_type;

#define IS_CFUNC(ob) IS_TYPE((ob), &cfunc_type)
#define IS_CODE(ob)  IS_TYPE((ob), &code_type)

Object *kl_new_code(char *name, Object *owner);
Object *kl_new_cfunc(char *name, NativeFunc fn, Object *owner);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_FUNCTION_H_ */
