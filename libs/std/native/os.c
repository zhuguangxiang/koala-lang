/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "args.h"
#include "atom.h"
#include "listobj.h"

#ifdef __cplusplus
extern "C" {
#endif

static TValue os_getenv(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    char *name = kl_arg_str(0);
    char *v = getenv(name);
    if (!v) return none_value;
    return kl_val_str(atom(v));
}

static TValue os_setenv(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    char *name = kl_arg_str(0);
    char *value = kl_arg_str(1);
    setenv(name, value, 1);
    return none_value;
}

static TValue os_unsetenv(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    char *name = kl_arg_str(0);
    unsetenv(name);
    return none_value;
}

extern KoalaOptions cmd_opt;

static TValue os_args(TValue *self, TValue *args, int nargs)
{
    Object *list = kl_new_list();

    for (int i = 1; i < cmd_opt.argc; i++) {
        TValue s = kl_val_str(cmd_opt.argv[i]);
        kl_list_append(list, s);
    }

    return obj_value(list);
}

void os_native_lib_init(NativeLib *lib)
{
    kl_reg_func(lib, "getenv", os_getenv);
    kl_reg_func(lib, "setenv", os_setenv);
    kl_reg_func(lib, "unsetenv", os_unsetenv);
    kl_reg_func(lib, "args", os_args);
}

#ifdef __cplusplus
}
#endif
