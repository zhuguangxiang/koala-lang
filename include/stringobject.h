/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_STRING_OBJECT_H_
#define _KOALA_STRING_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _StringObject {
    OBJECT_HEAD
    size_t size;
    void *array;
} StringObject;

extern TypeObject str_type;
#define IS_STR(ob) IS_TYPE((ob), &str_type)

#define STR_BUF(ob) (char *)(((StringObject *)(ob))->array)
#define STR_LEN(ob) (((StringObject *)(ob))->size)

Object *kl_new_nstr(char *s, size_t len);
static inline Object *kl_new_str(char *s) { return kl_new_nstr(s, strlen(s)); }
Object *kl_new_fmt_str(char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_STRING_OBJECT_H_ */
