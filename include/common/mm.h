/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2023 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_MEM_H_
#define _KOALA_MEM_H_

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The memory is set to zero. */
void *mm_alloc(int size);

/* The faster memory allocator, not set zero. */
void *mm_alloc_fast(int size);

/* Allocate object by its pointer. */
#define mm_alloc_obj(ptr) mm_alloc(OBJ_SIZE(ptr))

/* Allocate object faster by its pointer, not set zero. */
#define mm_alloc_obj_fast(ptr) mm_alloc_fast(OBJ_SIZE(ptr))

/* The func frees the memory space pointed by pointer. */
void mm_free(void *ptr);

/* Stat usage memory */
void mm_stat(void);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_MEM_H_ */
