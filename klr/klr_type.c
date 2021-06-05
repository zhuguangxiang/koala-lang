/*
 * This file is part of the koala-lang project, under the MIT License.
 *
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
 */

#include "klr_type.h"
#include "util/atom.h"
#include "util/list.h"

#ifdef __cplusplus
extern "C" {
#endif

static KlrType bases[] = {
    { KLR_TYPE_NONE },    { KLR_TYPE_INT8 },    { KLR_TYPE_INT16 },
    { KLR_TYPE_INT32 },   { KLR_TYPE_INT64 },   { KLR_TYPE_FLOAT16 },
    { KLR_TYPE_FLOAT32 }, { KLR_TYPE_FLOAT64 }, { KLR_TYPE_BOOL },
    { KLR_TYPE_CHAR },    { KLR_TYPE_STR },     { KLR_TYPE_ANY },
};

KlrTypeRef klr_type_none(void)
{
    return &bases[KLR_TYPE_NONE];
}

KlrTypeRef klr_type_int8(void)
{
    return &bases[KLR_TYPE_INT8];
}

KlrTypeRef klr_type_int16(void)
{
    return &bases[KLR_TYPE_INT16];
}

KlrTypeRef klr_type_int32(void)
{
    return &bases[KLR_TYPE_INT32];
}

KlrTypeRef klr_type_int64(void)
{
    return &bases[KLR_TYPE_INT64];
}

KlrTypeRef klr_type_float16(void)
{
    return &bases[KLR_TYPE_FLOAT16];
}

KlrTypeRef klr_type_float32(void)
{
    return &bases[KLR_TYPE_FLOAT32];
}

KlrTypeRef klr_type_float64(void)
{
    return &bases[KLR_TYPE_FLOAT64];
}

KlrTypeRef klr_type_bool(void)
{
    return &bases[KLR_TYPE_BOOL];
}

KlrTypeRef klr_type_char(void)
{
    return &bases[KLR_TYPE_CHAR];
}

KlrTypeRef klr_type_str(void)
{
    return &bases[KLR_TYPE_STR];
}

KlrTypeRef klr_type_any(void)
{
    return &bases[KLR_TYPE_ANY];
}

typedef struct _KlrKlass {
    KlrTypeKind kind;
    char *name;
    char *path;
} KlrKlass, *KlrKlassRef;

static Vector klass_types;

KlrTypeRef klr_type_klass(char *path, char *name)
{
    KlrKlassRef klass = mm_alloc_obj(klass);
    klass->kind = KLR_TYPE_KLASS;
    klass->path = path;
    klass->name = name;
    vector_push_back(&klass_types, &klass);
    return (KlrTypeRef)klass;
}

char *klr_get_path(KlrTypeRef type)
{
    KlrKlassRef klass = (KlrKlassRef)type;
    return klass->path;
}

char *klr_get_name(KlrTypeRef type)
{
    KlrKlassRef klass = (KlrKlassRef)type;
    return klass->name;
}

typedef struct _KlrProto {
    KlrTypeKind kind;
    KlrTypeRef ret;
    Vector *params;
} KlrProto, *KlrProtoRef;

static KlrProto _void_func_type = { .kind = KLR_TYPE_PROTO };
static Vector func_types;

KlrTypeRef klr_type_proto(KlrTypeRef ret, Vector *params)
{
    if (!ret && !params) return (KlrTypeRef)&_void_func_type;

    KlrProtoRef pty = mm_alloc_obj(pty);
    pty->kind = KLR_TYPE_PROTO;
    pty->ret = ret;
    pty->params = params;
    vector_push_back(&func_types, &pty);
    return (KlrTypeRef)pty;
}

Vector *klr_get_params(KlrTypeRef ty)
{
    if (!ty) return NULL;
    KlrProtoRef proto = (KlrProtoRef)ty;
    return proto->params;
}

KlrTypeRef klr_get_ret(KlrTypeRef ty)
{
    if (!ty) return NULL;
    KlrProtoRef proto = (KlrProtoRef)ty;
    return proto->ret;
}

void klr_init_types(void)
{
    vector_init_ptr(&klass_types);
    vector_init_ptr(&func_types);
}

void klr_fini_types(void)
{
    KlrKlassRef *k;
    vector_foreach(k, &klass_types, { mm_free(*k); });
    vector_fini(&klass_types);

    KlrProtoRef *p;
    vector_foreach(p, &func_types, {
        vector_destroy((*p)->params);
        mm_free(*p);
    });
    vector_fini(&func_types);
}

int klr_type_equal(KlrTypeRef ty1, KlrTypeRef ty2)
{
    if (ty1 == ty2) return 1;
    return 0;
}

char *klr_type_tostr(KlrTypeRef ty)
{
    static char *strs[] = {
        "none", "i8",   "i16",  "i32", "i64", "f32",
        "f64",  "bool", "char", "str", "any",
    };

    return strs[ty->kind];
}

/*
 * Type descriptor
 *
 * int: i8, i16, i32, i64
 * float: f32, f64
 * bool: b
 * char: c
 * str: s
 * any: A
 * array: [s
 * Map: Mss
 * Tuple: T(i1s[s)
 * varg: ...s
 * klass: Lio.File;
 * type param: LOption<i2>;
 * proto: Pi4s:i1
 */

static KlrTypeRef __to_klass(char *s, int len)
{
    int dot = -1;
    for (int i = 0; i < len; i++) {
        if (s[i] == '.') {
            dot = i;
            break;
        }
    }

    char *path = NULL;
    if (dot > 0) {
        path = atom_str(s, dot);
    }
    char *name = atom_str(s + dot + 1, len - dot - 1);
    return klr_type_klass(path, name);
}

static KlrTypeRef __to_type(char **str)
{
    KlrTypeRef ty;
    char *s = *str;
    char ch = *s;
    char *k;

    switch (ch) {
        case 'i': {
            /* i1, i2, i4, i8 */
            ch = *++s;
            if (ch == '1') {
                ty = klr_type_int8();
            } else if (ch == '2') {
                ty = klr_type_int16();
            } else if (ch == '4') {
                ty = klr_type_int32();
            } else {
                ty = klr_type_int64();
            }
            break;
        }
        case 'f': {
            /* f4, f8 */
            ch = *++s;
            if (ch == '4') {
                ty = klr_type_float32();
            } else {
                ty = klr_type_float64();
            }
            break;
        }
        case 'b': {
            /* bool */
            ty = klr_type_bool();
            break;
        }
        case 'c': {
            /* char */
            ty = klr_type_char();
            break;
        }
        case 's': {
            /* str */
            ty = klr_type_str();
            break;
        }
        case 'A': {
            /* any */
            ty = klr_type_any();
            break;
        }
        case '[': {
            break;
        }
        case 'M': {
            break;
        }
        case 'L': {
            k = ++s;
            while (*s != ';') ++s;
            ty = __to_klass(k, s - k);
            break;
        }
        default:
            break;
    }
    *str = s;
    return ty;
}

KlrTypeRef klr_type_from_str(char *str)
{
    if (!str) return NULL;
    return __to_type(&str);
}

KlrTypeRef klr_proto_from_str(char *params, char *ret)
{
    Vector *args = NULL;
    if (params) {
        args = vector_create(PTR_SIZE);
        KlrTypeRef ty;
        char *s = params;
        while (*s) {
            ty = __to_type(&s);
            vector_push_back(args, &ty);
            ++s;
        }
        if (vector_empty(args)) {
            vector_destroy(args);
            args = NULL;
        }
    }

    KlrTypeRef rty = NULL;
    if (ret) {
        rty = __to_type(&ret);
    }

    return klr_type_proto(rty, args);
}

#ifdef __cplusplus
}
#endif
