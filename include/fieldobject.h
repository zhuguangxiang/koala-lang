/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_FIELD_OBJECT_H_
#define _KOALA_FIELD_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _VarInfo {
    /* name */
    const char *name;
    /* type descriptor */
    const char *desc;
    /* flags */
    int flags;
    /* index of koala object layout */
    int index;
} VarInfo;

typedef struct _ValueInfo {
    /* name */
    const char *name;
    /* type descriptor */
    const char *desc;
    /* flags */
    int flags;
    /* value */
    Value value;
} ValueInfo;

typedef struct _FieldObject {
    OBJECT_HEAD
    /* which kind */
    int kind;
#define FIELD_KIND_GETSET 1
#define FIELD_KIND_MEMBER 2
#define FIELD_KIND_VAR    3
#define FIELD_KIND_VALUE  4
    /* below real data */
    char data[0];
} FieldObject;

Object *kl_field_from_member(MemberDef *member);
Object *kl_field_from_getset(GetSetDef *getset);
Object *kl_field_from_var(VarInfo *var);
Object *kl_field_from_value(ValueInfo *val);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_FIELD_OBJECT_H_ */
