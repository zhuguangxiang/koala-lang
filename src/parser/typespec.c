/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2023 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

// clang-format off
#include "symbol.h"
#include "typespec.h"
#include "mm.h"
// clang-format on

#ifdef __cplusplus
extern "C" {
#endif

TypeSpec *int_type_spec(int width, int sign)
{
    TypeSpec *ts = mm_alloc_obj_fast(ts);
    ts->kind = TYPE_INT;
    ts->int_flt_info.width = width;
    ts->int_flt_info.sign = sign;
    return ts;
}

// check dst is compatible with src
int type_spec_is_compatible(TypeSpec *dst, TypeSpec *src)
{
    if (dst->kind != src->kind) return 0;

    if (dst->kind == TYPE_INT) {
        if (dst->int_flt_info.sign != src->int_flt_info.sign) return 0;
        if (dst->int_flt_info.width > src->int_flt_info.width) return 0;
        return 1;
    }

    assert(0); // not implemented.
    return 0;
}

int type_spec_to_str(TypeSpec *ts, Buffer *buf)
{
    if (!ts) return 0;

    if (ts->kind == TYPE_INT) {
        if (ts->int_flt_info.width == 1) {
            char ch = ts->int_flt_info.sign ? 'c' : 'C';
            buf_write_char(buf, ch);
        } else if (ts->int_flt_info.width == 2) {
            char ch = ts->int_flt_info.sign ? 's' : 'S';
            buf_write_char(buf, ch);
        } else if (ts->int_flt_info.width == 4) {
            char ch = ts->int_flt_info.sign ? 'i' : 'I';
            buf_write_char(buf, ch);
        } else {
            char ch = ts->int_flt_info.sign ? 'j' : 'J';
            buf_write_char(buf, ch);
        }
    }

    return 0;
}

void type_spec_print(TypeSpec *ts, Buffer *buf)
{
    if (!ts) return;

    if (ts->kind == TYPE_INT) {
        if (!ts->int_flt_info.sign) {
            buf_write_char(buf, 'u');
        }
        buf_write_str(buf, "int");
        buf_write_int64(buf, ts->int_flt_info.width * 8);
    }
}

void type_spec_str_print(char *s, Buffer *buf)
{
    if (!s || s[0] == 0) {
        buf_write_str(buf, "unk");
        return;
    }

    int len = strlen(s);

    if (len == 1) {
        char ch = s[0];
        switch (ch) {
            case 'c':
                buf_write_str(buf, "int8");
                break;
            case 'C':
                buf_write_str(buf, "uint8");
                break;
            case 's':
                buf_write_str(buf, "int16");
                break;
            case 'S':
                buf_write_str(buf, "uint16");
                break;
            case 'i':
                buf_write_str(buf, "int32");
                break;
            case 'I':
                buf_write_str(buf, "uint32");
                break;
            case 'j':
                buf_write_str(buf, "int64");
                break;
            case 'J':
                buf_write_str(buf, "uint64");
                break;
            case 'f':
                buf_write_str(buf, "float");
                break;
            case 'z':
                buf_write_str(buf, "bool");
                break;
            default:
                break;
        }
        return;
    }
}

#ifdef __cplusplus
}
#endif
