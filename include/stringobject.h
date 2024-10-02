/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_STRING_OBJECT_H_
#define _KOALA_STRING_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _StrObject {
    OBJECT_HEAD
    int start;
    int stop;
    GcArrayObject *array;
} StrObject;

extern TypeObject str_type;
#define IS_STR(ob) IS_TYPE((ob), &str_type)

#define STR_BUF(ob) (const char *)(((StrObject *)(ob))->array + 1)
#define STR_LEN(ob) (((StrObject *)(ob))->stop - ((StrObject *)(ob))->start)

#define IS_STR_OBJ(v) (IS_OBJECT(v) ? IS_STR(value_object(v)) : 0)

Object *kl_new_nstr(const char *s, int len);
static inline Object *kl_new_str(const char *s) { return kl_new_nstr(s, strlen(s)); }
Object *kl_new_fmt_str(const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_STRING_OBJECT_H_ */
