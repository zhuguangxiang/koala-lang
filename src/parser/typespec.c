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

TypeSpec *no_type_spec(void)
{
    TypeSpec *ts = mm_alloc_obj(ts);
    ts->kind = TYPE_NO_TYPE;
    return ts;
}

TypeSpec *int_type_spec(int width, int sign)
{
    TypeSpec *ts = mm_alloc_obj(ts);
    ts->kind = TYPE_INT;
    ts->int_flt_info.width = width;
    ts->int_flt_info.sign = sign;
    return ts;
}

TypeSpec *bool_type_spec(void)
{
    TypeSpec *ts = mm_alloc_obj(ts);
    ts->kind = TYPE_BOOL;
    return ts;
}

TypeSpec *str_type_spec(void)
{
    TypeSpec *ts = mm_alloc_obj(ts);
    ts->kind = TYPE_STR;
    return ts;
}

TypeSpec *object_type_spec(void)
{
    TypeSpec *ts = mm_alloc_obj(ts);
    ts->kind = TYPE_OBJECT;
    return ts;
}

TypeSpec *va_list_type_spec(void)
{
    TypeSpec *ts = mm_alloc_obj(ts);
    ts->kind = TYPE_VA_LIST;
    return ts;
}

TypeSpec *generic_var_type_spec(char *name, int index, void *sym_id)
{
    TypeSpec *ts = mm_alloc_obj(ts);
    ts->kind = TYPE_GENERIC_VAR;
    ts->generic_var.name = name;
    ts->generic_var.index = index;
    ts->sym_id = (intptr_t)sym_id;
    return ts;
}

TypeSpec *specialized_type_spec(char *full_pkg, char *name, Vector *args)
{
    TypeSpec *ts = mm_alloc_obj(ts);
    ts->kind = TYPE_SPECIALIZED;
    ts->specialized.pkg = full_pkg;
    ts->specialized.name = name;
    ts->specialized.args = args;
    return ts;
}

TypeSpec *unresolved_type_spec(TypeIdent *pkg, TypeIdent name, Vector *args)
{
    TypeSpec *ts = mm_alloc_obj(ts);
    ts->kind = TYPE_UNRESOLVED;
    if (pkg) {
        ts->unresolved.pkg = *pkg;
    }
    ts->unresolved.name = name;
    ts->unresolved.args = args;
    return ts;
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
    } else if (ts->kind == TYPE_STR) {
        buf_write_char(buf, 'u');
    } else if (ts->kind == TYPE_VA_LIST) {
        buf_write_str(buf, "...");
    } else if (ts->kind == TYPE_NO_TYPE) {
        // do-nothing
    } else if (ts->kind == TYPE_BOOL) {
        buf_write_char(buf, 'z');
    } else if (ts->kind == TYPE_OBJECT) {
        buf_write_char(buf, 'o');
    } else if (ts->kind == TYPE_GENERIC_VAR) {
        buf_write_char(buf, '<');
        buf_write_str(buf, ts->generic_var.name);
        buf_write_char(buf, ';');
    } else if (ts->kind == TYPE_SPECIALIZED) {
        buf_write_char(buf, 'L');
        buf_write_str(buf, ts->specialized.name);
        buf_write_char(buf, ';');
    } else {
        UNREACHABLE();
    }

    return 0;
}

TypeSpec *type_spec_from_str(const char *s)
{
    if (!s || s[0] == 0) return NULL;

    TypeSpec *ty = NULL;

    int len = strlen(s);

    if (len == 1) {
        int width;
        int sign;
        char ch = s[0];
        switch (ch) {
            case 'c':
                width = 1;
                sign = 1;
                break;
            case 'C':
                width = 1;
                sign = 0;
                break;
            case 's':
                width = 2;
                sign = 1;
                break;
            case 'S':
                width = 2;
                sign = 0;
                break;
            case 'i':
                width = 4;
                sign = 1;
                break;
            case 'I':
                width = 4;
                sign = 0;
                break;
            case 'j':
                width = 8;
                sign = 1;
                break;
            case 'J':
                width = 8;
                sign = 0;
                break;
            case 'u':
                return str_type_spec();
                break;
            case 'z':
                return bool_type_spec();
                break;
            case 'o':
                return object_type_spec();
                break;
            default:
                UNREACHABLE();
                break;
        }
        ty = int_type_spec(width, sign);
        return ty;
    }

    if (!strcmp(s, "...")) {
        return va_list_type_spec();
    }

    UNREACHABLE();
    return NULL;
}

void type_spec_print(TypeSpec *ts, Buffer *buf)
{
    if (!ts) return;

    if (ts->kind == TYPE_NO_TYPE) {
        buf_write_str(buf, "<NO-TYPE>");
    } else if (ts->kind == TYPE_INT) {
        if (!ts->int_flt_info.sign) {
            buf_write_char(buf, 'u');
        }
        buf_write_str(buf, "int");
        buf_write_int64(buf, ts->int_flt_info.width * 8);
    } else if (ts->kind == TYPE_STR) {
        buf_write_str(buf, "str");
    } else if (ts->kind == TYPE_VA_LIST) {
        buf_write_str(buf, "...");
    } else if (ts->kind == TYPE_BOOL) {
        buf_write_str(buf, "bool");
    } else if (ts->kind == TYPE_OBJECT) {
        buf_write_str(buf, "object");
    } else if (ts->kind == TYPE_UNRESOLVED) {
        buf_write_str(buf, ts->unresolved.name.name);
    } else if (ts->kind == TYPE_GENERIC_VAR) {
        buf_write_str(buf, ts->generic_var.name);
    } else if (ts->kind == TYPE_SPECIALIZED) {
        buf_write_str(buf, ts->specialized.name);
    } else {
        UNREACHABLE();
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
            case 'u':
                buf_write_str(buf, "str");
                break;
            case 'o':
                buf_write_str(buf, "object");
                break;
            default:
                break;
        }
        return;
    }

    if (!strcmp(s, "...")) {
        buf_write_str(buf, "...");
        return;
    }

    if (s[0] == '<') {
        char *_s = s + 1;
        while (*_s != ';') _s++;
        buf_write_nstr(buf, s + 1, _s - (s + 1));
        return;
    }

    if (s[0] == 'L') {
        char *_s = s + 1;
        while (*_s != ';') _s++;
        buf_write_nstr(buf, s + 1, _s - (s + 1));
        return;
    }

    UNREACHABLE();
}

#ifdef __cplusplus
}
#endif
