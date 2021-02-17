/*----------------------------------------------------------------------------*\
|* This file is part of the koala project, under the MIT License.             *|
|* Copyright (c) 2021-2021 James <zhuguangxiang@gmail.com>                    *|
\*----------------------------------------------------------------------------*/

#include "klvm.h"

#ifdef __cplusplus
extern "C" {
#endif

klvm_type_t klvm_type_int32 = { KLVM_TYPE_INT32 };
klvm_type_t klvm_type_bool = { KLVM_TYPE_BOOL };

static struct basetypeinfo {
    klvm_type_kind_t kind;
    char *s;
} bases[] = {
    { KLVM_TYPE_INT8, "i8" },
    { KLVM_TYPE_INT16, "i16" },
    { KLVM_TYPE_INT32, "i32" },
    { KLVM_TYPE_INT64, "i64" },
    { KLVM_TYPE_FLOAT32, "f32" },
    { KLVM_TYPE_FLOAT64, "f64" },
    { KLVM_TYPE_BOOL, "bool" },
    { KLVM_TYPE_CHAR, "char" },
    { KLVM_TYPE_STRING, "str" },
    { KLVM_TYPE_ANY, "any" },
};

int klvm_type_check(klvm_type_t *ty1, klvm_type_t *ty2)
{
    if (ty1 == ty2) return 0;
    return -1;
}

char *klvm_type_string(klvm_type_t *ty)
{
    struct basetypeinfo *base;
    for (int i = 0; i < COUNT_OF(bases); i++) {
        base = bases + i;
        if (base->kind == ty->kind) return base->s;
    }
    return NULL;
}

typedef struct klvm_proto {
    klvm_type_kind_t kind;
    klvm_type_t *ret;
    vector_t *params;
} klvm_proto_t;

static klvm_proto_t klvm_proto_void_void = { .kind = KLVM_TYPE_PROTO };

klvm_type_t *klvm_type_proto(klvm_type_t *ret, klvm_type_t **params)
{
    if (!ret && !params) return (klvm_type_t *)&klvm_proto_void_void;

    klvm_proto_t *pty = malloc(sizeof(*pty));
    pty->kind = KLVM_TYPE_PROTO;
    pty->ret = ret;

    if (params) {
        vector_t *vec = vector_new(sizeof(void *));
        klvm_type_t **ty = params;
        while (*ty) {
            vector_push_back(vec, ty);
            ty++;
        }
        pty->params = vec;
    }

    return (klvm_type_t *)pty;
}

vector_t *klvm_proto_params(klvm_type_t *ty)
{
    if (!ty) return NULL;
    klvm_proto_t *proto = (klvm_proto_t *)ty;
    return proto->params;
}

klvm_type_t *klvm_proto_return(klvm_type_t *ty)
{
    if (!ty) return NULL;
    klvm_proto_t *proto = (klvm_proto_t *)ty;
    return proto->ret;
}

#ifdef __cplusplus
}
#endif
