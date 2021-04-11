/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2020-2021 James <zhuguangxiang@gmail.com>
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

#ifndef _KLIR_TYPE_H_
#define _KLIR_TYPE_H_

#if !defined(_KLVM_H_INSIDE_) && !defined(KLVM_COMPILATION)
#error "Only <klvm.h> can be included directly."
#endif

#include "common.h"
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _KLVMTypeKind {
    KLVM_TYPE_UNDEF,
    KLVM_TYPE_INT8,
    KLVM_TYPE_INT16,
    KLVM_TYPE_INT32,
    KLVM_TYPE_INT64,
    KLVM_TYPE_FLOAT32,
    KLVM_TYPE_FLOAT64,
    KLVM_TYPE_BOOL,
    KLVM_TYPE_CHAR,
    KLVM_TYPE_STR,
    KLVM_TYPE_ANY,
    KLVM_TYPE_PARAM,
    KLVM_TYPE_ARRAY,
    KLVM_TYPE_MAP,
    KLVM_TYPE_TUPLE,
    KLVM_TYPE_VALIST,
    KLVM_TYPE_KLASS,
    KLVM_TYPE_PROTO,
} KLVMTypeKind;

/*
 * int: i, i8, i16, i32, i64
 * float: f32, f64
 * bool: b
 * char: c
 * str: s
 * any: A
 * array: [s
 * Map: M(s:s)
 * Tuple: T(is[s)
 * varg: ...s
 * klass: Lio.File;
 * proto: P(is:i)
 */
typedef struct _KLVMType {
    KLVMTypeKind kind;
} KLVMType, *KLVMTypeRef;

void KLVMInitTypes(void);
void KLVMFiniTypes(void);

KLVMTypeRef KLVMTypeInt8(void);
KLVMTypeRef KLVMTypeInt16(void);
KLVMTypeRef KLVMTypeInt32(void);
KLVMTypeRef KLVMTypeInt64(void);
KLVMTypeRef KLVMTypeFloat32(void);
KLVMTypeRef KLVMTypeFloat64(void);
KLVMTypeRef KLVMTypeBool(void);
KLVMTypeRef KLVMTypeChar(void);
KLVMTypeRef KLVMTypeStr(void);
KLVMTypeRef KLVMTypeAny(void);

KLVMTypeRef KLVMTypeKlass(char *path, char *name);
char *KLVMGetPath(KLVMTypeRef ty);
char *KLVMGetName(KLVMTypeRef ty);

KLVMTypeRef KLVMTypeProto(KLVMTypeRef ret, KLVMTypeRef params[], int size);
VectorRef KLVMProtoParams(KLVMTypeRef ty);
KLVMTypeRef KLVMProtoRet(KLVMTypeRef ty);

char *KLVMTypeToStr(KLVMTypeRef ty);
int KLVMTypeEqual(KLVMTypeRef ty1, KLVMTypeRef ty2);

#ifdef __cplusplus
}
#endif

#endif /* _KLIR_TYPE_H_ */
