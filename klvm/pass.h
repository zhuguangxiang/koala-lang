/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KLVM_PASS_H_
#define _KLVM_PASS_H_

#if !defined(_KLVM_H_)
#error "Only <klvm/klvm.h> can be included directly."
#endif

/* no error here */
#include "klvm.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _KLVMPassGroup KLVMPassGroup;

struct _KLVMPassGroup {
    List list;
};

typedef void (*KLVMPassFunc)(KLVMFunc *fn, void *arg);

/* Initialize passes */
void klvm_init_passes(KLVMPassGroup *grp);

/* Finalize passes */
void klvm_fini_passes(KLVMPassGroup *grp);

/* Register pass */
void klvm_register_pass(KLVMPassGroup *grp, KLVMPassFunc fn, void *arg);

/* Execute passes */
void klvm_run_passes(KLVMPassGroup *grp, KLVMModule *m);

/* Add check unreachable block pass */
void klvm_add_unreachblock_pass(KLVMPassGroup *grp);

/* Add gen graphviz dot file and pdf pass */
void klvm_add_dot_pass(KLVMPassGroup *grp);

/* Add check unused variable pass */
void klvm_add_check_unused_pass(KLVMPassGroup *grp);

#ifdef __cplusplus
}
#endif

#endif /* _KLVM_PASS_H_ */
