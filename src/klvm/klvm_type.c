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

#include "klvm_type.h"

#ifdef __cplusplus
extern "C" {
#endif

static KLVMType bases[] = {
    { KLVM_TYPE_INVALID }, { KLVM_TYPE_INT8 },  { KLVM_TYPE_INT16 },
    { KLVM_TYPE_INT32 },   { KLVM_TYPE_INT64 }, { KLVM_TYPE_FLOAT32 },
    { KLVM_TYPE_FLOAT64 }, { KLVM_TYPE_BOOL },  { KLVM_TYPE_CHAR },
    { KLVM_TYPE_STR },     { KLVM_TYPE_ANY },
};

KLVMTypeRef klvm_type_int8(void)
{
    return &bases[KLVM_TYPE_INT8];
}

KLVMTypeRef klvm_type_int16(void)
{
    return &bases[KLVM_TYPE_INT16];
}

KLVMTypeRef klvm_type_int32(void)
{
    return &bases[KLVM_TYPE_INT32];
}

KLVMTypeRef klvm_type_int64(void)
{
    return &bases[KLVM_TYPE_INT64];
}

KLVMTypeRef klvm_type_int(void)
{
    return &bases[KLVM_TYPE_INT64];
}

KLVMTypeRef klvm_type_float32(void)
{
    return &bases[KLVM_TYPE_FLOAT32];
}

KLVMTypeRef klvm_type_float64(void)
{
    return &bases[KLVM_TYPE_FLOAT64];
}

KLVMTypeRef klvm_type_bool(void)
{
    return &bases[KLVM_TYPE_BOOL];
}

KLVMTypeRef klvm_type_char(void)
{
    return &bases[KLVM_TYPE_CHAR];
}

KLVMTypeRef klvm_type_str(void)
{
    return &bases[KLVM_TYPE_STR];
}

KLVMTypeRef klvm_type_any(void)
{
    return &bases[KLVM_TYPE_ANY];
}

typedef struct _KLVMProto {
    KLVMTypeKind kind;
    KLVMTypeRef ret;
    VectorRef params;
} KLVMProto, *KLVMProtoRef;

static KLVMProto _empty_proto = { .kind = KLVM_TYPE_PROTO };

KLVMTypeRef klvm_type_proto(KLVMTypeRef ret, VectorRef params)
{
    if (!ret && !params) return (KLVMTypeRef)&_empty_proto;

    KLVMProtoRef ty = mm_alloc(sizeof(*ty));
    ty->kind = KLVM_TYPE_PROTO;
    ty->ret = ret;
    ty->params = params;
    return (KLVMTypeRef)ty;
}

VectorRef klvm_proto_params(KLVMTypeRef ty)
{
    if (!ty) return NULL;
    KLVMProtoRef proto = (KLVMProtoRef)ty;
    return proto->params;
}

KLVMTypeRef klvm_proto_ret(KLVMTypeRef ty)
{
    if (!ty) return NULL;
    KLVMProtoRef proto = (KLVMProtoRef)ty;
    return proto->ret;
}

static KLVMTypeRef __to_int_type(char **str)
{
    KLVMTypeRef type;
    char *s = *str;
    if (*s == '8') {
        type = klvm_type_int8();
        s += 1;
    } else if (s[0] == '1' && s[1] == '6') {
        type = klvm_type_int16();
        s += 2;
    } else if (s[0] == '3' && s[1] == '2') {
        type = klvm_type_int32();
        s += 2;
    } else if (s[0] == '6' && s[1] == '4') {
        type = klvm_type_int64();
        s += 2;
    }
    *str = s;
    return type;
}

static KLVMTypeRef __to_type(char **str)
{
    char *s = *str;
    char ch = *s;
    KLVMTypeRef type;
    KLVMTypeRef t1;

    switch (ch) {
        case 'i': {
            s++;
            type = __to_int_type(&s);
            break;
        }
        case 'z': {
            type = klvm_type_bool();
            s++;
            break;
        }
        case 's': {
            type = klvm_type_str();
            s++;
            break;
        }
        case 'L': {
            break;
        }
        case '(': {
            break;
        }
        case '<': {
            break;
        }
        case '.': {
            break;
        }
        case '[': {
            break;
        }
        case 'M': {
            break;
        }
        case 'P': {
            s++;
            VectorRef args = NULL;
            if (*s != ':') {
                args = vector_new(sizeof(KLVMTypeRef));
                while (*s != ':') {
                    t1 = __to_type(&s);
                    vector_push_back(args, &t1);
                }
            }
            s++;
            t1 = __to_type(&s);
            type = klvm_type_proto(t1, args);
            break;
        }
        case 0: {
            type = NULL;
            break;
        }
        default: {
            abort();
        }
    }
    *str = s;
    return type;
}

KLVMTypeRef klvm_new_type(char *s)
{
    if (!s) return NULL;
    return __to_type(&s);
}

KLVMTypeRef klvm_new_proto(char *para, char *ret)
{
    VectorRef ptypes = NULL;
    if (para) {
        ptypes = vector_new(sizeof(KLVMTypeRef));
        char *s = para;
        KLVMTypeRef type;
        while (*s) {
            type = __to_type(&s);
            vector_push_back(ptypes, &type);
        }
    }

    KLVMTypeRef rtype = NULL;
    if (ret) {
        char *s = ret;
        rtype = __to_type(&s);
    }

    return klvm_type_proto(rtype, ptypes);
}

int klvm_type_equal(KLVMTypeRef ty1, KLVMTypeRef ty2)
{
    if (ty1 == ty2) return 1;
    return 0;
}

char *klvm_type_tostr(KLVMTypeRef ty)
{
    static char *strs[] = {
        "invalid", "i8",   "i16",  "i32", "i64", "f32",
        "f64",     "bool", "char", "str", "any",
    };

    return strs[ty->kind];
}

#ifdef __cplusplus
}
#endif
