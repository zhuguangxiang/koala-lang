/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_OBJECT_H_
#define _KOALA_OBJECT_H_

#include "common.h"
#include "gc.h"
#ifdef __cplusplus
extern "C" {
#endif

/* clang-format off */
#define OBJECT_HEAD GcHdr ob_gchdr; struct _TypeObject *ob_type;
/* clang-format on */

typedef struct _Object {
    OBJECT_HEAD
} Object;

#define OBJECT_HEAD_INIT(_type) \
    GC_HEAD_INIT(ob_gchdr, NULL, 0, -1, GC_COLOR_BLACK), .ob_type = (_type),

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
    |  1001 |  obj         |  invalid  |
    +-------+--------------+-----------+
    |  1111 |  obj         |  valid    |
    +-------+--------------+-----------+
*/
#define VAL_TAG_I32 0b0000
#define VAL_TAG_I64 0b0010
#define VAL_TAG_F32 0b0100
#define VAL_TAG_F64 0b0110
#define VAL_TAG_OBJ 0b1001
#define VAL_TAG_VTL 0b1111

typedef struct _TypeObject {
    OBJECT_HEAD
    const char *tp_name;

} TypeObject;

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_OBJECT_H_ */
