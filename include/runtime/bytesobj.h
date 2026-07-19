/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_BYTES_OBJECT_H_
#define _KOALA_BYTES_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _BytesObject {
    OBJECT_HEAD
    uint32_t offset;
    uint32_t size;
    uint8_t *data;
} BytesObject;

extern TypeObject bytes_type;
#define IS_BYTES(ob) IS_TYPE((ob), &bytes_type)
Object *kl_new_bytes(uint32_t size);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_BYTES_OBJECT_H_ */
