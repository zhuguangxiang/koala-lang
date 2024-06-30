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

typedef struct _AbstractMethodObject {
    OBJECT_HEAD
    /* abstract method name */
    char *name;
    /* arguments types */
    char *args_type;
    /* return type */
    char *ret_type;
} AbstractMethodObject;

extern TypeObject abstract_method_type;
#define IS_ABSTRACT(ob) IS_TYPE((ob), &abstract_method_type)

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_FUNC_OBJECT_H_ */
