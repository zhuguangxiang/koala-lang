/*===-- koala.h - Koala Language C Interfaces ---------------------*- C -*-===*\
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) 2020 James, https://github.com/zhuguangxiang                 *|
|*                                                                            *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* This header declares koala c interfaces.                                   *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_H_
#define _KOALA_H_

#include "vm.h"

#ifdef __cplusplus
extern "C" {
#endif

/* init koala */
void kl_init(void);

/* fini koala */
void kl_fini(void);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_H_ */
