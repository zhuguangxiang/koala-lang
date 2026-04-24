/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "buffer.h"
#include "modobj.h"

#ifdef __cplusplus
extern "C" {
#endif

static void print_value(TValue *val)
{
    if (is_int(val)) {
        printf("%" PRId64 " ", val->ival);
    } else if (is_uint(val)) {
        printf("%" PRIu64 " ", val->ival);
    } else if (is_float(val)) {
        printf("%.17g ", val->fval);
    } else if (is_bool(val)) {
        printf("%s ", val->bval ? "true" : "false");
    } else if (is_none(val)) {
        printf("none ");
    } else if (is_error(val)) {
        printf("error ");
    } else if (is_obj(val)) {
        Object *obj = val->obj;
        if (IS_STR(obj)) {
            printf("%s ", STR_BUF(obj));
        } else {
            printf("<object> ");
        }
    } else {
        NYI();
    }
}

/*
func print(objs ..., sep = ' ', end = '\n', file io.Writer? = null)
*/
static TValue builtin_print(TValue *self, TValue *args, int nargs)
{
    for (int i = 0; i < nargs; ++i) {
        print_value(args + i);
    }
    printf("\n");
    return none_value;
}

/*
func panic(msg str)
*/
static TValue builtin_panic(TValue *self, TValue *args, int nargs)
{
    print_value(args);
    printf("\n");
    exit(1);
    return none_value;
}

static MethodDef builtin_functions[] = {
    { "print", builtin_print },
    { "panic", builtin_panic },
    { NULL },
};

static TypeObject *builtin_types[] = {
    &any_type,
    &type_type,
    &none_type,
    &bool_type,
    &str_type,
    &exc_type,
    // &field_type,
    &cfunc_type,
    &code_type,
    &int_type,
    &float_type,
    &Number_type,
    NULL,
};

static ModuleDef builtin_module = {
    .path = "std/builtin",
    .funcs = builtin_functions,
    .types = builtin_types,
};

void init_builtin_module(void)
{
    kl_new_native_module(&builtin_module);
    // kl_dump_module(m);
}

#ifdef __cplusplus
}
#endif
