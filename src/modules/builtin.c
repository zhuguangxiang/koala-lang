/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include <math.h>
#include "bytebuf.h"
#include "bytesobj.h"
#include "dictobj.h"
#include "excobj.h"
#include "hashset.h"
#include "listobj.h"
#include "rangeobj.h"
#include "tupleobj.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
func print_intern(w io.Writer, _sep str, _end str, objs ...) {}
*/
TValue do_print(TValue *args, int nargs)
{
    ASSERT(nargs == 4);
    TValue writer = args[0];
    Object *sep = kl_arg_strobj(1);
    Object *end = kl_arg_strobj(2);
    Object *tuple = kl_arg_tuple(3);
    TValue *items = TUPLE_ITEMS(tuple);
    int size = TUPLE_SIZE(tuple);

    Formatter *fmt = kl_new_formatter(64);

    if (size == 0) {
        // print() with no objects writes only `end`
        kl_fmt_write_str(fmt, STR_BUF(end), STR_LEN(end));
    }

    for (int i = 0; i < size; ++i) {
        kl_call_fmt(items + i, fmt);
        if (i < size - 1) {
            kl_fmt_write_str(fmt, STR_BUF(sep), STR_LEN(sep));
        } else {
            kl_fmt_write_str(fmt, STR_BUF(end), STR_LEN(end));
        }
    }

    Object *s = kl_fmt_result(fmt);

    // write_str(s str) int
    TValue _args[] = { writer, obj_value(s) };
    TValue ret = kl_call_intf(_args, 2, 1);
    if (is_error(&ret)) {
        return ret;
    }

    // flush() int
    ret = kl_call_intf(&writer, 1, 2);
    if (is_error(&ret)) {
        return ret;
    }

    kl_free_str(s);
    kl_free_formatter(fmt);

    return no_value;
}

/*
func panic(msg str)
*/
static TValue builtin_panic(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    char *s = kl_arg_str(0);
    raise_exc_str(s);
    return error_value;
}

/*
func UNREACHABLE()
*/
static TValue builtin_unreachable(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    raise_exc_str("unreachable");
    return error_value;
}

void __kl_format__(Formatter *out, const char *fmt, int len, Object *args);

static TValue builtin_format(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs >= 1 && nargs <= 2);

    if (nargs == 1) {
        // return the format string itself if no arguments provided
        return args[0];
    }

    const char *f = kl_arg_str(0);
    int flen = strlen(f);
    Object *tuple = kl_arg_obj_as(1, tuple_type);

    Formatter *out = kl_new_formatter(32);
    __kl_format__(out, f, flen, tuple);
    Object *so = kl_new_nstr(BUF_STR(out->buf), BUF_LEN(out->buf));
    kl_free_formatter(out);
    return obj_value(so);
}

static TValue builtin_typeof(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    TypeObject *tp = kl_typeof(&args[0]);
    Object *tp_obj = (Object *)tp;
    return obj_value(tp_obj);
}

/*
func pow_int64_int64(base int, exp int) int

Specialization of the generic pow() for an int64 base and exponent.
Binary exponentiation performed entirely in uint64_t so that overflow wraps
deterministically (no signed-overflow UB), consistent with int64.pow().

A negative exponent yields the truncation toward zero of the exact
mathematical result: `1` for base 1, `±1` for base -1 (by parity), and `0`
for every other base (|base| >= 2 or base == 0).
*/
static TValue builtin_pow_int64_int64(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    int64_t base = kl_arg_int(0);
    int64_t exp = kl_arg_int(1);

    if (exp < 0) {
        if (base == 1) return int64_value(1);
        if (base == -1) return int64_value((exp & 1) ? -1 : 1);
        return int64_value(0);
    }

    uint64_t b = (uint64_t)base;
    uint64_t e = (uint64_t)exp;
    uint64_t r = 1;
    while (e) {
        if (e & 1) r *= b;
        b *= b;
        e >>= 1;
    }
    return int64_value((int64_t)r);
}

/*
func pow_float64_int64(base float, exp int) float

Specialization of the generic pow() for a float64 base and an int64 exponent.
Fast exponentiation by squaring; a negative exponent takes the reciprocal of
the positive-exponent result (base 0.0 then naturally yields ±inf per IEEE
754). The negation is done in uint64_t so that exp == INT64_MIN is safe.
*/
static TValue builtin_pow_float64_int64(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    double base = kl_arg_float(0);
    int64_t exp = kl_arg_int(1);

    uint64_t e = exp < 0 ? (uint64_t)(-(exp + 1)) + 1 : (uint64_t)exp;
    double b = base;
    double r = 1.0;
    while (e) {
        if (e & 1) r *= b;
        b *= b;
        e >>= 1;
    }
    if (exp < 0) r = 1.0 / r;
    return float64_value(r);
}

/*
func pow_float64_float64(base float, exp float) float

Specialization of the generic pow() for float64 base and exponent.
Routes directly to the C math library's pow().
*/
static TValue builtin_pow_float64_float64(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    double base = kl_arg_float(0);
    double exp = kl_arg_float(1);
    return float64_value(pow(base, exp));
}

/*
func round(x float) float

Return `x` rounded to the nearest integer value (as a float).
Routes directly to the C math library's round().
*/
static TValue builtin_round(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    double x = kl_arg_float(0);
    return float64_value(round(x));
}

/*
func floor(x float) float

Return the largest integer value less than or equal to `x` (as a float).
Routes directly to the C math library's floor().
*/
static TValue builtin_floor(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    double x = kl_arg_float(0);
    return float64_value(floor(x));
}

/*
func ceil(x float) float

Return the smallest integer value greater than or equal to `x` (as a float).
Routes directly to the C math library's ceil().
*/
static TValue builtin_ceil(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    double x = kl_arg_float(0);
    return float64_value(ceil(x));
}

static MethodDef builtin_functions[] = {
    { "panic", builtin_panic },
    { "unreachable", builtin_unreachable },
    { "format", builtin_format },
    { "typeof", builtin_typeof },
    { "pow_int64_int64", builtin_pow_int64_int64 },
    { "pow_float64_int64", builtin_pow_float64_int64 },
    { "pow_float64_float64", builtin_pow_float64_float64 },
    { "round", builtin_round },
    { "floor", builtin_floor },
    { "ceil", builtin_ceil },
    { NULL },
};

static TypeObject *builtin_types[] = {
    &type_type,
    &nil_type,
    &bool_type,
    &str_type,
    &exc_type,
    &field_type,
    &global_type,
    &cfunc_type,
    &code_type,
    &int8_type,
    &int16_type,
    &int32_type,
    &int64_type,
    &uint8_type,
    &uint16_type,
    &uint32_type,
    &uint64_type,
    &float16_type,
    &float32_type,
    &float64_type,
    &tuple_type,
    &range_type,
    &slice_type,
    &list_type,
    &bytes_type,
    &bytebuf_type,
    &fmt_type,
    &dict_type,
    &dict_keys_iter_type,
    &dict_values_iter_type,
    &dict_view_iter_type,
    &dict_rev_iter_type,
    &hashset_type,
    &hashset_iter_type,
};

void builtin_native_lib_init(NativeLib *lib)
{
    MethodDef *methdef = builtin_functions;
    while (methdef->name) {
        kl_reg_func(lib, methdef->name, methdef->cfunc);
        ++methdef;
    }

    for (int i = 0; i < COUNT_OF(builtin_types); ++i) {
        kl_reg_type(lib, builtin_types[i]);
    }
}

#ifdef __cplusplus
}
#endif
