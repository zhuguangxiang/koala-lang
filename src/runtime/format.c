/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "buffer.h"
#include "common.h"
#include "tupleobj.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char fill;      // fill character (default: ' ')
    char align;     // '<', '>', '^'
    char sign;      // '+', '-', ' '
    char type;      // 'd','f','x','X','b','o' or '\0'
    int width;      // field width
    int precision;  // precision for floats
    char flag_zero; // zero padding flag
} FormatSpec;

static void parse_format_spec(const char *fmt, int *i, FormatSpec *spec)
{
    memset(spec, 0, sizeof(*spec));
    spec->fill = ' '; // default fill

    (*i) += 2; // skip "{:"

    // fill + align
    if (fmt[*i + 1] == '<' || fmt[*i + 1] == '>' || fmt[*i + 1] == '^') {
        spec->fill = fmt[*i];
        spec->align = fmt[*i + 1];
        (*i) += 2;
    } else if (fmt[*i] == '<' || fmt[*i] == '>' || fmt[*i] == '^') {
        spec->align = fmt[*i];
        (*i)++;
    }

    // sign
    if (fmt[*i] == '+' || fmt[*i] == '-' || fmt[*i] == ' ') {
        spec->sign = fmt[*i];
        (*i)++;
    }

    // zero padding
    if (fmt[*i] == '0') {
        spec->flag_zero = 1;
        spec->fill = '0';
        (*i)++;
    }

    // width
    while (isdigit((unsigned char)fmt[*i])) {
        spec->width = spec->width * 10 + (fmt[*i] - '0');
        (*i)++;
    }

    // precision
    if (fmt[*i] == '.') {
        (*i)++;
        while (isdigit((unsigned char)fmt[*i])) {
            spec->precision = spec->precision * 10 + (fmt[*i] - '0');
            (*i)++;
        }
    }

    // type
    if (fmt[*i] != '}') {
        spec->type = fmt[*i];
        (*i)++;
    }

    if (fmt[*i] != '}') panic("FormatError: missing '}'");

    (*i)++;
}

static void format_int(Buffer *out, long v, const FormatSpec *spec)
{
    char tmp[128];
    int n;

    switch (spec->type) {
        case 'd':
            n = snprintf(tmp, sizeof(tmp), "%ld", v);
            break;
        case 'x':
            n = snprintf(tmp, sizeof(tmp), "%lx", v);
            break;
        case 'X':
            n = snprintf(tmp, sizeof(tmp), "%lX", v);
            break;
        case 'o':
            n = snprintf(tmp, sizeof(tmp), "%lo", v);
            break;
        case 'b': {
            n = 0;
            int started = 0;
            for (int i = 63; i >= 0; i--) {
                int bit = (v >> i) & 1;
                if (bit) started = 1;
                if (started || i == 0) tmp[n++] = bit ? '1' : '0';
            }
            break;
        }
        default:
            panic("TypeError: invalid integer format type");
    }

    int pad = spec->width > n ? spec->width - n : 0;

    if (spec->align == '<') {
        buf_write_nstr(out, tmp, n);
        for (int i = 0; i < pad; i++) buf_write_byte(out, spec->fill);
    } else if (spec->align == '^') {
        int left = pad / 2;
        int right = pad - left;
        for (int i = 0; i < left; i++) buf_write_byte(out, spec->fill);
        buf_write_nstr(out, tmp, n);
        for (int i = 0; i < right; i++) buf_write_byte(out, spec->fill);
    } else { // default: right align
        for (int i = 0; i < pad; i++) buf_write_byte(out, spec->fill);
        buf_write_nstr(out, tmp, n);
    }
}

static void format_float(Buffer *out, double v, const FormatSpec *spec)
{
    char fmt[16];
    char tmp[128];

    if (spec->precision > 0)
        snprintf(fmt, sizeof(fmt), "%%.%df", spec->precision);
    else
        strcpy(fmt, "%f");

    int n = snprintf(tmp, sizeof(tmp), fmt, v);
    int pad = spec->width > n ? spec->width - n : 0;

    if (spec->align == '<') {
        buf_write_nstr(out, tmp, n);
        for (int i = 0; i < pad; i++) buf_write_byte(out, spec->fill);
    } else if (spec->align == '^') {
        int left = pad / 2;
        int right = pad - left;
        for (int i = 0; i < left; i++) buf_write_byte(out, spec->fill);
        buf_write_nstr(out, tmp, n);
        for (int i = 0; i < right; i++) buf_write_byte(out, spec->fill);
    } else { // default: right align
        for (int i = 0; i < pad; i++) buf_write_byte(out, spec->fill);
        buf_write_nstr(out, tmp, n);
    }
}

static void format_str(Buffer *out, const char *s, int len, const FormatSpec *spec)
{
    int pad = spec->width > len ? spec->width - len : 0;

    if (spec->align == '<') {
        buf_write_nstr(out, s, len);
        for (int i = 0; i < pad; i++) buf_write_byte(out, spec->fill);
    } else if (spec->align == '^') {
        int left = pad / 2;
        int right = pad - left;
        for (int i = 0; i < left; i++) buf_write_byte(out, spec->fill);
        buf_write_nstr(out, s, len);
        for (int i = 0; i < right; i++) buf_write_byte(out, spec->fill);
    } else { // default: right align
        for (int i = 0; i < pad; i++) buf_write_byte(out, spec->fill);
        buf_write_nstr(out, s, len);
    }
}

