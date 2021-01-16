/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangxiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_TYPE_DESC_H_
#define _KOALA_TYPE_DESC_H_

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TYPE_INT8    1
#define TYPE_INT16   2
#define TYPE_INT32   3
#define TYPE_INT64   4
#define TYPE_FLOAT32 5
#define TYPE_FLOAT64 6
#define TYPE_BOOL    7
#define TYPE_CHAR    8
#define TYPE_STRING  9
#define TYPE_ANY     10
#define TYPE_ARRAY   11
#define TYPE_DICT    12
#define TYPE_TUPLE   13
#define TYPE_VALIST  14
#define TYPE_KLASS   15
#define TYPE_PROTO   16

/*
 * klass: Lio.File;
 * proto: Pis:i
 * array: [s
 * Map: Mss
 * varg: ...s
 */
typedef struct TypeDesc {
    uint8_t kind;
} TypeDesc;

extern TypeDesc kl_type_int8;
extern TypeDesc kl_type_int16;
extern TypeDesc kl_type_int32;
extern TypeDesc kl_type_int64;
extern TypeDesc kl_type_int;
extern TypeDesc kl_type_float32;
extern TypeDesc kl_type_float64;
extern TypeDesc kl_type_float;
extern TypeDesc kl_type_bool;
extern TypeDesc kl_type_char;
extern TypeDesc kl_type_str;
extern TypeDesc kl_type_any;

typedef struct KlassDesc {
    uint8_t kind;
    const char *path;
    const char *name;
    Vector *typeparas;
} KlassDesc;

typedef struct ProtoDesc {
    uint8_t kind;
    Vector *ptypes;
    TypeDesc *rtype;
} ProtoDesc;

typedef struct ArrayDesc {
    uint8_t kind;
    TypeDesc *sub;
} ArrayDesc;

typedef struct MapDesc {
    uint8_t kind;
    TypeDesc *key;
    TypeDesc *val;
} MapDesc;

typedef struct VaListDesc {
    uint8_t kind;
    TypeDesc *sub;
} VaListDesc;

typedef struct TupleDesc {
    uint8_t kind;
    Vector subs;
} TupleDesc;

/* base desc */
TypeDesc *base_desc(uint8_t kind);

/* base desc string */
char *base_desc_str(uint8_t kind);

/* function proto descriptor */
TypeDesc *desc_from_proto(Vector *args, TypeDesc *ret);

/* new type descriptor from string */
TypeDesc *to_desc(char *type);

/* new proto descriptor from string */
TypeDesc *to_proto(char *para, char *ret);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_TYPE_DESC_H_ */
