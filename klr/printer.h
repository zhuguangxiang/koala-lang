/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KLVM_PRINTER_H_
#define _KLVM_PRINTER_H_

#if !defined(_KLVM_H_)
#error "Only <klvm/klvm.h> can be included directly."
#endif

/* no error here */
#include "klvm.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Print instruction */
void klvm_print_inst(KLVMFunc *fn, KLVMInst *inst, FILE *fp);

/* Print liveness */
void klvm_print_liveness(KLVMFunc *fn, FILE *fp);

/* Print a module */
void klvm_print_module(KLVMModule *m, FILE *fp);

/* Dump a module */
/* clang-format off */
#define klvm_dump_module(m) klvm_print_module(m, stdout); fflush(stdout)
/* clang-format on */

#ifdef __cplusplus
}
#endif

#endif /* _KLVM_PRINTER_H_ */
