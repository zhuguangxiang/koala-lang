/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_ISEL_H_
#define _KOALA_ISEL_H_

#include "ir.h"

#ifdef __cplusplus
extern "C" {
#endif

void isel_lower_binary(KlrInsn *insn, KlrFunc *fn);
void isel_lower_local(KlrInsn *insn, KlrFunc *fn);
void klr_module_do_isel(KlrModule *m);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_ISEL_H_ */
