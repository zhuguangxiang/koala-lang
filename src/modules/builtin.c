/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "buffer.h"
#include "listobj.h"
#include "modobj.h"
#include "rangeobj.h"
#include "tupleobj.h"

#ifdef __cplusplus
extern "C" {
#endif

static void print_value(TValue *val)
{
    if (is_obj(val)) {
        Object *obj = to_obj(val);
        if (IS_STR(obj)) {
            printf("%s", STR_BUF(obj));
            return;
        }
    }

    Object *sobj = kl_to_str(val);
    printf("%s", STR_BUF(sobj));
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
    &any_type,   &type_type,  &none_type, &bool_type, &str_type,   &exc_type,
    &field_type, &cfunc_type, &code_type, &int_type,  &float_type, &Number_type,
    &tuple_type, &range_type, &list_type, NULL,
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
