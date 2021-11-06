/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_TYPEDESC_H_
#define _KOALA_TYPEDESC_H_

#include "buffer.h"
#include "log.h"
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _TypeKind TypeKind;
typedef struct _TypeDesc TypeDesc;
typedef struct _ArrayType ArrayType;
typedef struct _MapType MapType;
typedef struct _ProtoType ProtoType;
typedef struct _KlassType KlassType;

enum _TypeKind {
    TYPE_ANY_KIND,
    TYPE_I8_KIND,
    TYPE_I16_KIND,
    TYPE_I32_KIND,
    TYPE_I64_KIND,
    TYPE_F32_KIND,
    TYPE_F64_KIND,
    TYPE_BOOL_KIND,
    TYPE_CHAR_KIND,
    TYPE_STR_KIND,
    TYPE_ARRAY_KIND,
    TYPE_MAP_KIND,
    TYPE_PROTO_KIND,
    TYPE_KLASS_KIND,
    TYPE_MAX_KIND,
};

/* clang-format off */
#define DESC_HEAD TypeKind kind; int refcnt;
/* clang-format on */

struct _TypeDesc {
    DESC_HEAD
};

struct _ArrayType {
    DESC_HEAD
    TypeDesc *ty;
};

struct _MapType {
    DESC_HEAD
    TypeDesc *kty;
    TypeDesc *vty;
};

struct _ProtoType {
    DESC_HEAD
    TypeDesc *ret;
    Vector *params;
};

struct _KlassType {
    DESC_HEAD
    char *path;
    char *name;
    Vector *args;
};

void init_desc(void);
void fini_desc(void);
TypeDesc *desc_from_any(void);
TypeDesc *desc_from_int8(void);
TypeDesc *desc_from_int16(void);
TypeDesc *desc_from_int32(void);
TypeDesc *desc_from_int64(void);
TypeDesc *desc_from_float32(void);
TypeDesc *desc_from_float64(void);
TypeDesc *desc_from_bool(void);
TypeDesc *desc_from_char(void);
TypeDesc *desc_from_str(void);
TypeDesc *desc_from_array(TypeDesc *sub);
TypeDesc *desc_from_map(TypeDesc *kty, TypeDesc *vty);
TypeDesc *desc_from_proto(TypeDesc *ret, Vector *params);
TypeDesc *desc_from_proto2(TypeDesc *ret, TypeDesc *params[], int size);
TypeDesc *desc_from_klass(char *path, char *name, Vector *args);
static inline Vector *proto_params(TypeDesc *ty)
{
    expect(ty->kind == TYPE_PROTO_KIND);
    ProtoType *proto = (ProtoType *)ty;
    return proto->params;
}
static inline TypeDesc *proto_ret(TypeDesc *ty)
{
    expect(ty->kind == TYPE_PROTO_KIND);
    ProtoType *proto = (ProtoType *)ty;
    return proto->ret;
}
int desc_equal(TypeDesc *ty1, TypeDesc *ty2);
TypeDesc *str_to_desc(char *str);
TypeDesc *str_to_proto(char *params, char *ret);
void desc_to_str(TypeDesc *ty, Buffer *buf);
int desc_is_number(TypeDesc *ty);
int desc_is_integer(TypeDesc *ty);
int desc_is_float(TypeDesc *ty);
int desc_is_bool(TypeDesc *ty);
int desc_is_str(TypeDesc *ty);
int desc_is_klass(TypeDesc *ty);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_TYPEDESC_H_ */
