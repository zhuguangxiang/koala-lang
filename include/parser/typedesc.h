/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2023 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_TYPEDESC_H_
#define _KOALA_TYPEDESC_H_

#include "buffer.h"
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _DescKind {
    TYPE_UNK_KIND,
    TYPE_INT_KIND,
    TYPE_FLOAT_KIND,
    TYPE_BOOL_KIND,
    TYPE_STR_KIND,
    TYPE_ARRAY_KIND,
    TYPE_MAP_KIND,
    TYPE_TUPLE_KIND,
    TYPE_KLASS_KIND,
    TYPE_PROTO_KIND,
    TYPE_SELF_KIND,
    TYPE_PARAM_KIND,
    TYPE_OBJECT_KIND,
    TYPE_ENUM_KIND,
    TYPE_MAX_KIND,
} DescKind;

typedef struct _TypeDesc {
    DescKind kind;
} TypeDesc;

typedef struct _NumberDesc {
    DescKind kind;
    int width;
} NumberDesc;

extern NumberDesc int8_desc;
extern NumberDesc int16_desc;
extern NumberDesc int32_desc;
extern NumberDesc int64_desc;
extern NumberDesc float32_desc;
extern NumberDesc float64_desc;
extern TypeDesc bool_desc;
extern TypeDesc str_desc;
extern TypeDesc object_desc;

static inline TypeDesc *desc_int8(void) { return (TypeDesc *)&int8_desc; }
static inline TypeDesc *desc_int16(void) { return (TypeDesc *)&int16_desc; }
static inline TypeDesc *desc_int32(void) { return (TypeDesc *)&int32_desc; }
static inline TypeDesc *desc_int64(void) { return (TypeDesc *)&int64_desc; }
static inline TypeDesc *desc_float32(void) { return (TypeDesc *)&float32_desc; }
static inline TypeDesc *desc_float64(void) { return (TypeDesc *)&float64_desc; }
static inline TypeDesc *desc_str(void) { return &str_desc; }
static inline TypeDesc *desc_bool(void) { return &bool_desc; }
static inline TypeDesc *desc_object(void) { return &object_desc; }
TypeDesc *desc_enum(Vector *types);

static inline int desc_is_int(TypeDesc *desc) { return desc->kind == TYPE_INT_KIND; }

int desc_equal(TypeDesc *a, TypeDesc *b);
void desc_to_str(TypeDesc *desc, Buffer *buf);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_TYPEDESC_H_ */
