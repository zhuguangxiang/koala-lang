/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2018-2022 James <zhuguangxiang@gmail.com>
 */

#include "typedesc.h"

#ifdef __cplusplus
extern "C" {
#endif

static TypeDesc bases[] = {
    { TYPE_UNK_KIND },  { TYPE_ANY_KIND },  { TYPE_U8_KIND },
    { TYPE_U16_KIND },  { TYPE_U32_KIND },  { TYPE_U64_KIND },
    { TYPE_I8_KIND },   { TYPE_I16_KIND },  { TYPE_I32_KIND },
    { TYPE_I64_KIND },  { TYPE_F32_KIND },  { TYPE_F64_KIND },
    { TYPE_BOOL_KIND }, { TYPE_CHAR_KIND }, { TYPE_STR_KIND },
};

TypeDesc *desc_from_unk(void)
{
    return &bases[TYPE_UNK_KIND];
}

TypeDesc *desc_from_any(void)
{
    return &bases[TYPE_ANY_KIND];
}

TypeDesc *desc_from_uint8(void)
{
    return &bases[TYPE_U8_KIND];
}

TypeDesc *desc_from_uint16(void)
{
    return &bases[TYPE_U16_KIND];
}

TypeDesc *desc_from_uint32(void)
{
    return &bases[TYPE_U32_KIND];
}

TypeDesc *desc_from_uint64(void)
{
    return &bases[TYPE_U64_KIND];
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

TypeDesc *desc_from_ptr(TypeDesc *ty)
{
    PointerType *ptr = mm_alloc_obj(ptr);
    ptr->kind = TYPE_PTR_KIND;
    ptr->ty = ty;
    return (TypeDesc *)ptr;
}

static ArrayType uint8_array = { TYPE_ARRAY_KIND, &bases[TYPE_U8_KIND] };
static ArrayType uint16_array = { TYPE_ARRAY_KIND, &bases[TYPE_U16_KIND] };
static ArrayType uint32_array = { TYPE_ARRAY_KIND, &bases[TYPE_U32_KIND] };
static ArrayType uint64_array = { TYPE_ARRAY_KIND, &bases[TYPE_U64_KIND] };
static ArrayType int8_array = { TYPE_ARRAY_KIND, &bases[TYPE_I8_KIND] };
static ArrayType int16_array = { TYPE_ARRAY_KIND, &bases[TYPE_I16_KIND] };
static ArrayType int32_array = { TYPE_ARRAY_KIND, &bases[TYPE_I32_KIND] };
static ArrayType int64_array = { TYPE_ARRAY_KIND, &bases[TYPE_I64_KIND] };

TypeDesc *desc_from_array(TypeDesc *ty)
{
    if (ty->kind == TYPE_U8_KIND) return (TypeDesc *)&uint8_array;
    if (ty->kind == TYPE_U16_KIND) return (TypeDesc *)&uint16_array;
    if (ty->kind == TYPE_U32_KIND) return (TypeDesc *)&uint32_array;
    if (ty->kind == TYPE_U64_KIND) return (TypeDesc *)&uint64_array;
    if (ty->kind == TYPE_I8_KIND) return (TypeDesc *)&int8_array;
    if (ty->kind == TYPE_I16_KIND) return (TypeDesc *)&int16_array;
    if (ty->kind == TYPE_I32_KIND) return (TypeDesc *)&int32_array;
    if (ty->kind == TYPE_I64_KIND) return (TypeDesc *)&int64_array;

    ArrayType *arr = mm_alloc_obj(arr);
    arr->kind = TYPE_ARRAY_KIND;
    arr->ty = ty;
    return (TypeDesc *)arr;
}

TypeDesc *desc_from_map(TypeDesc *kty, TypeDesc *vty)
{
    MapType *map = mm_alloc_obj(map);
    map->kind = TYPE_MAP_KIND;
    map->kty = kty;
    map->vty = vty;
    return (TypeDesc *)map;
}

TypeDesc *desc_from_klass(char *pkg, char *name, Vector *args)
{
    KlassType *kls = mm_alloc_obj(kls);
    kls->kind = TYPE_KLASS_KIND;
    kls->pkg = pkg;
    kls->name = name;
    kls->type_params = args;
    return (TypeDesc *)kls;
}

static ProtoType void_proto = { TYPE_PROTO_KIND, NULL, NULL };

TypeDesc *desc_from_proto(TypeDesc *ret, Vector *params)
{
    if (!ret && !params) return (TypeDesc *)&void_proto;

    ProtoType *pty = mm_alloc_obj(pty);
    pty->kind = TYPE_PROTO_KIND;
    pty->ret = ret;
    pty->params = params;
    return (TypeDesc *)pty;
}

int desc_is_numb(TypeDesc *ty)
{
    if (ty->kind == TYPE_U8_KIND) return 1;
    if (ty->kind == TYPE_U16_KIND) return 1;
    if (ty->kind == TYPE_U32_KIND) return 1;
    if (ty->kind == TYPE_U64_KIND) return 1;
    if (ty->kind == TYPE_I8_KIND) return 1;
    if (ty->kind == TYPE_I16_KIND) return 1;
    if (ty->kind == TYPE_I32_KIND) return 1;
    if (ty->kind == TYPE_I64_KIND) return 1;
    if (ty->kind == TYPE_F32_KIND) return 1;
    if (ty->kind == TYPE_F64_KIND) return 1;
    return 0;
}

int desc_is_int(TypeDesc *ty)
{
    if (ty->kind == TYPE_U8_KIND) return 1;
    if (ty->kind == TYPE_U16_KIND) return 1;
    if (ty->kind == TYPE_U32_KIND) return 1;
    if (ty->kind == TYPE_U64_KIND) return 1;
    if (ty->kind == TYPE_I8_KIND) return 1;
    if (ty->kind == TYPE_I16_KIND) return 1;
    if (ty->kind == TYPE_I32_KIND) return 1;
    if (ty->kind == TYPE_I64_KIND) return 1;
    return 0;
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

    if (ty == (TypeDesc *)&void_proto) {
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
        "Unk",     "Any",     "UInt8", "UInt16", "UInt32",
        "UInt64",  "Int8",    "Int16", "Int32",  "Int64",
        "Float32", "Float64", "Bool",  "Char",   "String",
    };

    if (ty->kind >= TYPE_ANY_KIND && ty->kind <= TYPE_STR_KIND) {
        buf_write_str(buf, strs[ty->kind]);
        return;
    }

    if (ty->kind == TYPE_KLASS_KIND) {
        KlassType *kls = (KlassType *)ty;
        if (kls->pkg) {
            buf_write_str(buf, kls->pkg);
            buf_write_char(buf, '.');
        }
        buf_write_str(buf, kls->name);
        if (kls->type_params) {
            buf_write_char(buf, '<');
            TypeDesc **item;
            vector_foreach(item, kls->type_params, {
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

void free_desc(TypeDesc *ty)
{
    if (!ty) return;

    int kind = ty->kind;
    if (kind >= TYPE_UNK_KIND && kind <= TYPE_STR_KIND) return;
    if (kind == TYPE_ARRAY_KIND) return;

    mm_free(ty);
}

#ifdef __cplusplus
}
#endif
