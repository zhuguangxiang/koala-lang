/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangXiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_CFUNC_H_
#define _KOALA_CFUNC_H_

#include "object.h"
#include "typedesc.h"
#include <ffi.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cfunc {
    ffi_cif cif;
    void *ptr;
    ffi_type *rtype;
    ffi_type *ptypes[0];
} cfunc_t;

/* new ffi c function */
cfunc_t *kl_new_cfunc(TypeDesc *proto, void *ptr);

/* call c function with ffi */
void kl_call_cfunc(cfunc_t *cf, TValueRef *args, int narg, intptr_t *ret);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_CFUNC_H_ */
