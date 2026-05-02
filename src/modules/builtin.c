/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "buffer.h"
#include "modobj.h"
#include "tupleobj.h"

#ifdef __cplusplus
extern "C" {
#endif

static void print_value(TValue *val)
{
    if (is_int(val)) {
        int ti = val->tag & 0b0011;
        if (ti == 0) {
            printf("%d", (int8_t)val->ival);
        } else if (ti == 1) {
            printf("%d", (int16_t)val->ival);
        } else if (ti == 2) {
            printf("%d", (int32_t)val->ival);
        } else {
            printf("%" PRId64 "", val->ival);
        }
    } else if (is_uint(val)) {
        int ti = val->tag & 0b0011;
        if (ti == 0) {
            printf("%u", (uint8_t)val->ival);
        } else if (ti == 1) {
            printf("%u", (uint16_t)val->ival);
        } else if (ti == 2) {
            printf("%u", (uint32_t)val->ival);
        } else {
            printf("%" PRIu64 "", val->ival);
        }
    } else if (is_float(val)) {
        printf("%.17g", val->fval);
    } else if (is_bool(val)) {
        printf("%s", val->bval ? "true" : "false");
    } else if (is_none(val)) {
        printf("none");
    } else if (is_error(val)) {
        printf("error");
    } else if (is_obj(val)) {
        Object *obj = val->obj;
        if (IS_STR(obj)) {
            printf("%s", STR_BUF(obj));
        } else {
            printf("<object>");
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
    ASSERT(nargs == 3);
    Object *tuple = to_obj(&args[0]);
    Object *sep = to_obj(&args[1]);
    Object *end = to_obj(&args[2]);

    TValue *items = TUPLE_ITEMS(tuple);
    int size = TUPLE_SIZE(tuple);
    for (int i = 0; i < size; ++i) {
        print_value(items + i);
        if (i < size - 1) {
            printf("%s", STR_BUF(sep));
        } else {
            printf("%s", STR_BUF(end));
        }
    }
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
    &tuple_type,
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
