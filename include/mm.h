/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangXiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_MM_H_
#define _KOALA_MM_H_

#ifdef __cplusplus
extern "C" {
#endif

/* The memory is set to zero. */
void *mm_alloc(int size);

/* The func frees the memory space pointed by ptr. */
void mm_free(void *ptr);

/* Stat usage memory */
void mm_stat(void);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_MM_H_ */
