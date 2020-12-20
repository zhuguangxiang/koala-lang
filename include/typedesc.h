/*===-- typedesc.h - Type Descriptor ------------------------------*- C -*-===*\
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) 2020 James, https://github.com/zhuguangxiang                 *|
|*                                                                            *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* This header declares type descriptor structure and its interfaces.         *|
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

#define TYPE_BYTE   'b'
#define TYPE_INT    'i'
#define TYPE_FLOAT  'f'
#define TYPE_BOOL   'z'
#define TYPE_CHAR   'c'
#define TYPE_STR    's'
#define TYPE_ANY    'A'

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

typedef struct KlassDesc {
    TYPE_DESC_HEAD
    const char *path;
    const char *name;
    Vector *typeparas;
} KlassDesc;

typedef struct ProtoDesc {
    TYPE_DESC_HEAD
    TypeDesc *ret;
    Vector *args;
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

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_TYPE_DESC_H_ */
