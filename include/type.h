/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _KOALA_TYPE_DESC_H_
#define _KOALA_TYPE_DESC_H_

#include "klvm_type.h"

#ifdef __cplusplus
extern "C" {
#endif

/* identifier */
typedef struct _Ident {
    char *name;
    short row;
    short col;
} Ident;

// clang-format off
#define TYPE_HEAD KLVMTypeKind kind; short row; short col;
// clang-format on

/* type */
typedef struct _Type {
    TYPE_HEAD
} Type, *TypeRef;

typedef struct _ArrayType {
    TYPE_HEAD
    TypeRef subtype;
} ArrayType, *ArrayTypeRef;

typedef struct _MapType {
    TYPE_HEAD
    TypeRef ktype;
    TypeRef vtype;
} MapType, *MapTypeRef;

typedef struct _TupleType {
    TYPE_HEAD
    VectorRef subtypes;
} TupleType, *TupleTypeRef;

typedef struct _VaListType {
    TYPE_HEAD
    TypeRef subtype;
} VaListType, *VaListTypeRef;

typedef struct _KlassType {
    TYPE_HEAD
    Ident pkg;
    Ident name;
    VectorRef tps;
} KlassType, *KlassTypeRef;

static inline void type_set_loc(TypeRef ty, int row, int col)
{
    ty->row = row;
    ty->col = col;
}

TypeRef ast_type_int8(void);
TypeRef ast_type_klass(Ident *pkg, Ident *name);
TypeRef ast_type_proto(TypeRef ret, VectorRef params);
TypeRef ast_type_valist(TypeRef subtype);

static inline void type_add_typeparam(Type *_type, Type *param)
{
    VectorRef params = ((KlassTypeRef)_type)->tps;
    if (!params) {
        params = vector_new(sizeof(Type));
        ((KlassTypeRef)_type)->tps = params;
    }
    vector_push_back(params, &param);
}

static inline void type_set_typeparams(Type *_type, VectorRef params)
{
    ((KlassTypeRef)_type)->tps = params;
}

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_TYPE_DESC_H_ */
