/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "typedesc.h"
#include "atom.h"

#ifdef __cplusplus
extern "C" {
#endif

static TypeDesc bases[] = {
    { TYPE_ANY_KIND }, { TYPE_I8_KIND },   { TYPE_I16_KIND },
    { TYPE_I32_KIND }, { TYPE_I64_KIND },  { TYPE_F32_KIND },
    { TYPE_F64_KIND }, { TYPE_BOOL_KIND }, { TYPE_CHAR_KIND },
    { TYPE_STR_KIND },
};

TypeDesc *desc_from_any(void)
{
    return &bases[TYPE_ANY_KIND];
}

TypeDesc *desc_from_int8(void)
{
    return &bases[TYPE_I8_KIND];
}

TypeDesc *desc_from_int16(void)
{
    return &bases[TYPE_I16_KIND];
}

TypeDesc *desc_from_int32(void)
{
    return &bases[TYPE_I32_KIND];
}

TypeDesc *desc_from_int64(void)
{
    return &bases[TYPE_I64_KIND];
}

TypeDesc *desc_from_float32(void)
{
    return &bases[TYPE_F32_KIND];
}

TypeDesc *desc_from_float64(void)
{
    return &bases[TYPE_F64_KIND];
}

TypeDesc *desc_from_bool(void)
{
    return &bases[TYPE_BOOL_KIND];
}

TypeDesc *desc_from_char(void)
{
    return &bases[TYPE_CHAR_KIND];
}

TypeDesc *desc_from_str(void)
{
    return &bases[TYPE_STR_KIND];
}

static Vector *cache_types;
static ProtoType _void_proto_type = { .kind = TYPE_PROTO_KIND };

TypeDesc *desc_from_array(TypeDesc *sub)
{
    ArrayType *arr = mm_alloc_obj(arr);
    arr->kind = TYPE_ARRAY_KIND;
    arr->ty = sub;
    arr->refcnt = 1;
    ++sub->refcnt;
    vector_push_back(cache_types, &arr);
    return (TypeDesc *)arr;
}

TypeDesc *desc_from_map(TypeDesc *kty, TypeDesc *vty)
{
    MapType *map = mm_alloc_obj(map);
    map->kind = TYPE_MAP_KIND;
    map->refcnt = 1;
    map->kty = kty;
    map->vty = vty;
    ++kty->refcnt;
    ++vty->refcnt;
    vector_push_back(cache_types, &map);
    return (TypeDesc *)map;
}

TypeDesc *desc_from_klass(char *path, char *name, Vector *args)
{
    KlassType *kls = mm_alloc_obj(kls);
    kls->kind = TYPE_KLASS_KIND;
    kls->path = path;
    kls->name = name;
    kls->args = args;
    kls->refcnt = 1;
    vector_push_back(cache_types, &kls);
    return (TypeDesc *)kls;
}

TypeDesc *desc_from_proto(TypeDesc *ret, Vector *params)
{
    if (!ret && !params) return (TypeDesc *)&_void_proto_type;

    ProtoType *pty = mm_alloc_obj(pty);
    pty->kind = TYPE_PROTO_KIND;
    pty->ret = ret;
    pty->params = params;
    pty->refcnt = 1;
    if (ret) ++ret->refcnt;
    vector_push_back(cache_types, &pty);
    return (TypeDesc *)pty;
}

TypeDesc *desc_from_proto2(TypeDesc *ret, TypeDesc *params[], int size)
{
    Vector *vec = null;
    if (params && size > 0) {
        vec = vector_create_ptr();
        for (int i = 0; i < size; i++) vector_push_back(vec, &params[i]);
    }
    return desc_from_proto(ret, vec);
}

int desc_equal(TypeDesc *ty1, TypeDesc *ty2)
{
    if (ty1 == ty2) return 1;
    if (ty1->kind != ty2->kind) return 0;

    return 0;
}

static void proto_to_str(TypeDesc *ty, Buffer *buf)
{
    if (!ty) {
        buf_write_str(buf, "()");
        return;
    }

    if (ty == (TypeDesc *)&_void_proto_type) {
        buf_write_str(buf, "() -> _");
        return;
    }

    buf_write_char(buf, '(');
    ProtoType *proto = (ProtoType *)ty;
    Vector *params = proto->params;
    TypeDesc **item;
    vector_foreach(item, params, {
        if (i != 0) buf_write_str(buf, ", ");
        if ((*item)->kind == TYPE_PROTO_KIND) {
            proto_to_str(*item, buf);
        } else {
            desc_to_str(*item, buf);
        }
    });
    buf_write_char(buf, ')');

    TypeDesc *ret = proto->ret;
    if (!ret) return;
    buf_write_str(buf, " -> ");
    if (ret->kind == TYPE_PROTO_KIND) {
        proto_to_str(ret, buf);
    } else {
        desc_to_str(ret, buf);
    }
}

void desc_to_str(TypeDesc *ty, Buffer *buf)
{
    if (!ty) return;

    static char *strs[] = {
        "any",     "int8",    "int16", "int32", "int64",
        "float32", "float64", "bool",  "char",  "string",
    };

    if (ty->kind >= TYPE_ANY_KIND && ty->kind <= TYPE_STR_KIND) {
        buf_write_str(buf, strs[ty->kind]);
        return;
    }

    if (ty->kind == TYPE_KLASS_KIND) {
        KlassType *kls = (KlassType *)ty;
        if (kls->path) {
            buf_write_str(buf, kls->path);
            buf_write_char(buf, '.');
        }
        buf_write_str(buf, kls->name);
        if (kls->args) {
            buf_write_char(buf, '<');
            TypeDesc **item;
            vector_foreach(item, kls->args, {
                if (i != 0) buf_write_str(buf, ", ");
                desc_to_str(*item, buf);
            });
            buf_write_char(buf, '>');
        }
        return;
    }

    if (ty->kind == TYPE_ARRAY_KIND) {
        return;
    }

    if (ty->kind == TYPE_MAP_KIND) {
        return;
    }

    if (ty->kind == TYPE_PROTO_KIND) {
        proto_to_str(ty, buf);
        return;
    }

    assert(0);
    return;
}

/*
 * int: i8 i16 i32 i64
 * float: f32 f64
 * bool: b
 * char: c
 * str: s
 * any: A
 * array: [s
 * Map: Mss
 * Tuple: Ti8s[s;
 * varg: ...s
 * klass: Lio.File;
 * type param: LOption<i8>;
 * proto: Pi8s:i8
 */

static TypeDesc *__to_klass(char *s, int len)
{
    int dot = -1;
    for (int i = 0; i < len; i++) {
        if (s[i] == '.') {
            dot = i;
            break;
        }
    }

    char *path = null;
    if (dot > 0) {
        path = atom_str(s, dot);
    }
    char *name = atom_str(s + dot + 1, len - dot - 1);
    return desc_from_klass(path, name, null);
}

static TypeDesc *__to_descr(char **str)
{
    TypeDesc *ty;
    char *s = *str;
    char ch = *s;
    char *k;

    switch (ch) {
        case 'i': {
            /* i8, i16, i32, i64 */
            ch = *++s;
            if (ch == '8') {
                ty = desc_from_int8();
            } else if (ch == '1') {
                ch = *++s;
                assert(ch == '6');
                ty = desc_from_int16();
            } else if (ch == '3') {
                ch = *++s;
                assert(ch == '2');
                ty = desc_from_int32();
            } else if (ch == '6') {
                ch = *++s;
                assert(ch == '4');
                ty = desc_from_int64();
            } else {
                assert(0);
            }
            break;
        }
        case 'f': {
            /* f32, f64 */
            ch = *++s;
            if (ch == '3') {
                ch = *++s;
                assert(ch == '2');
                ty = desc_from_float32();
            } else if (ch == '6') {
                ch = *++s;
                assert(ch == '4');
                ty = desc_from_float64();
            } else {
                assert(0);
            }
            break;
        }
        case 'b': {
            /* bool */
            ty = desc_from_bool();
            break;
        }
        case 'c': {
            /* char */
            ty = desc_from_char();
            break;
        }
        case 's': {
            /* str */
            ty = desc_from_str();
            break;
        }
        case 'A': {
            /* any */
            ty = desc_from_any();
            break;
        }
        case '[': {
            /* Array */
            ++s;
            TypeDesc *sub;
            sub = __to_descr(&s);
            ty = desc_from_array(sub);
            break;
        }
        case 'M': {
            assert(0);
            break;
        }
        case 'P': {
            assert(0);
            break;
        }
        case 'L': {
            k = ++s;
            while (*s != ';') ++s;
            ty = __to_klass(k, s - k);
            break;
        }
        default: {
            assert(0);
            break;
        }
    }
    *str = s;
    return ty;
}

TypeDesc *str_to_desc(char *str)
{
    if (!str) return null;
    return __to_descr(&str);
}

TypeDesc *str_to_proto(char *params, char *ret)
{
    Vector *vec = null;
    if (params) {
        vec = vector_create_ptr();
        TypeDesc *ty;
        char *s = params;
        while (*s) {
            ty = __to_descr(&s);
            vector_push_back(vec, &ty);
            ++ty->refcnt;
            ++s;
        }
    }

    TypeDesc *rty = null;
    if (ret) {
        rty = __to_descr(&ret);
    }

    return desc_from_proto(rty, vec);
}

int desc_is_number(TypeDesc *ty)
{
    return desc_is_integer(ty) || desc_is_float(ty);
}

int desc_is_integer(TypeDesc *ty)
{
    return (ty->kind == TYPE_I64_KIND) || (ty->kind == TYPE_I32_KIND) ||
           (ty->kind == TYPE_I16_KIND) || (ty->kind == TYPE_I8_KIND);
}

int desc_is_float(TypeDesc *ty)
{
    return (ty->kind == TYPE_F32_KIND) || (ty->kind == TYPE_F64_KIND);
}

int desc_is_bool(TypeDesc *ty)
{
    return ty->kind == TYPE_BOOL_KIND;
}

int desc_is_str(TypeDesc *ty)
{
    return ty->kind == TYPE_STR_KIND;
}

int desc_is_klass(TypeDesc *ty)
{
    return ty->kind == TYPE_KLASS_KIND;
}

void init_desc(void)
{
    cache_types = vector_create_ptr();
}

static void free_desc(TypeDesc *ty)
{
    if (!ty) return;

    int kind = ty->kind;
    if (kind >= TYPE_ANY_KIND && kind <= TYPE_STR_KIND) return;

    if (kind == TYPE_KLASS_KIND) {
        KlassType *kls = (KlassType *)ty;
        if (--kls->refcnt) return;
    } else if (kind == TYPE_ARRAY_KIND) {
        ArrayType *arr = (ArrayType *)ty;
        if (--arr->refcnt) return;
        free_desc(arr->ty);
    } else if (kind == TYPE_MAP_KIND) {
        MapType *map = (MapType *)ty;
        if (--map->refcnt) return;
        free_desc(map->kty);
        free_desc(map->vty);
    } else if (kind == TYPE_PROTO_KIND) {
        ProtoType *pty = (ProtoType *)ty;
        if (--pty->refcnt) return;
        free_desc(pty->ret);
        TypeDesc **param;
        vector_foreach(param, pty->params, { free_desc(*param); });
        vector_destroy(pty->params);
    } else {
        assert(0);
    }
    mm_free(ty);
}

void fini_desc(void)
{
    TypeDesc **ty;
    vector_foreach(ty, cache_types, { free_desc(*ty); });
    vector_destroy(cache_types);
}

#ifdef __cplusplus
}
#endif