void __kl_format__(Formatter *fmt_obj, const char *fmt, int len, Object *args)
{
    Buffer *out = &fmt_obj->buf;

    ASSERT(IS_TUPLE(args));
    TValue *items = TUPLE_ITEMS(args);
    int size = TUPLE_SIZE(args);

    Formatter *fmt_tmp = kl_new_formatter(32);

    int i = 0;
    int argi = 0;

    while (i < len) {
        // "{{" → "{"
        if (fmt[i] == '{' && fmt[i + 1] == '{') {
            buf_write_byte(out, '{');
            i += 2;
            continue;
        }

        // "}}" → "}"
        if (fmt[i] == '}' && fmt[i + 1] == '}') {
            buf_write_byte(out, '}');
            i += 2;
            continue;
        }

        // "{}" → call fmt()
        if (fmt[i] == '{' && fmt[i + 1] == '}') {
            if (argi >= size) panic("ArgumentError: not enough arguments for {}");

            TValue a = items[argi++];
            kl_fmt_clear(fmt_tmp);
            kl_fmt_call(fmt_tmp, &a);
            Object *s = kl_fmt_result(fmt_tmp);
            FormatSpec spec = { 0 };
            format_str(out, STR_BUF(s), STR_LEN(s), &spec);
            i += 2;
            continue;
        }

        // "{:...}" → formatted specifier
        if (fmt[i] == '{' && fmt[i + 1] == ':') {
            if (argi >= size) panic("ArgumentError: not enough arguments for format spec");

            FormatSpec spec;
            parse_format_spec(fmt, &i, &spec);

            TValue a = items[argi++];

            if (spec.type == 'd' || spec.type == 'x' || spec.type == 'X' || spec.type == 'b' ||
                spec.type == 'o') {
                if (!is_int(&a)) panic("TypeError: expected int for integer format");

                format_int(out, to_int64(&a), &spec);
            } else if (spec.type == 'f') {
                if (!is_float(&a)) panic("TypeError: expected float for float format");

                format_float(out, to_float64(&a), &spec);
            } else { // default: fmt()
                kl_fmt_clear(fmt_tmp);
                kl_fmt_call(fmt_tmp, &a);
                Object *s = kl_fmt_result(fmt_tmp);
                format_str(out, STR_BUF(s), STR_LEN(s), &spec);
            }

            continue;
        }

        // error on unescaped '{' or '}'
        if (fmt[i] == '{' || fmt[i] == '}') {
            panic("FormatError: unescaped '{' or '}'");
        }

        buf_write_byte(out, fmt[i]);
        i++;
    }

    kl_free_formatter(fmt_tmp);

    if (argi < size) panic("ArgumentError: too many arguments for format string");
}

static TValue _fmt_write_fmt(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs >= 1 && nargs <= 2);
    Formatter *fmt = SELF_AS(fmt_type);

    if (nargs == 1) {
        char *s = kl_arg_str(0);
        buf_write_str(&fmt->buf, s);
        return nil_value;
    }

    Object *sobj = kl_arg_strobj(0);
    Object *args_obj = kl_arg_obj(1);
    __kl_format__(fmt, STR_BUF(sobj), STR_LEN(sobj), args_obj);
    return nil_value;
}

static TValue _fmt_write_str(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    Formatter *fmt = SELF_AS(fmt_type);
    char *s = kl_arg_str(0);
    buf_write_str(&fmt->buf, s);
    return nil_value;
}

static TValue _fmt___int__(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    Formatter *fmt = SELF_AS(fmt_type);
    int64_t size = kl_arg_int64(0);
    buf_reserve(&fmt->buf, size);
    return nil_value;
}

static TValue _fmt_write_byte(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    Formatter *fmt = SELF_AS(fmt_type);
    uint8_t value = kl_arg_uint8(0);
    buf_write_byte(&fmt->buf, value);
    return nil_value;
}

static TValue _fmt_result(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    Formatter *fmt = SELF_AS(fmt_type);
    Object *res = kl_new_nstr(BUF_STR(fmt->buf), BUF_LEN(fmt->buf));
    return obj_value(res);
}

static TValue _fmt_clear(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    Formatter *fmt = SELF_AS(fmt_type);
    RESET_BUF(fmt->buf);
    return nil_value;
}

TypeObject fmt_type = {
    ._type = &type_type,
    .name = "Formatter",
    .flags = TP_FLAGS_CLASS,
    .priv_size = sizeof(Formatter),
    .methdefs =
        (MethodDef[]){
            { "write_fmt", _fmt_write_fmt },
            { "write_str", _fmt_write_str },
            { "write_byte", _fmt_write_byte },
            { "__init__", _fmt___int__ },
            { "result", _fmt_result },
            { "clear", _fmt_clear },
            { NULL, NULL },
        },
};

Formatter *kl_new_formatter(size_t size)
{
    Formatter *fmt = mm_alloc_obj(fmt);
    INIT_OBJECT_HEAD(fmt, &fmt_type, 0);
    buf_reserve(&fmt->buf, size);
    return fmt;
}

void kl_free_formatter(Formatter *fmt)
{
    ASSERT(OB_TYPE(fmt) == &fmt_type);
    FINI_BUF(fmt->buf);
    mm_free(fmt);
}

void kl_fmt_write_str(Formatter *fmt, char *s, size_t len)
{
    ASSERT(OB_TYPE(fmt) == &fmt_type);
    buf_write_nstr(&fmt->buf, s, len);
}

Object *kl_fmt_result(Formatter *fmt)
{
    ASSERT(OB_TYPE(fmt) == &fmt_type);
    return kl_new_nstr(BUF_STR(fmt->buf), BUF_LEN(fmt->buf));
}

void kl_fmt_clear(Formatter *fmt)
{
    ASSERT(OB_TYPE(fmt) == &fmt_type);
    RESET_BUF(fmt->buf);
}

#ifdef __cplusplus
}
#endif
