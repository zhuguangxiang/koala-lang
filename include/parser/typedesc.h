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
    TYPE_CHAR_KIND,
    TYPE_BYTES_KIND,
    TYPE_TYPE_KIND,
    TYPE_RANGE_KIND,
    TYPE_OPTIONAL_KIND,
    TYPE_MAX_KIND,
} DescKind;

typedef struct _TypeDesc {
    int refcnt;
    DescKind kind;
} TypeDesc;

typedef struct _OptionalDesc {
    int refcnt;
    DescKind kind;
    TypeDesc *type;
} OptionalDesc;

typedef struct _ArrayDesc {
    int refcnt;
    DescKind kind;
    TypeDesc *type;
} ArrayDesc;

#define DESC_INCREF(ty) ++(ty)->refcnt

// clang-format off
#define DESC_INCREF_GET(ty) ({++(ty)->refcnt; (ty);})
// clang-format on

extern TypeDesc int_desc;
extern TypeDesc float_desc;
extern TypeDesc bool_desc;
extern TypeDesc str_desc;
extern TypeDesc object_desc;
extern TypeDesc char_desc;
extern TypeDesc bytes_desc;
extern TypeDesc type_desc;
extern TypeDesc range_desc;

static inline TypeDesc *desc_int(void) { return (TypeDesc *)DESC_INCREF_GET(&int_desc); }

static inline TypeDesc *desc_float(void)
{
    return (TypeDesc *)DESC_INCREF_GET(&float_desc);
}

static inline TypeDesc *desc_str(void) { return (TypeDesc *)DESC_INCREF_GET(&str_desc); }

static inline TypeDesc *desc_bool(void)
{
    return (TypeDesc *)DESC_INCREF_GET(&bool_desc);
}

static inline TypeDesc *desc_object(void)
{
    return (TypeDesc *)DESC_INCREF_GET(&object_desc);
}

static inline TypeDesc *desc_char(void)
{
    return (TypeDesc *)DESC_INCREF_GET(&char_desc);
}

static inline TypeDesc *desc_bytes(void)
{
    return (TypeDesc *)DESC_INCREF_GET(&bytes_desc);
}

static inline TypeDesc *desc_type(void)
{
    return (TypeDesc *)DESC_INCREF_GET(&type_desc);
}

static inline TypeDesc *desc_range(void)
{
    return (TypeDesc *)DESC_INCREF_GET(&range_desc);
}

TypeDesc *desc_optional(TypeDesc *ty);
TypeDesc *desc_array(TypeDesc *sub);

static inline int desc_is_int(TypeDesc *desc) { return desc->kind == TYPE_INT_KIND; }
void free_desc(TypeDesc *ty);
int desc_equal(TypeDesc *a, TypeDesc *b);
int desc_to_str(TypeDesc *ty, Buffer *buf);
TypeDesc *desc_from_str(const char *s);
void desc_print(TypeDesc *desc, Buffer *buf);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_TYPEDESC_H_ */
