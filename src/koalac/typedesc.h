/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2018-2022 James <zhuguangxiang@gmail.com>
 */

#ifndef _KOALA_TYPE_DESC_H_
#define _KOALA_TYPE_DESC_H_

#include "common/buffer.h"
#include "common/common.h"
#include "common/vector.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _TypeKind TypeKind;
typedef struct _TypeDesc TypeDesc;
typedef struct _PointerType PointerType;
typedef struct _ArrayType ArrayType;
typedef struct _MapType MapType;
typedef struct _ProtoType ProtoType;
typedef struct _KlassType KlassType;

enum _TypeKind {
    TYPE_UNK_KIND,
    TYPE_ANY_KIND,
    TYPE_U8_KIND,
    TYPE_U16_KIND,
    TYPE_U32_KIND,
    TYPE_U64_KIND,
    TYPE_I8_KIND,
    TYPE_I16_KIND,
    TYPE_I32_KIND,
    TYPE_I64_KIND,
    TYPE_F32_KIND,
    TYPE_F64_KIND,
    TYPE_BOOL_KIND,
    TYPE_CHAR_KIND,
    TYPE_STR_KIND,
    TYPE_PTR_KIND,
    TYPE_ARRAY_KIND,
    TYPE_MAP_KIND,
    TYPE_PROTO_KIND,
    TYPE_KLASS_KIND,
    TYPE_MAX_KIND,
};

/* clang-format off */
#define DESC_HEAD TypeKind kind;
/* clang-format on */

struct _TypeDesc {
    DESC_HEAD
};

struct _PointerType {
    DESC_HEAD
    TypeDesc *ty;
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
    char *pkg;
    char *name;
    Vector *type_params;
};

TypeDesc *desc_from_unk(void);
TypeDesc *desc_from_any(void);
TypeDesc *desc_from_uint8(void);
TypeDesc *desc_from_uint16(void);
TypeDesc *desc_from_uint32(void);
TypeDesc *desc_from_uint64(void);
TypeDesc *desc_from_int8(void);
TypeDesc *desc_from_int16(void);
TypeDesc *desc_from_int32(void);
TypeDesc *desc_from_int64(void);
TypeDesc *desc_from_float32(void);
TypeDesc *desc_from_float64(void);
TypeDesc *desc_from_bool(void);
TypeDesc *desc_from_char(void);
TypeDesc *desc_from_str(void);
TypeDesc *desc_from_ptr(TypeDesc *ty);
TypeDesc *desc_from_array(TypeDesc *ty);
TypeDesc *desc_from_map(TypeDesc *kty, TypeDesc *vty);
TypeDesc *desc_from_proto(TypeDesc *ret, Vector *params);
TypeDesc *desc_from_klass(char *pkg, char *name, Vector *type_params);
static inline Vector *proto_params(TypeDesc *ty)
{
    assert(ty->kind == TYPE_PROTO_KIND);
    ProtoType *proto = (ProtoType *)ty;
    return proto->params;
}
static inline TypeDesc *proto_ret(TypeDesc *ty)
{
    assert(ty->kind == TYPE_PROTO_KIND);
    ProtoType *proto = (ProtoType *)ty;
    return proto->ret;
}

int desc_is_numb(TypeDesc *ty);
int desc_is_int(TypeDesc *ty);

int desc_equal(TypeDesc *ty1, TypeDesc *ty2);
void desc_to_str(TypeDesc *ty, Buffer *buf);
void free_desc(TypeDesc *ty);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_TYPE_DESC_H_ */
