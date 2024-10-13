/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2023 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "typedesc.h"

#ifdef __cplusplus
extern "C" {
#endif

TypeDesc int_desc = { 1, TYPE_INT_KIND };
TypeDesc float_desc = { 1, TYPE_FLOAT_KIND };
TypeDesc bool_desc = { 1, TYPE_BOOL_KIND };
TypeDesc str_desc = { 1, TYPE_STR_KIND };
TypeDesc object_desc = { 1, TYPE_OBJECT_KIND };
TypeDesc bytes_desc = { 1, TYPE_BYTES_KIND };
TypeDesc type_desc = { 1, TYPE_TYPE_KIND };
TypeDesc range_desc = { 1, TYPE_RANGE_KIND };

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

    if (a->kind != b->kind) return 0;
    return 1;
}

static void int_desc_to_str(TypeDesc *ty, Buffer *buf) { buf_write_char(buf, 'i'); }

static void float_desc_to_str(TypeDesc *ty, Buffer *buf) { buf_write_char(buf, 'f'); }

int desc_to_str(TypeDesc *ty, Buffer *buf)
{
    switch (ty->kind) {
        case TYPE_INT_KIND:
            int_desc_to_str(ty, buf);
            break;
        case TYPE_FLOAT_KIND:
            float_desc_to_str(ty, buf);
            break;
        case TYPE_OPTIONAL_KIND: {
            buf_write_char(buf, '?');
            OptionalDesc *opt = (OptionalDesc *)ty;
            desc_to_str(opt->type, buf);
            break;
        }
        default:
            UNREACHABLE();
            break;
    }
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
        default: {
            buf_write_str(buf, "unk");
            break;
        }
    }
}

#ifdef __cplusplus
}
#endif
