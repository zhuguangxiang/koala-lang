/*----------------------------------------------------------------------------*\
|* This file is part of the koala project, under the MIT License.             *|
|* Copyright (c) 2021-2021 James <zhuguangxiang@gmail.com>                    *|
\*----------------------------------------------------------------------------*/

#ifndef _KOALA_KLVM_PASSES_H_
#define _KOALA_KLVM_PASSES_H_

#include "klvm.h"

#ifdef __cplusplus
extern "C" {
#endif

void dot_pass_register(klvm_module_t *m);
void block_pass_register(klvm_module_t *m);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_KLVM_PASSES_H_ */
