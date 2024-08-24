/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "klc.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TYPE_NONE    'N'
#define TYPE_FALSE   'F'
#define TYPE_TRUE    'T'
#define TYPE_INT     'i'
#define TYPE_FLOAT   'f'
#define TYPE_ASCII   'A'
#define TYPE_UNICODE 'U'
#define TYPE_TUPLE   '('
#define TYPE_LIST    '['
#define TYPE_MAP     '{'
#define TYPE_SET     '<'
#define TYPE_CODE    'c'
#define TYPE_BYTES   'b'
#define TYPE_DESCR   'L'
#define TYPE_CLASS   'C'
#define TYPE_TRAIT   'I'
#define TYPE_VAR     'v'
#define TYPE_PROTO   'p'
#define TYPE_NAME    'n'
#define TYPE_REF     'r'

/* with a type, add obj to index */
#define FLAG_REF '\x80'

#define TYPE_SHORT_ASCII 'a'
#define TYPE_SMALL_TUPLE ')'

#ifdef __cplusplus
}
#endif
