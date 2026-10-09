/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "bytebuf.h"
#include "bytesobj.h"
#include "excobj.h"
#include "listobj.h"
#include "rangeobj.h"
#include "tupleobj.h"

#ifdef __cplusplus
extern "C" {
#endif

static void print_value(TValue *val, Buffer *buf)
{
    if (is_ref(val)) {
        Object *obj = to_obj(val);
        if (IS_STR(obj)) {
            buf_write_nstr(buf, STR_BUF(obj), STR_LEN(obj));
            return;
        }
    }

    Object *sobj = kl_to_str(val);
    buf_write_nstr(buf, STR_BUF(sobj), STR_LEN(sobj));
}

/*
func print_intern(w io.Writer, _sep str, _end str, objs ...) {}
*/
TValue do_print(TValue *args, int nargs)
{
    ASSERT(nargs == 4);
    TValue writer = args[0];
    Object *sep = kl_arg_strobj(1);
    Object *end = kl_arg_strobj(2);
    Object *upper_tuple = kl_arg_tuple(3);

    // TODO: double tuple wrapper
    TValue *upper_items = TUPLE_ITEMS(upper_tuple);
    ASSERT(TUPLE_SIZE(upper_tuple) == 1);

    Object *tuple = to_obj(upper_items);
    ASSERT(IS_TUPLE(tuple));
    TValue *items = TUPLE_ITEMS(tuple);
    int size = TUPLE_SIZE(tuple);

    Formatter *fmt = kl_new_formatter(64);

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

    return nil_value;
}

/*
func panic(msg str)
*/
static TValue builtin_panic(TValue *self, TValue *args, int nargs)
{
    BUF(buf);
    print_value(args, &buf);
    raise_exc_str(BUF_STR(buf));
    FINI_BUF(buf);
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

static MethodDef builtin_functions[] = {
    { "panic", builtin_panic },
    { "format", builtin_format },
    { "typeof", builtin_typeof },
    { NULL },
};

static TypeObject *builtin_types[] = {
    &type_type,    &nil_type,     &bool_type,   &str_type,    &exc_type,    &field_type,
    &global_type,  &cfunc_type,   &code_type,   &int8_type,   &int16_type,  &int32_type,
    &int64_type,   &uint8_type,   &uint16_type, &uint32_type, &uint64_type, &float16_type,
    &float32_type, &float64_type, &tuple_type,  &range_type,  &slice_type,  &list_type,
    &bytes_type,   &bytebuf_type, &fmt_type,
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
