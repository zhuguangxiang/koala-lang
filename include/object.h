/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_OBJECT_H_
#define _KOALA_OBJECT_H_

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OBJECT_HEAD struct _TypeObject *ob_type;

typedef struct _Object {
    OBJECT_HEAD
} Object;

typedef struct _Value {
    /* value */
    union {
        Object *obj;
        int64_t ival;
        double fval;
    };
    /* type */
    union {
        /* virtual table */
        Object *vtbl;
        uintptr_t tag : 4;
    };
} Value;

/*
    +-------+--------------+-----------+
    |  tag  |   value      |    vtbl   |
    +-------+--------------+-----------+
    |  0000 |  int32       |  invalid  |
    +-------+--------------+-----------+
    |  0010 |  int64       |  invalid  |
    +-------+--------------+-----------+
    |  0100 |  float32     |  invalid  |
    +-------+--------------+-----------+
    |  0110 |  float54     |  invalid  |
    +-------+--------------+-----------+
    |  1000 |  obj(no gc)  |  invalid  |
    +-------+--------------+-----------+
    |  1001 |  obj(gc)     |  invalid  |
    +-------+--------------+-----------+
    |  1110 |  obj(no gc)  |  valid    |
    +-------+--------------+-----------+
    |  1111 |  obj(gc)     |  valid    |
    +-------+--------------+-----------+
*/
#define VAL_TAG_I32    0b0000
#define VAL_TAG_I64    0b0010
#define VAL_TAG_F32    0b0100
#define VAL_TAG_F64    0b0110
#define VAL_TAG_OBJ    0b1000
#define VAL_TAG_GC_OBJ 0b1001
#define VAL_TAG_VTL    0b1110
#define VAL_TAG_GC_VTL 0b1111

typedef struct _TypeObject {
    OBJECT_HEAD
    const char *tp_name;

} TypeObject;

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_OBJECT_H_ */
