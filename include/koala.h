/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_H_
#define _KOALA_H_

#include "excobj.h"
#include "module.h"
#include "opcode.h"
#include "vm.h"

#ifdef __cplusplus
extern "C" {
#endif

void koala_initialize(void);
void koala_run_file(char *path);
void koala_finalize(void);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_H_ */
