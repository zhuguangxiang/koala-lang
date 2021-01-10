/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangxiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_ARRAY_OBJECT_H_
#define _KOALA_ARRAY_OBJECT_H_

#include "gc.h"
#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

/* `Array` object layout. */
typedef struct ArrayObject {
    /* object */
    OBJECT_HEAD
    /* start in GcArray */
    int offset;
    /* array length */
    int len;
    /* GcArray */
    GcArray *arr;
} ArrayObject;

Object *array_new(int objsize);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_ARRAY_OBJECT_H_ */
