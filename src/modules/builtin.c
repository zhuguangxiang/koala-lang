/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "bytebuf.h"
#include "bytesobj.h"
#include "listobj.h"
#include "rangeobj.h"
#include "tupleobj.h"
#include "vm.h"

#ifdef __cplusplus
extern "C" {
#endif

extern TValue *kl_stdout;
extern Object *buf_write_str_func;
extern Object *buf_flush_func;

static void stdout_write_str(Object *sobj)
{
    TValue args[2];
    args[0] = *kl_stdout;
    args[1] = obj_value(sobj);
    TValue callable = obj_value(buf_write_str_func);
    kl_do_call(&callable, args, 2);
}

static void stdout_flush()
{
    TValue callable = obj_value(buf_flush_func);
    TValue arg = *kl_stdout;
    kl_do_call_one_arg(&callable, &arg);
}

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
func print(objs ..., sep = ' ', end = '\n')
*/
static TValue builtin_print(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 3);
    Object *tuple = to_obj(&args[0]);
    Object *sep = to_obj(&args[1]);
    Object *end = to_obj(&args[2]);
    BUF(buf);

    TValue *items = TUPLE_ITEMS(tuple);
    int size = TUPLE_SIZE(tuple);
    for (int i = 0; i < size; ++i) {
        print_value(items + i, &buf);
        if (i < size - 1) {
            buf_write_nstr(&buf, STR_BUF(sep), STR_LEN(sep));
        } else {
            buf_write_nstr(&buf, STR_BUF(end), STR_LEN(end));
        }
    }

    Object *sobj = kl_new_nstr(BUF_STR(buf), BUF_LEN(buf));
    stdout_write_str(sobj);
    stdout_flush();

    FINI_BUF(buf);
    return none_value;
}

/*
func panic(msg str)
*/
static TValue builtin_panic(TValue *self, TValue *args, int nargs)
{
    KoalaState *ks = __ks();
    kl_trace_back(ks);

    BUF(buf);

    buf_write_str(&buf, "\nPanic: ");
    print_value(args, &buf);
    buf_write_str(&buf, "\n\n");

    Object *sobj = kl_new_nstr(BUF_STR(buf), BUF_LEN(buf));

    stdout_write_str(sobj);
    stdout_flush();

    kl_free_str(sobj);

    FINI_BUF(buf);

    exit(-1);
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
    { "print", builtin_print },
    { "panic", builtin_panic },
    { "format", kl_format },
    { "typeof", builtin_typeof },
    { NULL },
};

static TypeObject *builtin_types[] = {
    &type_type,   &none_type,  &bool_type, &str_type,   &exc_type,     &field_type,
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

#ifdef __cplusplus
}
#endif
