/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_STR_OBJECT_H_
#define _KOALA_STR_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _StrObject {
    OBJECT_HEAD
    int start;
    int end;
    char *array;
} StrObject;

extern TypeObject str_type;
#define IS_STR(ob) IS_TYPE((ob), &str_type)

Object *kl_new_nstr(const char *s, int len);

static inline Object *kl_new_str(const char *s)
{
    return kl_new_nstr(s, strlen(s));
}

Object *kl_new_fmt_str(const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_STR_OBJECT_H_ */
