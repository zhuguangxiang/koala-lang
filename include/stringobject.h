/*===-- stringobject.h - String Object ----------------------------*- C -*-===*\
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) 2020 James, https://github.com/zhuguangxiang                 *|
|*                                                                            *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* This header declares the Koala `String` object.                            *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_STRING_OBJECT_H_
#define _KOALA_STRING_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

/* `String` object layout. */
typedef struct StringObject {
    /* object */
    OBJECT_HEAD
    /* string length in bytes */
    int len;
    /* utf8 string(raw string) */
    char s[0];
} StringObject;

/* initialize `String` type */
void init_string_type(void);

/* new `String` object */
Object *string_new(const char *s);
const char *string_tocstr(Object *obj);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_STRING_OBJECT_H_ */
