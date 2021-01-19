/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangXiang https://github.com/zhuguangxiang               *|
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
