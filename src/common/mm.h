/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2018-2022 James <zhuguangxiang@gmail.com>
 */

#ifndef _KOALA_MM_H_
#define _KOALA_MM_H_

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

// The memory is set to zero.
void *mm_alloc(int size);

// Allocate object by its pointer.
#define mm_alloc_obj(ptr) mm_alloc(OBJ_SIZE(ptr))

// The func frees the memory space pointed by pointer.
void mm_free(void *ptr);

// Stat usage memory
void mm_stat(void);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_MM_H_ */
