/*===-- fieldobject.h - Field Object ------------------------------*- C -*-===*\
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) 2020 James, https://github.com/zhuguangxiang                 *|
|*                                                                            *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* This header declares the Koala 'Field' object.                             *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_FIELD_OBJECT_H_
#define _KOALA_FIELD_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

/* `Field` object layout */
typedef struct FieldObject {
    OBJECT_HEAD
    /* field name */
    const char *name;
    /* offset of value */
    int offset;
} FieldObject;

Object *field_new(const char *name);
void field_set_offset(Object *field, int offset);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_FIELD_OBJECT_H_ */
