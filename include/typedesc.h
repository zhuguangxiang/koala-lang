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

#define TYPE_NIL    0
#define TYPE_KLASS  1
#define TYPE_PROTO  2
#define TYPE_ARRAY  3
#define TYPE_MAP    4
#define TYPE_VALIST 5
#define TYPE_TUPLE  6

#define TYPE_BYTE  'b'
#define TYPE_INT   'i'
#define TYPE_FLOAT 'f'
#define TYPE_BOOL  'z'
#define TYPE_CHAR  'c'
#define TYPE_STR   's'
#define TYPE_ANY   'A'

#define TYPE_DESC_HEAD char kind;

/*
 * klass: Lio.File;
 * proto: Pis:i
 * array: [s
 * Map: Mss
 * varg: ...s
 */
typedef struct TypeDesc {
    TYPE_DESC_HEAD
} TypeDesc;

extern TypeDesc kl_type_byte;
extern TypeDesc kl_type_int;
extern TypeDesc kl_type_float;
extern TypeDesc kl_type_bool;
extern TypeDesc kl_type_char;
extern TypeDesc kl_type_str;
extern TypeDesc kl_type_any;

typedef struct KlassDesc {
    TYPE_DESC_HEAD
    const char *path;
    const char *name;
    Vector *typeparas;
} KlassDesc;

typedef struct ProtoDesc {
    TYPE_DESC_HEAD
    Vector *ptypes;
    TypeDesc *rtype;
} ProtoDesc;

typedef struct ArrayDesc {
    TYPE_DESC_HEAD
    TypeDesc *sub;
} ArrayDesc;

typedef struct MapDesc {
    TYPE_DESC_HEAD
    TypeDesc *key;
    TypeDesc *val;
} MapDesc;

typedef struct VaListDesc {
    TYPE_DESC_HEAD
    TypeDesc *sub;
} VaListDesc;

typedef struct TupleDesc {
    TYPE_DESC_HEAD
    Vector *subs;
} TupleDesc;

/* base type string */
char *base_type_str(char kind);

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
