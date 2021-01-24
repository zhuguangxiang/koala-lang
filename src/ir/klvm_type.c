/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangXiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "klvm.h"

#ifdef __cplusplus
extern "C" {
#endif

KLVMType *KLVMInt32Type(void)
{
    static KLVMType i32 = { .kind = KLVM_TYPE_INT32 };
    return &i32;
}

static struct baseinfo {
    KLVMTypeKind kind;
    char *s;
} bases[] = {
    { KLVM_TYPE_INT8, "int8" },
    { KLVM_TYPE_INT16, "int16" },
    { KLVM_TYPE_INT32, "int32" },
    { KLVM_TYPE_INT64, "int64" },
    { KLVM_TYPE_FLOAT32, "float32" },
    { KLVM_TYPE_FLOAT64, "float64" },
    { KLVM_TYPE_BOOL, "bool" },
    { KLVM_TYPE_CHAR, "char" },
    { KLVM_TYPE_STRING, "string" },
    { KLVM_TYPE_ANY, "any" },
};

int KLVMTypeCheck(KLVMType *ty1, KLVMType *ty2)
{
    if (ty1 == ty2) return 0;
    return -1;
}

typedef struct _KLVMProtoType {
    KLVMTypeKind kind;
    KLVMType *ret;
    Vector *params;
} _KLVMProtoType;

KLVMType *KLVMProtoType(KLVMType *ret, KLVMType **params)
{
    _KLVMProtoType *pty = mm_alloc(sizeof(*pty));
    pty->kind = KLVM_TYPE_PROTO;
    pty->ret = ret;

    if (params) {
        pty->params = vector_new(sizeof(void *));
        KLVMType **ty = params;
        while (*ty) {
            vector_push_back(pty->params, ty);
            ty++;
        }
    }
    return (KLVMType *)pty;
}

Vector *KLVMProtoTypeParams(KLVMType *ty)
{
    if (!ty) return NULL;
    _KLVMProtoType *pty = (_KLVMProtoType *)ty;
    return pty->params;
}

KLVMType *KLVMProtoTypeReturn(KLVMType *ty)
{
    if (!ty) return NULL;
    _KLVMProtoType *pty = (_KLVMProtoType *)ty;
    return pty->ret;
}

char *KLVMTypeString(KLVMType *ty)
{
    struct baseinfo *base;
    for (int i = 0; i < COUNT_OF(bases); i++) {
        base = bases + i;
        if (base->kind == ty->kind) return base->s;
    }
    return NULL;
}

#ifdef __cplusplus
}
#endif
