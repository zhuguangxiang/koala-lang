/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_MM_H_
#define _KOALA_MM_H_

#ifdef __cplusplus
extern "C" {
#endif

/* The memory is set to zero. */
void *mm_alloc(int size);

/* Allocate by ptr */
#define mm_alloc_obj(ptr) mm_alloc(OBJ_SIZE(ptr))

/* The func frees the memory space pointed by ptr. */
void mm_free(void *ptr);

/* Stat usage memory */
void mm_stat(void);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_MM_H_ */
