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

#include "klvm/klvm_type.h"
#include "atom.h"
#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

static KLVMType bases[] = {
    { KLVM_TYPE_UNDEF },   { KLVM_TYPE_INT8 },  { KLVM_TYPE_INT16 },
    { KLVM_TYPE_INT32 },   { KLVM_TYPE_INT64 }, { KLVM_TYPE_FLOAT32 },
    { KLVM_TYPE_FLOAT64 }, { KLVM_TYPE_BOOL },  { KLVM_TYPE_CHAR },
    { KLVM_TYPE_STR },     { KLVM_TYPE_ANY },
};

KLVMTypeRef KLVMTypeInt8(void)
{
    return &bases[KLVM_TYPE_INT8];
}

KLVMTypeRef KLVMTypeInt16(void)
{
    return &bases[KLVM_TYPE_INT16];
}

KLVMTypeRef KLVMTypeInt32(void)
{
    return &bases[KLVM_TYPE_INT32];
}

KLVMTypeRef KLVMTypeInt64(void)
{
    return &bases[KLVM_TYPE_INT64];
}

KLVMTypeRef KLVMTypeFloat32(void)
{
    return &bases[KLVM_TYPE_FLOAT32];
}

KLVMTypeRef KLVMTypeFloat64(void)
{
    return &bases[KLVM_TYPE_FLOAT64];
}

KLVMTypeRef KLVMTypeBool(void)
{
    return &bases[KLVM_TYPE_BOOL];
}

KLVMTypeRef KLVMTypeChar(void)
{
    return &bases[KLVM_TYPE_CHAR];
}

KLVMTypeRef KLVMTypeStr(void)
{
    return &bases[KLVM_TYPE_STR];
}

KLVMTypeRef KLVMTypeAny(void)
{
    return &bases[KLVM_TYPE_ANY];
}

typedef struct _KLVMKlass {
    KLVMTypeKind kind;
    char *name;
    char *path;
} KLVMKlass, *KLVMKlassRef;

static Vector klasstypes;

static void __fini_klasstypes(void)
{
    KLVMKlassRef *k;
    VectorForEach(k, &klasstypes, { MemFree(*k); });
}

KLVMTypeRef KLVMTypeKlass(char *path, char *name)
{
    KLVMKlassRef klass = MemAllocWithPtr(klass);
    klass->kind = KLVM_TYPE_KLASS;
    klass->path = path;
    klass->name = name;
    VectorPushBack(&klasstypes, &klass);
    return (KLVMTypeRef)klass;
}

char *KLVMGetPath(KLVMTypeRef type)
{
    KLVMKlassRef klass = (KLVMKlassRef)type;
    return klass->path;
}

char *KLVMGetName(KLVMTypeRef type)
{
    KLVMKlassRef klass = (KLVMKlassRef)type;
    return klass->name;
}

typedef struct _KLVMProto {
    KLVMTypeKind kind;
    KLVMTypeRef ret;
    VectorRef params;
} KLVMProto, *KLVMProtoRef;

static KLVMProto _empty_functype = { .kind = KLVM_TYPE_PROTO };
static Vector functypes;

static void __fini_functypes(void)
{
    KLVMProtoRef *p;
    VectorForEach(p, &functypes, { MemFree(*p); });
}

KLVMTypeRef KLVMTypeProto(KLVMTypeRef ret, KLVMTypeRef params[], int size)
{
    if (!ret && !params) return (KLVMTypeRef)&_empty_functype;

    KLVMProtoRef pty = MemAllocWithPtr(pty);
    pty->kind = KLVM_TYPE_PROTO;
    pty->ret = ret;
    if (params && size > 0) {
        VectorRef vec = VectorCreate(PTR_SIZE);
        KLVMTypeRef *ty;
        for (int i = 0; i < size; i++) {
            ty = params + i;
            VectorPushBack(vec, ty);
        }
        pty->params = vec;
    }
    VectorPushBack(&functypes, &pty);
    return (KLVMTypeRef)pty;
}

VectorRef KLVMProtoParams(KLVMTypeRef ty)
{
    if (!ty) return NULL;
    KLVMProtoRef proto = (KLVMProtoRef)ty;
    return proto->params;
}

KLVMTypeRef KLVMProtoRet(KLVMTypeRef ty)
{
    if (!ty) return NULL;
    KLVMProtoRef proto = (KLVMProtoRef)ty;
    return proto->ret;
}

void KLVMInitTypes(void)
{
    VectorInitPtr(&klasstypes);
    VectorInitPtr(&functypes);
}

void KLVMFiniTypes(void)
{
    __fini_klasstypes();
    __fini_functypes();
}

char *KLVMTypeToStr(KLVMTypeRef ty)
{
    static char *strs[] = {
        "undef", "i8",   "i16",  "i32", "i64", "f32",
        "f64",   "bool", "char", "str", "any",
    };

    return strs[ty->kind];
}

int KLVMTypeEqual(KLVMTypeRef ty1, KLVMTypeRef ty2)
{
    if (ty1 == ty2) return 1;
    return 0;
}

#ifdef __cplusplus
}
#endif
