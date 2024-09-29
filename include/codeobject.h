/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_CODE_OBJECT_H_
#define _KOALA_CODE_OBJECT_H_

#include "codespec.h"
#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _CodeObject {
    FUNCTION_HEAD
    /* code spec from klc */
    CodeSpec cs;
} CodeObject;

extern TypeObject code_type;
#define IS_CODE(ob) IS_TYPE((ob), &code_type)

Object *kl_new_code(char *name, Object *m, TypeObject *cls);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_CODE_OBJECT_H_ */
