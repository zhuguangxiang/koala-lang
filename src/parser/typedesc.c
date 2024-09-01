/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2023 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "typedesc.h"

#ifdef __cplusplus
extern "C" {
#endif

NumberDesc int8_desc = { 1, TYPE_INT_KIND, 8 };
NumberDesc int16_desc = { 1, TYPE_INT_KIND, 16 };
NumberDesc int32_desc = { 1, TYPE_INT_KIND, 32 };
NumberDesc int64_desc = { 1, TYPE_INT_KIND, 64 };
NumberDesc int128_desc = { 1, TYPE_INT_KIND, 128 };
NumberDesc float32_desc = { 1, TYPE_FLOAT_KIND, 32 };
NumberDesc float64_desc = { 1, TYPE_FLOAT_KIND, 64 };
NumberDesc float128_desc = { 1, TYPE_FLOAT_KIND, 128 };
TypeDesc bool_desc = { 1, TYPE_BOOL_KIND };
TypeDesc str_desc = { 1, TYPE_STR_KIND };
TypeDesc object_desc = { 1, TYPE_OBJECT_KIND };
TypeDesc char_desc = { 1, TYPE_CHAR_KIND };
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

void desc_to_str(TypeDesc *desc, Buffer *buf)
{
    if (!desc) {
        buf_write_str(buf, "unk");
        return;
    }

    switch (desc->kind) {
        case TYPE_INT_KIND: {
            NumberDesc *num = (NumberDesc *)desc;
            char s[8] = { 0 };
            sprintf(s, "int%d", num->width);
            buf_write_str(buf, s);
            break;
        }
        case TYPE_FLOAT_KIND: {
            NumberDesc *num = (NumberDesc *)desc;
            char s[16] = { 0 };
            sprintf(s, "float%d", num->width);
            buf_write_str(buf, s);
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
