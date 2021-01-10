/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangxiang https://github.com/zhuguangxiang               *|
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
    /* length */
    int nbytes;
    /* utf8 string */
    const char *s;
} StringObject;

/* initialize `String` type */
void init_string_type(void);

/* new `String` object */
Object *string_new(const char *s);

/* get c string(char *) */
const char *string_tocstr(Object *obj);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_STRING_OBJECT_H_ */
