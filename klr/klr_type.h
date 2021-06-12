/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_IR_TYPE_H_
#define _KOALA_IR_TYPE_H_

#if !defined(_KLR_H_INSIDE_) && !defined(KLR_COMPILE)
#error "Only <klr/klr.h> can be included directly."
#endif

#include "util/vector.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _KlrTypeKind {
    KLR_TYPE_NONE,
    KLR_TYPE_INT8,
    KLR_TYPE_INT16,
    KLR_TYPE_INT32,
    KLR_TYPE_INT64,
    KLR_TYPE_FLOAT32,
    KLR_TYPE_FLOAT64,
    KLR_TYPE_BOOL,
    KLR_TYPE_CHAR,
    KLR_TYPE_STR,
    KLR_TYPE_ANY,
    KLR_TYPE_PARAM,
    KLR_TYPE_ARRAY,
    KLR_TYPE_MAP,
    KLR_TYPE_TUPLE,
    KLR_TYPE_VALIST,
    KLR_TYPE_KLASS,
    KLR_TYPE_PROTO,
    KLR_TYPE_MAX,
} KlrTypeKind;

typedef struct _KlrType {
    KlrTypeKind kind;
} KlrType, *KlrTypeRef;

void klr_init_types(void);
void klr_fini_types(void);

KlrTypeRef klr_type_none(void);
KlrTypeRef klr_type_int8(void);
KlrTypeRef klr_type_int16(void);
KlrTypeRef klr_type_int32(void);
KlrTypeRef klr_type_int64(void);
KlrTypeRef klr_type_float32(void);
KlrTypeRef klr_type_float64(void);
KlrTypeRef klr_type_bool(void);
KlrTypeRef klr_type_char(void);
KlrTypeRef klr_type_str(void);
KlrTypeRef klr_type_any(void);

KlrTypeRef klr_type_klass(char *path, char *name);
char *klr_get_path(KlrTypeRef ty);
char *klr_get_name(KlrTypeRef ty);

KlrTypeRef klr_type_proto(KlrTypeRef ret, Vector *params);
Vector *klr_get_params(KlrTypeRef ty);
KlrTypeRef klr_get_ret(KlrTypeRef ty);

int klr_type_equal(KlrTypeRef ty1, KlrTypeRef ty2);

char *klr_type_tostr(KlrTypeRef ty);
KlrTypeRef klr_type_from_str(char *str);
KlrTypeRef klr_proto_from_str(char *params, char *ret);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_IR_TYPE_H_ */
