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

TValue kl_format(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs >= 1 && nargs <= 2);

    if (nargs == 1) {
        Object *fmt_obj = kl_arg_obj(0);
        ASSERT(IS_STR(fmt_obj));
        return args[0]; // return the format string itself if no arguments provided
    }

    TupleObject *tuple = kl_arg_obj_as(1, tuple_type);
    TValue *items = TUPLE_ITEMS(tuple);
    int size = TUPLE_SIZE(tuple);

    const char *f = kl_arg_str(0);
    int flen = strlen(f);

    BUF(out);

    int i = 0;
    int argi = 0;

    while (i < flen) {
        // "{{" → "{"
        if (f[i] == '{' && f[i + 1] == '{') {
            buf_write_byte(&out, '{');
            i += 2;
            continue;
        }

        // "}}" → "}"
        if (f[i] == '}' && f[i + 1] == '}') {
            buf_write_byte(&out, '}');
            i += 2;
            continue;
        }

        // "{}" → call __str__()
        if (f[i] == '{' && f[i + 1] == '}') {
            if (argi >= size) panic("ArgumentError: not enough arguments for {}");

            TValue a = items[argi++];
            Object *s = kl_to_str(&a);
            FormatSpec spec = { 0 };
            format_str(&out, STR_BUF(s), STR_LEN(s), &spec);
            i += 2;
            continue;
        }

        // "{:...}" → formatted specifier
        if (f[i] == '{' && f[i + 1] == ':') {
            if (argi >= size) panic("ArgumentError: not enough arguments for format spec");

            FormatSpec spec;
            parse_format_spec(f, &i, &spec);

            TValue a = items[argi++];

            if (spec.type == 'd' || spec.type == 'x' || spec.type == 'X' || spec.type == 'b' ||
                spec.type == 'o') {
                if (!is_int(&a)) panic("TypeError: expected int for integer format");

                format_int(&out, to_int64(&a), &spec);
            } else if (spec.type == 'f') {
                if (!is_float(&a)) panic("TypeError: expected float for float format");

                format_float(&out, to_float64(&a), &spec);
            } else { // default: __str__()
                Object *s = kl_to_str(&a);
                format_str(&out, STR_BUF(s), STR_LEN(s), &spec);
            }

            continue;
        }

        // error on unescaped '{' or '}'
        if (f[i] == '{' || f[i] == '}') {
            panic("FormatError: unescaped '{' or '}'");
        }

        buf_write_byte(&out, f[i]);
        i++;
    }

    if (argi < size) panic("ArgumentError: too many arguments for format string");

    Object *so = kl_new_nstr(BUF_STR(out), BUF_LEN(out));
    FINI_BUF(out);
    return obj_value(so);
}

#ifdef __cplusplus
}
#endif
