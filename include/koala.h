/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_API_H_
#define _KOALA_API_H_

#include "version.h"
#include "debug.h"

#ifdef __cplusplus
extern "C" {
#endif

void koala_initialize(void);
void koala_destroy(void);
void koala_active(void);
void koala_compile(char *path);
void koala_run(char *path);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_API_H_ */
