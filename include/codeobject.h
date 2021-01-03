/*===-- codeobject.h - Code Object --------------------------------*- C -*-===*\
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) 2020 James, https://github.com/zhuguangxiang                 *|
|*                                                                            *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* This header declares the Koala `Code` object.                              *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_CODE_OBJECT_H_
#define _KOALA_CODE_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

/* `Code` object layout */
typedef struct CodeObject {
    OBJECT_HEAD
    int size;
    uint8_t *codes;
} CodeObject;

/* initialize `Code` type */
void init_code_type(void);

/* new code object */
Object *code_new(uint8_t *codes, int size);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_CODE_OBJECT_H_ */
