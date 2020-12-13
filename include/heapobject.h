/*===-- heapobject.h - Heap Object --------------------------------*- C -*-===*\
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) 2020 James, https://github.com/zhuguangxiang                 *|
|*                                                                            *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* This header declares the Koala `HeapObject` object.                        *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_HEAP_OBJECT_H_
#define _KOALA_HEAP_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

/* `HeapObject` is instance object layout. */
typedef struct HeapObject {
    /* object */
    OBJECT_HEAD
    /* each field value */
    kl_value_t items[0];
} HeapObject;

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_HEAP_OBJECT_H_ */
