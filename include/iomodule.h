/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_IO_MODULE_H_
#define _KOALA_IO_MODULE_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

void init_io_module(void);
void io_put(Object *ob);
void io_putln(Object *ob);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_FMT_MODULE_H_ */
