/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */
#ifndef _KOALA_KLC_H_
#define _KOALA_KLC_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _KLCFile {
    FILE *filp;
    Vector objs;
} KLCFile;

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_KLC_H_ */
