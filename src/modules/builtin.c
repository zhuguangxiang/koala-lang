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
static TValue print_intern(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 4);
    TValue writer = args[0];
    Object *sep = to_obj(&args[1]);
    Object *end = to_obj(&args[2]);
    Object *upper_tuple = to_obj(&args[3]);

    TValue *upper_items = TUPLE_ITEMS(upper_tuple);
    ASSERT(TUPLE_SIZE(upper_tuple) == 1);

    Object *tuple = to_obj(upper_items);
    TValue *items = TUPLE_ITEMS(tuple);
    int size = TUPLE_SIZE(tuple);

    BUF(buf);

    for (int i = 0; i < size; ++i) {
        print_value(items + i, &buf);
        if (i < size - 1) {
            buf_write_nstr(&buf, STR_BUF(sep), STR_LEN(sep));
        } else {
            buf_write_nstr(&buf, STR_BUF(end), STR_LEN(end));
        }
    }

    Object *sobj = kl_new_nstr(BUF_STR(buf), BUF_LEN(buf));

    // write_str(s str) int
    Object *write_str_fn = kl_get_intf_func(&writer, 1);
    TValue _args[] = { writer, obj_value(sobj) };
    kl_object_call(write_str_fn, _args, 2);

    // flush() int
    Object *flush_fn = kl_get_intf_func(&writer, 2);
    kl_object_call(flush_fn, &writer, 1);

    FINI_BUF(buf);
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

TValue kl_format(TValue *self, TValue *args, int nargs);

static TValue builtin_typeof(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    TypeObject *tp = kl_typeof(&args[0]);
    Object *tp_obj = (Object *)tp;
    return obj_value(tp_obj);
}

static MethodDef builtin_functions[] = {
    { "panic", builtin_panic },
    { "format", kl_format },
    { "typeof", builtin_typeof },
    { NULL },
};

static TypeObject *builtin_types[] = {
    &type_type,   &nil_type,   &bool_type, &str_type,   &exc_type,     &field_type,
    &global_type, &cfunc_type, &code_type, &int_type,   &float_type,   &tuple_type,
    &range_type,  &slice_type, &list_type, &bytes_type, &ByteBuf_type,
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

static MethodDef print_lib_functions[] = {
    { "print_intern", print_intern },
    { NULL },
};

void print_native_lib_init(NativeLib *lib)
{
    MethodDef *methdef = print_lib_functions;
    while (methdef->name) {
        kl_reg_func(lib, methdef->name, methdef->cfunc);
        ++methdef;
    }
}

#ifdef __cplusplus
}
#endif
