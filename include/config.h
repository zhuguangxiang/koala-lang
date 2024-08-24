/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2023 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_CONFIG_H_
#define _KOALA_CONFIG_H_

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _KoalaConfig {
    int num_threads;
    int max_gc_mem_size;
} KoalaConfig;

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_CONFIG_H_ */
