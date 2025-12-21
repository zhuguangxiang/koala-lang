/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2023 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "typedesc.h"
#include "atom.h"

#ifdef __cplusplus
extern "C" {
#endif

TypeDesc no_type_desc = { 1, TYPE_NO_TYPE_KIND };
TypeDesc int_desc = { 1, TYPE_INT_KIND };
TypeDesc float_desc = { 1, TYPE_FLOAT_KIND };
TypeDesc bool_desc = { 1, TYPE_BOOL_KIND };
TypeDesc str_desc = { 1, TYPE_STR_KIND };
TypeDesc object_desc = { 1, TYPE_OBJECT_KIND };
TypeDesc bytes_desc = { 1, TYPE_BYTES_KIND };
TypeDesc type_desc = { 1, TYPE_TYPE_KIND };
TypeDesc range_desc = { 1, TYPE_RANGE_KIND };
TypeDesc valist_desc = { 1, TYPE_VA_LIST_KIND };

TypeDesc *desc_optional(TypeDesc *ty)
{
    OptionalDesc *opt = mm_alloc_obj_fast(opt);
    opt->refcnt = 1;
    opt->kind = TYPE_OPTIONAL_KIND;
    opt->type = DESC_INCREF_GET(ty);
    return (TypeDesc *)opt;
}

TypeDesc *desc_array(TypeDesc *sub)
{
    ArrayDesc *arr = mm_alloc_obj_fast(arr);
    arr->refcnt = 1;
    arr->kind = TYPE_ARRAY_KIND;
    if (!sub) {
        arr->type = desc_object();
    } else {
        arr->type = DESC_INCREF_GET(sub);
    }
    return (TypeDesc *)arr;
}

TypeDesc *desc_klass(char *m, char *id, Vector *params)
{
    KlassDesc *kls = mm_alloc_obj_fast(kls);
    kls->refcnt = 1;
    kls->kind = TYPE_KLASS_KIND;
    kls->module = m;
    kls->symbol = id;
    kls->params = params;
    return (TypeDesc *)kls;
}

TypeDesc *desc_proto(TypeDesc *ret, Vector *params)
{
    ProtoDesc *proto = mm_alloc_obj_fast(proto);
    proto->refcnt = 1;
    proto->kind = TYPE_PROTO_KIND;
    if (ret) DESC_INCREF(ret);
    proto->ret = ret;
    proto->params = params;
    return (TypeDesc *)proto;
}

TypeDesc *desc_enum(void)
{
    EnumDesc *e = mm_alloc_obj_fast(e);
    e->refcnt = 1;
    e->kind = TYPE_ENUM_KIND;
    return (TypeDesc *)e;
}

void free_desc(TypeDesc *ty)
{
    if (!ty) return;
    switch (ty->kind) {
        case TYPE_INT_KIND:
            break;

        default:
            break;
    }
    --ty->refcnt;
    if (ty->refcnt <= 0) {
        mm_free(ty);
    }
}

int desc_equal(TypeDesc *a, TypeDesc *b)
{
    if (a == b) return 1;

    if (a == &object_desc) return 1;

    if (a->kind == TYPE_OPTIONAL_KIND) {
        OptionalDesc *opt = (OptionalDesc *)a;
        return desc_equal(opt->type, b);
    }

    if (a->kind != b->kind) return 0;
    return 1;
}

int desc_to_str(TypeDesc *ty, Buffer *buf)
{
    if (!ty) return 0;

    switch (ty->kind) {
        case TYPE_INT_KIND: {
            buf_write_char(buf, 'i');
            break;
        }
        case TYPE_FLOAT_KIND: {
            buf_write_char(buf, 'f');
            break;
        }
        case TYPE_STR_KIND: {
            buf_write_char(buf, 's');
            break;
        }
        case TYPE_OPTIONAL_KIND: {
            buf_write_char(buf, '?');
            OptionalDesc *opt = (OptionalDesc *)ty;
            desc_to_str(opt->type, buf);
            break;
        }
        case TYPE_BOOL_KIND: {
            buf_write_char(buf, 'z');
            break;
        }
        case TYPE_VA_LIST_KIND: {
            buf_write_str(buf, "...");
            break;
        }
        case TYPE_NO_TYPE_KIND: {
            // do-nothing
            break;
        }
        case TYPE_ARRAY_KIND: {
            buf_write_char(buf, '[');
            ArrayDesc *arr = (ArrayDesc *)ty;
            desc_to_str(arr->type, buf);
            break;
        }
        default: {
            UNREACHABLE();
            break;
        }
    }

    return 0;
}

TypeDesc *desc_from_str(const char *s)
{
    if (!s || s[0] == 0) return desc_no_type();

    TypeDesc *ty = NULL;

    int len = strlen(s);
    if (len == 1) {
        char ch = s[0];
        switch (ch) {
            case 'i':
                ty = desc_int();
                break;
            case 'f':
                ty = desc_float();
                break;
            case 's':
                ty = desc_str();
                break;
            case 'z':
                ty = desc_bool();
                break;
            default:
                break;
        }
        return ty;
    }

    if (s[0] == '?') {
        TypeDesc *sub = desc_from_str(s + 1);
        ty = desc_optional(sub);
        return ty;
    }

    if (s[0] == '[') {
        TypeDesc *sub = desc_from_str(s + 1);
        ty = desc_array(sub);
        return ty;
    }

    return ty;
}

void desc_print(TypeDesc *desc, Buffer *buf)
{
    if (!desc) {
        buf_write_str(buf, "unk");
        return;
    }

    switch (desc->kind) {
        case TYPE_INT_KIND: {
            buf_write_str(buf, "int");
            break;
        }
        case TYPE_FLOAT_KIND: {
            buf_write_str(buf, "float");
            break;
        }
        case TYPE_BOOL_KIND: {
            buf_write_str(buf, "bool");
            break;
        }
        case TYPE_STR_KIND: {
            buf_write_str(buf, "str");
            break;
        }
        case TYPE_OBJECT_KIND: {
            buf_write_str(buf, "object");
            break;
        }
        case TYPE_OPTIONAL_KIND: {
            OptionalDesc *opt = (OptionalDesc *)desc;
            desc_print(opt->type, buf);
            buf_write_char(buf, '?');
            break;
        }
        case TYPE_VA_LIST_KIND: {
            buf_write_str(buf, "...");
            break;
        }
        case TYPE_NO_TYPE_KIND: {
            buf_write_str(buf, "no-type");
            break;
        }
        case TYPE_ARRAY_KIND: {
            buf_write_char(buf, '[');
            ArrayDesc *arr = (ArrayDesc *)desc;
            desc_print(arr->type, buf);
            buf_write_char(buf, ']');
            break;
        }
        default: {
            UNREACHABLE();
            break;
        }
    }
}

void desc_str_print(char *s, Buffer *buf)
{
    if (!s || s[0] == 0) {
        buf_write_str(buf, "unk");
        return;
    }

    int len = strlen(s);
    if (len == 1) {
        char ch = s[0];
        switch (ch) {
            case 'i':
                buf_write_str(buf, "int");
                break;
            case 'f':
                buf_write_str(buf, "float");
                break;
            case 's':
                buf_write_str(buf, "str");
                break;
            case 'z':
                buf_write_str(buf, "bool");
                break;
            default:
                break;
        }
        return;
    }

    if (s[0] == '?') {
        buf_write_char(buf, '?');
        desc_str_print(s + 1, buf);
        return;
    }

    if (s[0] == '[') {
        buf_write_char(buf, '[');
        desc_str_print(s + 1, buf);
        buf_write_char(buf, ']');
        return;
    }

    buf_write_str(buf, s);
}

#ifdef __cplusplus
}
#endif
