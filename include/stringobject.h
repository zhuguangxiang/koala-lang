/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_STRING_OBJECT_H_
#define _KOALA_STRING_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _StringObject {
    OBJECT_HEAD
    /* The slice [start ..< end] */
    int start;
    int end;
    /* The utf8 length, 0 ascii string, others utf8 string */
    int utf8_length;
    /* The ascii length */
    int length;
    /*
     * If the buf points to the end of this structure, the string belongs to
     * itself; otherwise, this string is another string's slice. The char type
     * is easy to use.
     */
    char *buf;
    /*
     * The real string ascii(utf8) data, if it exists. The string is
     * null-terminated string, this is compatible with c-string.
     */
    char data[0];
} StringObject;

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_STRING_OBJECT_H_ */